/* ============================================================
* Quill is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 Quill Team <quillteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "interactivemark.h"

#include <QTimer>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include "newsmodel.h"
#include "newsview.h"
#include "mainapplication.h"

// ----------------------------------------------------------------------------
InteractiveMarkController::InteractiveMarkController(NewsView *view, NewsModel *model,
                                                     QSqlDatabase db, QObject *parent)
  : QObject(parent)
  , view_(view)
  , model_(model)
  , db_(db)
  , hoverMarkRead_(false)
  , hoverDelay_(1000)
  , scrollMarkRead_(false)
  , viewportMarkRead_(false)
  , excludeOnlyStarred_(false)
  , isReadLaterView_(false)
  , hoverRow_(-1)
  , hoverCheckedNewsId_(-1)
{
  hoverTimer_ = new QTimer(this);
  hoverTimer_->setSingleShot(true);
  connect(hoverTimer_, SIGNAL(timeout()), this, SLOT(slotHoverTimeout()));
}

// ----------------------------------------------------------------------------
void InteractiveMarkController::setConfig(bool hoverMarkRead, int hoverDelay,
                                          bool scrollMarkRead, bool viewportMarkRead,
                                          bool excludeOnlyStarred,
                                          const QStringList &excludedGroups,
                                          const QStringList &excludedFeeds,
                                          bool isReadLaterView)
{
  hoverMarkRead_ = hoverMarkRead;
  hoverDelay_ = hoverDelay;
  scrollMarkRead_ = scrollMarkRead;
  viewportMarkRead_ = viewportMarkRead;
  excludeOnlyStarred_ = excludeOnlyStarred;
  excludedGroups_ = excludedGroups;
  excludedFeeds_ = excludedFeeds;
  isReadLaterView_ = isReadLaterView;
}

// ----------------------------------------------------------------------------
int InteractiveMarkController::newsIdByRow(int row) const
{
  if (!model_ || row < 0 || row >= model_->rowCount()) return -1;
  return model_->dataField(row, "id").toInt();
}

// ----------------------------------------------------------------------------
bool InteractiveMarkController::isExcludedFeed(int row) const
{
  if (!model_ || row < 0 || row >= model_->rowCount()) return false;
  int feedId = model_->dataField(row, "feedId").toInt();
  return excludedFeeds_.contains(QString::number(feedId));
}

// ----------------------------------------------------------------------------
bool InteractiveMarkController::isExcludedGroup(int row) const
{
  if (!model_ || row < 0 || row >= model_->rowCount()) return false;
  if (excludedGroups_.isEmpty()) return false;

  int feedId = model_->dataField(row, "feedId").toInt();
  QList<int> visitedIds;
  int currentId = feedId;

  QSqlQuery q(db_);
  while (currentId > 0) {
    if (visitedIds.contains(currentId)) break;
    visitedIds.append(currentId);
    if (excludedGroups_.contains(QString::number(currentId)))
      return true;
    q.prepare("SELECT parentId FROM feeds WHERE id = ?");
    q.addBindValue(currentId);
    q.exec();
    if (!q.next()) break;
    currentId = q.value(0).toInt();
  }
  return false;
}

// ----------------------------------------------------------------------------
void InteractiveMarkController::markRowsRead(const QList<int> &rows, bool recordUndo)
{
  if (rows.isEmpty()) return;
  if (isReadLaterView_) return;

  QList<int> idsToMark;
  foreach (int row, rows) {
    if (isExcludedFeed(row) || isExcludedGroup(row)) continue;
    int id = newsIdByRow(row);
    if (id <= 0) continue;
    if (excludeOnlyStarred_ && model_->dataField(row, "starred").toInt() == 1)
      continue;
    if (model_->dataField(row, "read").toInt() == 1)
      continue;
    if (idsToMark.contains(id)) continue;
    idsToMark.append(id);
  }
  if (idsToMark.isEmpty()) return;

  QStringList idStrs;
  foreach (int id, idsToMark)
    idStrs << QString::number(id);

  db_.transaction();
  QSqlQuery q(db_);
  q.exec(QString("UPDATE news SET read=1, lastReadTime='%1' WHERE id IN (%2)")
         .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate))
         .arg(idStrs.join(",")));
  db_.commit();

  // Persist to disk shortly after marking read, so a crash does not lose it.
  emit mainApp->signalRequestSaveMemoryDB();

  // Update in-memory model state
  for (int i = 0; i < model_->rowCount(); i++) {
    if (idsToMark.contains(newsIdByRow(i))) {
      model_->setData(model_->index(i, model_->fieldIndex("read")), 1);
    }
  }
  view_->viewport()->update();

  emit rowsMarkedRead(idsToMark.count());

  if (recordUndo)
    lastBulkMarkedIds_ = idsToMark;
}

// ----------------------------------------------------------------------------
void InteractiveMarkController::markAboveRead(int sourceRow)
{
  if (sourceRow <= 0) return;
  if (isReadLaterView_) return;

  QList<int> rows;
  for (int row = 0; row < sourceRow; row++)
    rows.append(row);
  markRowsRead(rows, true);
}

// ----------------------------------------------------------------------------
void InteractiveMarkController::markBelowRead(int sourceRow)
{
  if (isReadLaterView_) return;

  QList<int> rows;
  for (int row = sourceRow + 1; row < model_->rowCount(); row++)
    rows.append(row);
  markRowsRead(rows, true);
}

// ----------------------------------------------------------------------------
void InteractiveMarkController::markRowRead(int row)
{
  QList<int> rows;
  rows << row;
  markRowsRead(rows, false);
}

// ----------------------------------------------------------------------------
void InteractiveMarkController::slotHoverRowChanged(int row)
{
  hoverRow_ = row;
  if (row < 0) return;
  if (!hoverMarkRead_) return;

  if (!hoverTimer_->isActive()) {
    hoverCheckedNewsId_ = -1;
    hoverTimer_->start(hoverDelay_);
  }
}

// ----------------------------------------------------------------------------
void InteractiveMarkController::slotHoverTimeout()
{
  if (hoverRow_ < 0) return;
  if (!hoverMarkRead_) return;

  int newsId = newsIdByRow(hoverRow_);
  if (newsId == hoverCheckedNewsId_) return;

  hoverCheckedNewsId_ = newsId;
  markRowRead(hoverRow_);
}

// ----------------------------------------------------------------------------
void InteractiveMarkController::slotRowsScrolledOut(QList<int> rows)
{
  if (!scrollMarkRead_) return;
  markRowsRead(rows, true);
}

// ----------------------------------------------------------------------------
void InteractiveMarkController::slotRowsInserted()
{
  if (!viewportMarkRead_) return;

  // Find rows currently inside the viewport and mark them
  if (!view_) return;
  QList<int> visibleRows;
  int rowCount = model_->rowCount();
  for (int row = 0; row < rowCount; row++) {
    QModelIndex index = model_->index(row, 0);
    if (!index.isValid()) continue;
    if (view_->visualRect(index).intersects(view_->viewport()->rect()))
      visibleRows.append(row);
  }
  markRowsRead(visibleRows, false);
}

// ----------------------------------------------------------------------------
bool InteractiveMarkController::undoLastBulkMark()
{
  if (lastBulkMarkedIds_.isEmpty()) return false;

  QStringList idStrs;
  foreach (int id, lastBulkMarkedIds_)
    idStrs << QString::number(id);

  db_.transaction();
  QSqlQuery q(db_);
  q.exec(QString("UPDATE news SET read=0 WHERE id IN (%1)").arg(idStrs.join(",")));
  db_.commit();

  // Persist the unread state to disk shortly after the undo.
  emit mainApp->signalRequestSaveMemoryDB();

  // Update model state back to unread
  for (int i = 0; i < model_->rowCount(); i++) {
    if (lastBulkMarkedIds_.contains(newsIdByRow(i))) {
      model_->setData(model_->index(i, model_->fieldIndex("read")), 0);
    }
  }
  view_->viewport()->update();

  lastBulkMarkedIds_.clear();
  emit undoDone();
  return true;
}

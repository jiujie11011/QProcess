/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 QuiteRSS Team <quiterssteam@gmail.com>
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
#ifndef INTERACTIVEMARK_H
#define INTERACTIVEMARK_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QSqlDatabase>
#include <QTimer>

class NewsView;
class NewsModel;

/*! Interactive "mark as read" controller.
 *
 * Implements mark-as-read on hover, on scroll-out and on entering the
 * viewport, honouring excluded feeds/groups and offering undo for the
 * last bulk mark operation. Configured through the interaction settings.
 */
class InteractiveMarkController : public QObject
{
  Q_OBJECT
public:
  explicit InteractiveMarkController(NewsView *view, NewsModel *model,
                                     QSqlDatabase db, QObject *parent = 0);

  /*! Enable/disable automatic marking and set timing.
   *  isReadLaterView must be true for star/label filtered views
   *  where automatic marking is skipped. */
  void setConfig(bool hoverMarkRead, int hoverDelay,
                 bool scrollMarkRead, bool viewportMarkRead,
                 bool excludeOnlyStarred,
                 const QStringList &excludedGroups,
                 const QStringList &excludedFeeds,
                 bool isReadLaterView);

  /*! Restore read=0 for the last bulk-marked rows. Returns false if there
   *  is nothing to undo. */
  bool undoLastBulkMark();

  /*! Whether an undo is currently available. */
  bool canUndo() const { return !lastBulkMarkedIds_.isEmpty(); }

public slots:
  void slotHoverRowChanged(int row);
  void slotRowsScrolledOut(QList<int> rows);
  void slotRowsInserted();
  void slotHoverTimeout();

signals:
  /*! Emitted after a bulk mark was undone. */
  void undoDone();
  /*! Emitted after rows were auto-marked as read. */
  void rowsMarkedRead(int count);

private:
  int  newsIdByRow(int row) const;
  bool isExcludedFeed(int row) const;
  bool isExcludedGroup(int row) const;
  void markRowsRead(const QList<int> &rows, bool recordUndo);
  void markRowRead(int row);

  NewsView *view_;
  NewsModel *model_;
  QSqlDatabase db_;

  bool hoverMarkRead_;
  int  hoverDelay_;
  bool scrollMarkRead_;
  bool viewportMarkRead_;
  bool excludeOnlyStarred_;
  QStringList excludedGroups_;
  QStringList excludedFeeds_;
  bool isReadLaterView_;

  int hoverRow_;
  int hoverCheckedNewsId_;
  QTimer *hoverTimer_;

  QList<int> lastBulkMarkedIds_;
};

#endif // INTERACTIVEMARK_H

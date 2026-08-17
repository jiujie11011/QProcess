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
#include "dedupdialog.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

// ----------------------------------------------------------------------------
DedupDialog::DedupDialog(QWidget *parent, const QList<int> &feedIds,
                         const QString &groupName)
  : QDialog(parent)
  , feedIds_(feedIds)
  , groupName_(groupName)
{
  setWindowTitle(tr("Duplicate articles in \"%1\"").arg(groupName_));
  setMinimumSize(760, 480);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  infoLabel_ = new QLabel(tr("Click \"Scan\" to look for articles with the "
                             "same link or the same title."));
  infoLabel_->setWordWrap(true);
  mainLayout->addWidget(infoLabel_);

  table_ = new QTableWidget;
  table_->setColumnCount(5);
  table_->setHorizontalHeaderLabels(QStringList()
      << tr("Group") << tr("Feed") << tr("Title") << tr("Published") << tr("Link"));
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::MultiSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->setAlternatingRowColors(true);
  mainLayout->addWidget(table_);

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  scanButton_ = new QPushButton(tr("Scan"));
  markReadButton_ = new QPushButton(tr("Mark selected as read"));
  deleteButton_ = new QPushButton(tr("Delete selected"));
  closeButton_ = new QPushButton(tr("Close"));
  markReadButton_->setEnabled(false);
  deleteButton_->setEnabled(false);
  buttonLayout->addWidget(scanButton_);
  buttonLayout->addWidget(markReadButton_);
  buttonLayout->addWidget(deleteButton_);
  buttonLayout->addStretch();
  buttonLayout->addWidget(closeButton_);
  mainLayout->addLayout(buttonLayout);

  connect(scanButton_, SIGNAL(clicked()), this, SLOT(slotScan()));
  connect(markReadButton_, SIGNAL(clicked()), this, SLOT(slotMarkSelectedRead()));
  connect(deleteButton_, SIGNAL(clicked()), this, SLOT(slotDeleteSelected()));
  connect(closeButton_, SIGNAL(clicked()), this, SLOT(slotClose()));
  connect(table_, &QTableWidget::itemSelectionChanged, this, [this]() {
    bool has = !table_->selectedItems().isEmpty();
    markReadButton_->setEnabled(has);
    deleteButton_->setEnabled(has);
  });

  // Trigger the first scan automatically.
  slotScan();
}

// ----------------------------------------------------------------------------
QString DedupDialog::normalize(const QString &text) const
{
  QString s = text.trimmed().toLower();
  // Keep letters/digits, drop everything else (punctuation, whitespace).
  QString result;
  result.reserve(s.size());
  foreach (const QChar &c, s) {
    if (c.isLetterOrNumber())
      result.append(c);
  }
  return result;
}

// ----------------------------------------------------------------------------
QList<DedupDialog::ArticleInfo> DedupDialog::loadArticles() const
{
  QList<ArticleInfo> result;
  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isOpen() || feedIds_.isEmpty())
    return result;

  QString placeholders;
  QStringList binds;
  foreach (int id, feedIds_) {
    if (!placeholders.isEmpty())
      placeholders += ",";
    placeholders += "?";
    binds << QString::number(id);
  }

  QSqlQuery q(db);
  q.prepare("SELECT n.id, n.feedId, n.title, n.link_href, n.published, f.text "
            "FROM news n LEFT JOIN feeds f ON n.feedId=f.id "
            "WHERE n.feedId IN (" + placeholders + ") AND n.deleted==0 "
            "ORDER BY n.published DESC");
  foreach (const QString &b, binds)
    q.addBindValue(b);
  if (!q.exec()) {
    qWarning() << "DedupDialog::loadArticles error:" << q.lastError().text();
    return result;
  }
  while (q.next()) {
    ArticleInfo a;
    a.id = q.value(0).toInt();
    a.feedId = q.value(1).toInt();
    a.title = q.value(2).toString();
    a.link = q.value(3).toString();
    a.published = q.value(4).toString();
    a.feedName = q.value(5).toString();
    result.append(a);
  }
  return result;
}

// ----------------------------------------------------------------------------
void DedupDialog::slotScan()
{
  QList<ArticleInfo> articles = loadArticles();
  fillTable(articles);
}

// ----------------------------------------------------------------------------
void DedupDialog::fillTable(const QList<ArticleInfo> &articles)
{
  table_->setRowCount(0);

  if (articles.isEmpty()) {
    infoLabel_->setText(tr("No articles found in this group."));
    return;
  }

  // Group by normalized link and by normalized title.
  QHash<QString, QList<int> > byLink;
  QHash<QString, QList<int> > byTitle;
  for (int i = 0; i < articles.size(); ++i) {
    const ArticleInfo &a = articles.at(i);
    if (!a.link.isEmpty())
      byLink[normalize(a.link)].append(i);
    QString nt = normalize(a.title);
    if (!nt.isEmpty())
      byTitle[nt].append(i);
  }

  // Assign a sequential group number per cluster of duplicates.
  QHash<int, int> rowGroup;   // original index -> group number
  int nextGroup = 1;
  QHashIterator<QString, QList<int> > it(byLink);
  while (it.hasNext()) {
    it.next();
    if (it.value().size() > 1) {
      foreach (int row, it.value())
        rowGroup.insert(row, nextGroup);
      ++nextGroup;
    }
  }
  QHashIterator<QString, QList<int> > it2(byTitle);
  while (it2.hasNext()) {
    it2.next();
    if (it2.value().size() > 1) {
      foreach (int row, it2.value()) {
        if (!rowGroup.contains(row))
          rowGroup.insert(row, nextGroup);
      }
      ++nextGroup;
    }
  }

  // Only show rows that are part of a duplicate cluster.
  QList<int> showRows;
  foreach (int row, rowGroup.keys())
    showRows << row;
  std::sort(showRows.begin(), showRows.end());

  table_->setRowCount(showRows.size());
  for (int r = 0; r < showRows.size(); ++r) {
    int row = showRows.at(r);
    const ArticleInfo &a = articles.at(row);
    QTableWidgetItem *gItem = new QTableWidgetItem(
        QString::number(rowGroup.value(row)));
    QTableWidgetItem *fItem = new QTableWidgetItem(a.feedName);
    QTableWidgetItem *tItem = new QTableWidgetItem(a.title);
    QTableWidgetItem *pItem = new QTableWidgetItem(a.published);
    QTableWidgetItem *lItem = new QTableWidgetItem(a.link);
    gItem->setData(Qt::UserRole, a.id);
    table_->setItem(r, 0, gItem);
    table_->setItem(r, 1, fItem);
    table_->setItem(r, 2, tItem);
    table_->setItem(r, 3, pItem);
    table_->setItem(r, 4, lItem);
  }

  int dupCount = rowGroup.size();
  if (dupCount == 0)
    infoLabel_->setText(tr("No duplicate articles found in this group."));
  else
    infoLabel_->setText(tr("Found %1 duplicate article(s) in this group "
                           "(same link or same title). Select rows and mark "
                           "them as read or delete them.").arg(dupCount));
}

// ----------------------------------------------------------------------------
void DedupDialog::slotMarkSelectedRead()
{
  QList<int> ids;
  foreach (const QModelIndex &idx, table_->selectionModel()->selectedRows()) {
    QTableWidgetItem *gItem = table_->item(idx.row(), 0);
    if (gItem)
      ids.append(gItem->data(Qt::UserRole).toInt());
  }
  if (ids.isEmpty())
    return;

  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isOpen())
    return;
  QSqlQuery q(db);
  q.prepare("UPDATE news SET read=1 WHERE id=?");
  foreach (int id, ids) {
    q.addBindValue(id);
    if (!q.exec())
      qWarning() << "DedupDialog mark read error:" << q.lastError().text();
  }
  slotScan();
}

// ----------------------------------------------------------------------------
void DedupDialog::slotDeleteSelected()
{
  QList<int> ids;
  foreach (const QModelIndex &idx, table_->selectionModel()->selectedRows()) {
    QTableWidgetItem *gItem = table_->item(idx.row(), 0);
    if (gItem)
      ids.append(gItem->data(Qt::UserRole).toInt());
  }
  if (ids.isEmpty())
    return;

  if (QMessageBox::question(this, tr("Delete articles"),
        tr("Delete %1 selected article(s)? This cannot be undone.")
          .arg(ids.size())) != QMessageBox::Yes)
    return;

  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isOpen())
    return;
  QSqlQuery q(db);
  q.prepare("UPDATE news SET deleted=1 WHERE id=?");
  foreach (int id, ids) {
    q.addBindValue(id);
    if (!q.exec())
      qWarning() << "DedupDialog delete error:" << q.lastError().text();
  }
  slotScan();
}

// ----------------------------------------------------------------------------
void DedupDialog::slotClose()
{
  accept();
}

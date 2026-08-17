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
#include "feedsmanagementdialog.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSqlQuery>
#include <QSqlError>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>

enum {
  ColId = 0,
  ColName,
  ColGroup,
  ColUpdated,
  ColInterval,
  ColStatus
};

// ----------------------------------------------------------------------------
FeedsManagementDialog::FeedsManagementDialog(QWidget *parent, QSqlDatabase db)
  : QDialog(parent)
  , db_(db)
{
  setWindowTitle(tr("Feeds Management"));
  setMinimumSize(560, 400);

  feedsTree_ = new QTreeWidget();
  feedsTree_->setColumnCount(6);
  QStringList headers;
  headers << tr("ID") << tr("Name") << tr("Group") << tr("Updated")
          << tr("Update Interval") << tr("Status");
  feedsTree_->setHeaderLabels(headers);
  feedsTree_->setRootIsDecorated(false);
  feedsTree_->setAlternatingRowColors(true);
  feedsTree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  feedsTree_->header()->setStretchLastSection(true);
  feedsTree_->header()->setSortIndicator(0, Qt::AscendingOrder);
  feedsTree_->setSortingEnabled(true);

  QPushButton *deleteButton = new QPushButton(tr("Delete Selected..."));
  connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteSelected()));

  QPushButton *markAllButton = new QPushButton(tr("Select All"));
  connect(markAllButton, SIGNAL(clicked()), this, SLOT(markAllFeeds()));

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QHBoxLayout *topLayout = new QHBoxLayout();
  topLayout->addWidget(deleteButton);
  topLayout->addWidget(markAllButton);
  topLayout->addStretch();

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(topLayout);
  mainLayout->addWidget(feedsTree_);
  mainLayout->addWidget(buttonBox);

  loadFeeds();
}

// ----------------------------------------------------------------------------
FeedsManagementDialog::~FeedsManagementDialog()
{
}

// ----------------------------------------------------------------------------
void FeedsManagementDialog::loadFeeds()
{
  feedsTree_->clear();
  deletedFeedIds_.clear();

  QSqlQuery q(db_);
  if (!q.exec("SELECT id, text, parentId, updated, updateIntervalEnable, "
              "updateInterval, updateIntervalType, xmlUrl "
              "FROM feeds ORDER BY parentId, rowToParent")) {
    qDebug() << "FeedsManagementDialog: query failed:" << q.lastError().text();
    return;
  }

  QSqlQuery groupQuery(db_);
  QHash<int, QString> groupNames;
  if (groupQuery.exec("SELECT id, text FROM feeds WHERE xmlUrl=''")) {
    while (groupQuery.next())
      groupNames.insert(groupQuery.value(0).toInt(),
                        groupQuery.value(1).toString());
  }

  while (q.next()) {
    int id = q.value(0).toInt();
    QString name = q.value(1).toString();
    int parentId = q.value(2).toInt();
    QString updated = q.value(3).toString();
    bool intervalEnable = q.value(4).toBool();
    int interval = q.value(5).toInt();
    QString intervalType = q.value(6).toString();
    bool isGroup = q.value(7).toString().isEmpty();

    QString groupName = groupNames.value(parentId);
    if (groupName.isEmpty()) groupName = tr("(root)");

    QString intervalText = tr("disabled");
    if (intervalEnable && !isGroup) {
      intervalText = QString::number(interval) + " " + intervalType;
    }

    QString statusText = tr("normal");
    if (isGroup)
      statusText = tr("group");
    else {
      QDateTime updatedTime = QDateTime::fromString(updated, Qt::ISODate);
      if (updatedTime.isValid() && updatedTime.daysTo(QDateTime::currentDateTimeUtc()) > 14)
        statusText = tr("stale");
      if (!intervalEnable)
        statusText = tr("paused");
    }

    QStringList cols;
    cols << QString::number(id) << name << groupName << updated << intervalText << statusText;
    QTreeWidgetItem *item = new QTreeWidgetItem(feedsTree_, cols);
    item->setData(0, Qt::UserRole, id);
    item->setData(0, Qt::UserRole + 1, isGroup);
  }
}

// ----------------------------------------------------------------------------
void FeedsManagementDialog::deleteSelected()
{
  QList<QTreeWidgetItem *> selected = feedsTree_->selectedItems();
  if (selected.isEmpty()) return;

  QStringList idList;
  foreach (QTreeWidgetItem *item, selected)
    idList << item->text(0);
  if (idList.isEmpty()) return;

  QMessageBox::StandardButton answer =
      QMessageBox::question(this, tr("Delete Feeds"),
                            tr("Delete %1 selected item(s)? "
                               "News of deleted feeds will be removed too.").
                            arg(idList.count()),
                            QMessageBox::Yes | QMessageBox::No,
                            QMessageBox::No);
  if (answer != QMessageBox::Yes) return;

  db_.transaction();
  QSqlQuery q(db_);
  foreach (const QString &idStr, idList) {
    int id = idStr.toInt();

    q.exec(QString("SELECT id FROM feeds WHERE parentId=%1").arg(id));
    QList<int> childrenIds;
    while (q.next()) childrenIds.append(q.value(0).toInt());

    // Delete news of this feed and of all children
    QStringList allIds;
    allIds << QString::number(id);
    foreach (int childId, childrenIds)
      allIds << QString::number(childId);

    q.exec(QString("DELETE FROM news WHERE feedId IN (%1)").
           arg(allIds.join(",")));
    q.exec(QString("DELETE FROM feeds WHERE id=%1").arg(id));
    q.exec(QString("DELETE FROM feeds WHERE parentId=%1").arg(id));

    deletedFeedIds_.append(id);
    foreach (int childId, childrenIds)
      deletedFeedIds_.append(childId);
  }
  db_.commit();

  loadFeeds();
}

// ----------------------------------------------------------------------------
void FeedsManagementDialog::markAllFeeds()
{
  feedsTree_->selectAll();
}

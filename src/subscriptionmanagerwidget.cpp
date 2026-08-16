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
#include "subscriptionmanagerwidget.h"

#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTreeWidget>
#include <QVBoxLayout>

SubscriptionManagerWidget::SubscriptionManagerWidget(QWidget *parent)
  : QWidget(parent)
{
  addButton_ = new QPushButton(tr("Add Subscription..."));
  addButton_->setObjectName("addSubscriptionButton");
  labelsButton_ = new QPushButton(tr("Manage Labels..."));
  labelsButton_->setObjectName("manageLabelsButton");
  deleteButton_ = new QPushButton(tr("Delete Selected"));
  deleteButton_->setObjectName("deleteSubscriptionButton");
  deleteButton_->setEnabled(false);
  selectAllButton_ = new QPushButton(tr("Select All"));
  selectAllButton_->setObjectName("selectAllSubscriptionsButton");
  selectAllButton_->setCheckable(true);

  summaryLabel_ = new QLabel();
  summaryLabel_->setObjectName("subscriptionSummaryLabel");

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->setContentsMargins(0, 0, 0, 0);
  buttonLayout->addWidget(addButton_);
  buttonLayout->addWidget(labelsButton_);
  buttonLayout->addWidget(deleteButton_);
  buttonLayout->addStretch();
  buttonLayout->addWidget(selectAllButton_);

  tree_ = new QTreeWidget();
  tree_->setObjectName("subscriptionManagerTree");
  tree_->setColumnCount(6);
  QStringList headers;
  headers << tr("Select") << tr("Name") << tr("Category")
          << tr("Frequency") << tr("Status") << tr("Last Update");
  tree_->setHeaderLabels(headers);
  tree_->setRootIsDecorated(false);
  tree_->setSortingEnabled(true);
  tree_->sortItems(1, Qt::AscendingOrder);
  tree_->setSelectionMode(QAbstractItemView::NoSelection);
  tree_->setAlternatingRowColors(true);
  tree_->header()->setStretchLastSection(true);
  tree_->header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addLayout(buttonLayout);
  mainLayout->addWidget(tree_);
  mainLayout->addWidget(summaryLabel_);
  setLayout(mainLayout);

  connect(addButton_, SIGNAL(clicked()), this, SIGNAL(addFeedRequested()));
  connect(labelsButton_, SIGNAL(clicked()), this, SIGNAL(manageLabelsRequested()));
  connect(deleteButton_, SIGNAL(clicked()), this, SLOT(deleteSelectedItems()));
  connect(selectAllButton_, SIGNAL(toggled(bool)),
          this, SLOT(selectAllItems(bool)));
  connect(tree_, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this, SLOT(updateSummary()));
  connect(tree_, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
          this, SLOT(updateSummary()));
}

QString SubscriptionManagerWidget::categoryName(int parentId) const
{
  if (parentId <= 0)
    return QString();
  QSqlDatabase db = QSqlDatabase::database();
  QSqlQuery query(db);
  query.prepare("SELECT text FROM feeds WHERE id = ?");
  query.addBindValue(parentId);
  if (query.exec() && query.next())
    return query.value(0).toString();
  return QString();
}

QString SubscriptionManagerWidget::frequencyText(int interval, int type) const
{
  // type: 0 = seconds, 1 = minutes, 2 = hours
  if (type == 0)
    return tr("%1 seconds").arg(interval);
  if (type == 2)
    return tr("%1 hours").arg(interval);
  return tr("%1 minutes").arg(interval);
}

void SubscriptionManagerWidget::refresh()
{
  loadFeeds();
}

void SubscriptionManagerWidget::loadFeeds()
{
  tree_->clear();

  QSqlDatabase db = QSqlDatabase::database();
  QSqlQuery query(db);
  query.exec("SELECT id, text, xmlUrl, parentId, updateInterval, "
             "updateIntervalType, status, updated "
             "FROM feeds WHERE hasChildren == 0 ORDER BY text COLLATE NOCASE");

  int total = 0;
  while (query.next()) {
    QTreeWidgetItem *item = new QTreeWidgetItem(tree_);
    item->setCheckState(0, Qt::Unchecked);
    item->setData(0, Qt::UserRole, query.value(0).toInt());
    item->setText(1, query.value(1).toString());
    item->setText(2, categoryName(query.value(3).toInt()));

    bool intervalEnabled = query.value(4).toInt() > 0;
    QString freq;
    if (intervalEnabled) {
      freq = frequencyText(query.value(4).toInt(),
                           query.value(5).toInt());
    } else {
      freq = tr("Default");
    }
    item->setText(3, freq);

    QString status = query.value(6).toString().trimmed();
    QString statusText = tr("OK");
    if (!status.isEmpty() && status != "0")
      statusText = status;
    item->setText(4, statusText);
    item->setData(4, Qt::UserRole, status);

    QString updated = query.value(7).toString();
    item->setText(5, updated.section('T', 0, 0) + " " +
                       updated.section('T', 1, 1).section('.', 0, 0));
    item->setTextAlignment(0, Qt::AlignCenter);
    total++;
  }

  Q_UNUSED(total);
  updateSummary();
}

void SubscriptionManagerWidget::selectAllItems(bool checked)
{
  for (int i = 0; i < tree_->topLevelItemCount(); ++i)
    tree_->topLevelItem(i)->setCheckState(0,
        checked ? Qt::Checked : Qt::Unchecked);
  updateSummary();
}

void SubscriptionManagerWidget::updateSummary()
{
  int checkedCount = 0;
  int total = tree_->topLevelItemCount();
  for (int i = 0; i < total; ++i) {
    if (tree_->topLevelItem(i)->checkState(0) == Qt::Checked)
      checkedCount++;
  }
  summaryLabel_->setText(tr("%1 subscriptions, %2 selected")
                         .arg(total).arg(checkedCount));
  deleteButton_->setEnabled(checkedCount > 0);
}

void SubscriptionManagerWidget::deleteSelectedItems()
{
  QList<int> idList;
  for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
    QTreeWidgetItem *item = tree_->topLevelItem(i);
    if (item->checkState(0) == Qt::Checked)
      idList << item->data(0, Qt::UserRole).toInt();
  }
  if (idList.isEmpty())
    return;

  QMessageBox msgBox(this);
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle(tr("Confirm Delete"));
  msgBox.setText(tr("Are you sure to delete selected subscriptions?"));
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);
  if (msgBox.exec() == QMessageBox::No)
    return;

  QString idStr;
  QString feedIdStr;
  foreach (int id, idList) {
    if (idStr.isEmpty()) {
      idStr = QString("id='%1'").arg(id);
      feedIdStr = QString("feedId='%1'").arg(id);
    } else {
      idStr += QString(" OR id='%1'").arg(id);
      feedIdStr += QString(" OR feedId='%1'").arg(id);
    }
  }

  QSqlDatabase db = QSqlDatabase::database();
  db.transaction();
  QSqlQuery query(db);
  query.exec(QString("DELETE FROM feeds WHERE %1").arg(idStr));
  query.exec(QString("DELETE FROM news WHERE %1").arg(feedIdStr));
  db.commit();

  emit feedsChanged();
  refresh();
  emit statusMessage(tr("Selected subscriptions deleted."));
}

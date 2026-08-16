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
#ifndef SUBSCRIPTIONMANAGERWIDGET_H
#define SUBSCRIPTIONMANAGERWIDGET_H

#include <QList>
#include <QWidget>

class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

/** @brief Manage subscriptions (feeds) inside the Options dialog.
 *
 * Lists all real feeds (folders are shown via the Category column) with
 * sortable columns (name, category, update frequency, status), supports
 * multi-select via checkboxes, and provides add / delete / move to
 * another group / manage labels actions.
 */
class SubscriptionManagerWidget : public QWidget
{
  Q_OBJECT
public:
  explicit SubscriptionManagerWidget(QWidget *parent = 0);

  /** Reloads the feed list from the database. */
  void refresh();

signals:
  void addFeedRequested();
  void manageLabelsRequested();
  void feedsChanged();
  void statusMessage(const QString &message);
  /** Request a status check (re-fetch) for the given feed ids. */
  void checkStatusRequested(const QList<int> &feedIds);

private slots:
  void selectAllItems(bool checked);
  void updateSummary();
  void deleteSelectedItems();
  void checkSelectedStatus();
  /** Moves all checked feeds into a (possibly new) group. */
  void moveSelectedToGroup();
  void slotHeaderClicked(int column);

private:
  void loadFeeds();
  QString categoryName(int parentId) const;
  QString frequencyText(int interval, int type) const;
  QList<int> checkedFeedIds() const;

  QTreeWidget *tree_;
  QPushButton *addButton_;
  QPushButton *labelsButton_;
  QPushButton *moveToGroupButton_;
  QPushButton *deleteButton_;
  QPushButton *checkStatusButton_;
  QPushButton *selectAllButton_;
  QLabel *summaryLabel_;
};

#endif // SUBSCRIPTIONMANAGERWIDGET_H

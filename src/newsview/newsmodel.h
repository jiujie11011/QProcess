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
#ifndef NEWSMODEL_H
#define NEWSMODEL_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>

class NewsModel : public QSqlTableModel
{
  Q_OBJECT
public:
  /*! Custom roles used by the news list delegate. */
  enum NewsRole {
    SummaryRole = Qt::UserRole + 120   //!< AI summary text (may be empty)
  };

  NewsModel(QObject *parent, QTreeView *view);
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  virtual void sort(int column, Qt::SortOrder order);
  virtual QModelIndexList match(
      const QModelIndex &start, int role, const QVariant &value, int hits = 1,
      Qt::MatchFlags flags =
      Qt::MatchFlags(Qt::MatchExactly|Qt::MatchWrap)
      ) const;
  QVariant dataField(int row, const QString &fieldName) const;
  void setFilter(const QString &filter);
  bool select();

  // Paging: the list is loaded in chunks and grows as the user scrolls.
  // Bulk operations must call fetchAll() first so their rowCount() loops
  // see every matching row.
  static const int pageSize = 500;  //!< rows loaded per select/fetchMore
  void fetchAll();                  //!< load the remaining pages immediately
  bool allFetched() const;          //!< true once every row is loaded

  QString formatDate_;
  QString formatTime_;
  bool simplifiedDateTime_;
  QString textColor_;
  QString newNewsTextColor_;
  QString unreadNewsTextColor_;
  QString focusedNewsTextColor_;
  QString focusedNewsBGColor_;
  bool dimRead_;
  QString dimReadColor_;

signals:
  void signalSort(int column, int order);

protected:
  bool selectWithLimit();

private:
  QTreeView *view_;
  int loadedLimit_;
  mutable int totalCountCache_;
  int totalCount() const;

};

#endif // NEWSMODEL_H

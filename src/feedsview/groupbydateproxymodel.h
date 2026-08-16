/*****************************************************************************
 * GroupByDateProxyModel
 *
 * Groups flat news rows into "Today / Yesterday / Earlier" parent rows
 * (S-2). The underlying news model stays untouched; this proxy only adds
 * tree structure on top of the already sorted source rows.
 *****************************************************************************/

#ifndef GROUPBYDATEPROXYMODEL_H
#define GROUPBYDATEPROXYMODEL_H

#include <QAbstractProxyModel>
#include <QDate>
#include <QList>
#include <QString>

class GroupByDateProxyModel : public QAbstractProxyModel
{
  Q_OBJECT

public:
  explicit GroupByDateProxyModel(QObject *parent = 0);

  void setSourceModel(QAbstractItemModel *sourceModel);

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &child) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
  QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

  // Column used to extract the grouping date (default: 0 -> first column)
  int dateColumn() const { return dateColumn_; }
  void setDateColumn(int column) { dateColumn_ = column; }

  bool isGroupRow(const QModelIndex &proxyIndex) const
  { return proxyIndex.isValid() && !proxyIndex.parent().isValid(); }

  QString groupLabel(int groupRow) const;

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private slots:
  void slotSourceModelReset();
  void slotSourceRowsInserted(const QModelIndex &parent, int first, int last);
  void slotSourceRowsRemoved(const QModelIndex &parent, int first, int last);
  void slotSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  void slotSourceLayoutChanged();

private:
  struct Group {
    QString label;
    QList<int> rows;   // source row numbers in display order
  };

  void rebuildGroups();
  QString groupKeyForRow(int sourceRow) const;

  QList<Group> groups_;
  int dateColumn_;
};

#endif // GROUPBYDATEPROXYMODEL_H

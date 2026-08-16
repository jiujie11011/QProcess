#include "groupbydateproxymodel.h"

#include <QDateTime>
#include <QTime>

#include "newsmodel.h"

GroupByDateProxyModel::GroupByDateProxyModel(QObject *parent)
  : QAbstractProxyModel(parent)
  , dateColumn_(0)
{
}

void GroupByDateProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
  if (this->sourceModel()) {
    disconnect(this->sourceModel(), 0, this, 0);
  }

  QAbstractProxyModel::setSourceModel(sourceModel);

  if (sourceModel) {
    connect(sourceModel, SIGNAL(modelReset()), this, SLOT(slotSourceModelReset()));
    connect(sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(slotSourceRowsInserted(QModelIndex,int,int)));
    connect(sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(slotSourceRowsRemoved(QModelIndex,int,int)));
    connect(sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotSourceDataChanged(QModelIndex,QModelIndex)));
    connect(sourceModel, SIGNAL(layoutChanged()),
            this, SLOT(slotSourceLayoutChanged()));
  }
  rebuildGroups();
}

bool GroupByDateProxyModel::filterAcceptsRow(int source_row,
                                             const QModelIndex &source_parent) const
{
  Q_UNUSED(source_row)
  Q_UNUSED(source_parent)
  return true;
}

QString GroupByDateProxyModel::groupKeyForRow(int sourceRow) const
{
  QAbstractItemModel *model = sourceModel();
  if (!model)
    return QString();

  QModelIndex idx = model->index(sourceRow, dateColumn_);
  QDateTime dt = QDateTime::fromString(idx.data(Qt::EditRole).toString(), Qt::ISODate);
  if (!dt.isValid())
    dt = QDateTime::fromString(idx.data(Qt::DisplayRole).toString(), Qt::ISODate);
  if (!dt.isValid()) {
    // Fallback: parse ISO date part (yyyy-MM-dd)
    QDate d = QDate::fromString(idx.data().toString().left(10), Qt::ISODate);
    if (d.isValid())
      dt = QDateTime(d, QTime(0, 0));
  }
  if (!dt.isValid())
    return tr("Other");

  QDate today = QDate::currentDate();
  if (dt.date() == today)
    return tr("Today");
  if (dt.date() == today.addDays(-1))
    return tr("Yesterday");
  return tr("Earlier");
}

QString GroupByDateProxyModel::groupLabel(int groupRow) const
{
  if (groupRow < 0 || groupRow >= groups_.size())
    return QString();
  return groups_.at(groupRow).label;
}

void GroupByDateProxyModel::rebuildGroups()
{
  QAbstractItemModel *model = sourceModel();
  groups_.clear();
  if (!model)
    return;

  int count = model->rowCount();
  QString curLabel;
  for (int row = 0; row < count; ++row) {
    QString label = groupKeyForRow(row);
    if (groups_.isEmpty() || label != curLabel) {
      Group g;
      g.label = label;
      groups_.append(g);
      curLabel = label;
    }
    groups_.last().rows.append(row);
  }
}

QModelIndex GroupByDateProxyModel::index(int row, int column,
                                         const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  if (!parent.isValid()) {
    if (row < 0 || row >= groups_.size())
      return QModelIndex();
    return createIndex(row, column);
  }

  if (!parent.parent().isValid()) {
    int group = parent.row();
    if (group < 0 || group >= groups_.size())
      return QModelIndex();
    if (row < 0 || row >= groups_.at(group).rows.size())
      return QModelIndex();
    // internalId = group + 1 (0 means "no parent" -> top-level group row)
    return createIndex(row, column, quintptr(group + 1));
  }

  return QModelIndex();
}

QModelIndex GroupByDateProxyModel::parent(const QModelIndex &child) const
{
  if (!child.isValid())
    return QModelIndex();

  quintptr id = child.internalId();
  if (id == 0)
    return QModelIndex();   // group row -> top level

  int group = int(id) - 1;
  if (group < 0 || group >= groups_.size())
    return QModelIndex();
  return createIndex(group, 0);
}

int GroupByDateProxyModel::rowCount(const QModelIndex &parent) const
{
  if (!parent.isValid())
    return groups_.size();
  if (!parent.parent().isValid())
    return groups_.at(parent.row()).rows.size();
  return 0;
}

int GroupByDateProxyModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent)
  QAbstractItemModel *model = sourceModel();
  return model ? model->columnCount() : 0;
}

QVariant GroupByDateProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
  if (!proxyIndex.isValid())
    return QVariant();

  if (!proxyIndex.parent().isValid()) {
    // group row
    int g = proxyIndex.row();
    if (g < 0 || g >= groups_.size())
      return QVariant();
    if (role == Qt::DisplayRole)
      return groups_.at(g).label;
    if (role == Qt::TextAlignmentRole)
      return int(Qt::AlignCenter);
    return QVariant();
  }

  QModelIndex src = mapToSource(proxyIndex);
  return src.data(role);
}

QVariant GroupByDateProxyModel::headerData(int section, Qt::Orientation orientation,
                                           int role) const
{
  QAbstractItemModel *model = sourceModel();
  if (model)
    return model->headerData(section, orientation, role);
  return QVariant();
}

QModelIndex GroupByDateProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
  if (!proxyIndex.isValid())
    return QModelIndex();

  QAbstractItemModel *model = sourceModel();
  if (!model)
    return QModelIndex();

  if (!proxyIndex.parent().isValid())
    return QModelIndex();   // group rows have no source row

  int group = proxyIndex.parent().row();
  int offset = proxyIndex.row();
  if (group < 0 || group >= groups_.size())
    return QModelIndex();
  if (offset < 0 || offset >= groups_.at(group).rows.size())
    return QModelIndex();

  return model->index(groups_.at(group).rows.at(offset), proxyIndex.column());
}

QModelIndex GroupByDateProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
  if (!sourceIndex.isValid())
    return QModelIndex();

  int srcRow = sourceIndex.row();
  for (int g = 0; g < groups_.size(); ++g) {
    int offset = groups_.at(g).rows.indexOf(srcRow);
    if (offset >= 0)
      return index(offset, sourceIndex.column(), index(g, 0));
  }
  return QModelIndex();
}

void GroupByDateProxyModel::slotSourceModelReset()
{
  beginResetModel();
  rebuildGroups();
  endResetModel();
}

void GroupByDateProxyModel::slotSourceRowsInserted(const QModelIndex &parent, int first, int last)
{
  Q_UNUSED(parent)
  Q_UNUSED(first)
  Q_UNUSED(last)
  beginResetModel();
  rebuildGroups();
  endResetModel();
}

void GroupByDateProxyModel::slotSourceRowsRemoved(const QModelIndex &parent, int first, int last)
{
  Q_UNUSED(parent)
  Q_UNUSED(first)
  Q_UNUSED(last)
  beginResetModel();
  rebuildGroups();
  endResetModel();
}

void GroupByDateProxyModel::slotSourceDataChanged(const QModelIndex &topLeft,
                                                  const QModelIndex &bottomRight)
{
  Q_UNUSED(topLeft)
  Q_UNUSED(bottomRight)
  beginResetModel();
  rebuildGroups();
  endResetModel();
}

void GroupByDateProxyModel::slotSourceLayoutChanged()
{
  beginResetModel();
  rebuildGroups();
  endResetModel();
}

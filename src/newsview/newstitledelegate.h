// newstitledelegate.h
#ifndef NEWSTITLEDELEGATE_H
#define NEWSTITLEDELEGATE_H

#include "delegatewithoutfocus.h"

/*! Delegate for the title column of the news list.
 *
 * When a news row carries an AI summary (NewsModel::SummaryRole), the
 * summary is rendered as a second, smaller and italic line directly below
 * the title so it is clearly distinguished from the title and from the
 * article body.
 */
class NewsTitleDelegate : public DelegateWithoutFocus
{
  Q_OBJECT
public:
  explicit NewsTitleDelegate(QObject *parent = 0);

protected:
  virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                     const QModelIndex &index) const;
  virtual QSize sizeHint(const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
};

#endif // NEWSTITLEDELEGATE_H

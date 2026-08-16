// newstitledelegate.cpp
#include "newstitledelegate.h"

#include "newsmodel.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionViewItem>

NewsTitleDelegate::NewsTitleDelegate(QObject *parent)
  : DelegateWithoutFocus(parent)
{
}

void NewsTitleDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
  const QString summary = index.data(NewsModel::SummaryRole).toString();
  if (summary.isEmpty()) {
    DelegateWithoutFocus::paint(painter, option, index);
    return;
  }

  QStyleOptionViewItem opt = option;
  opt.state &= ~QStyle::State_HasFocus;

  QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

  // Background (selection / hover / alternate row color).
  style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter,
                       opt.widget);

  const QRect rect = opt.rect.adjusted(4, 2, -4, -2);
  const bool selected = (opt.state & QStyle::State_Selected);

  // Title (first line).
  QFont titleFont = opt.font;
  const QFont boldFont = index.data(Qt::FontRole).value<QFont>();
  if (boldFont.bold())
    titleFont.setBold(true);

  QColor titleColor = index.data(Qt::TextColorRole).value<QColor>();
  if (!titleColor.isValid())
    titleColor = opt.palette.color(QPalette::Text);
  if (selected)
    titleColor = opt.palette.color(QPalette::HighlightedText);

  const QFontMetrics fm(titleFont);
  const QRect titleRect(rect.left(), rect.top(), rect.width(), fm.height());
  painter->setFont(titleFont);
  painter->setPen(titleColor);
  painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                    fm.elidedText(index.data(Qt::DisplayRole).toString(),
                                  Qt::ElideRight, rect.width()));

  // Summary (second line): smaller, italic, gray.
  QFont summaryFont = opt.font;
  summaryFont.setItalic(true);
  const int pointSize = summaryFont.pointSize();
  if (pointSize > 0)
    summaryFont.setPointSize(pointSize - 1);
  else if (summaryFont.pixelSize() > 0)
    summaryFont.setPixelSize(summaryFont.pixelSize() - 1);

  QColor summaryColor = selected
      ? opt.palette.color(QPalette::HighlightedText)
      : opt.palette.color(QPalette::Disabled, QPalette::Text);
  summaryColor.setAlpha(selected ? 180 : 160);

  const QFontMetrics sfm(summaryFont);
  const QRect summaryRect(rect.left(), titleRect.bottom() + 2, rect.width(),
                          rect.bottom() - titleRect.bottom() - 2);
  if (summaryRect.height() <= 0)
    return;
  painter->setFont(summaryFont);
  painter->setPen(summaryColor);
  painter->drawText(summaryRect, Qt::AlignLeft | Qt::AlignTop,
                    sfm.elidedText(summary, Qt::ElideRight, rect.width()));
}

QSize NewsTitleDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
  QSize size = DelegateWithoutFocus::sizeHint(option, index);
  if (!index.data(NewsModel::SummaryRole).toString().isEmpty()) {
    const int lineHeight = option.fontMetrics.height() + 4;
    size.setHeight(lineHeight * 2);
  }
  return size;
}

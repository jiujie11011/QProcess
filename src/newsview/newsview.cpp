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
#include "newsview.h"
#include "delegatewithoutfocus.h"

#include <QAbstractProxyModel>
#include <functional>

NewsView::NewsView(QWidget * parent)
  : QTreeView(parent)
{
  setObjectName("newsView_");
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setMinimumWidth(120);
  setSortingEnabled(true);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setMouseTracking(true);
  DelegateWithoutFocus *itemDelegate = new DelegateWithoutFocus(this);
  setItemDelegate(itemDelegate);
  setContextMenuPolicy(Qt::CustomContextMenu);
}

/*virtual*/ void NewsView::mousePressEvent(QMouseEvent *event)
{
  if (!indexAt(QEVENT_POS(event)).isValid()) return;
  indexClicked_ = indexAt(QEVENT_POS(event));

  QModelIndex index = indexAt(QEVENT_POS(event));
  QSqlTableModel *model_ = (QSqlTableModel*)model();
  if (event->buttons() & Qt::LeftButton) {
    if (index.column() == model_->fieldIndex("starred")) {
      if (index.data(Qt::EditRole).toInt() == 0) {
        emit signalSetItemStar(index, 1);
      } else {
        emit signalSetItemStar(index, 0);
      }
      event->ignore();
      return;
    } else if (index.column() == model_->fieldIndex("read")) {
      if (index.data(Qt::EditRole).toInt() == 0) {
        emit signalSetItemRead(index, 1);
      } else {
        emit signalSetItemRead(index, 0);
      }
      event->ignore();
      return;
    } else if (index.column() == model_->fieldIndex("label")) {
      if (QApplication::keyboardModifiers() == Qt::NoModifier) {
        emit signaNewslLabelClicked(index);
        event->ignore();
        return;
      }
    }
    if (QApplication::keyboardModifiers() == Qt::AltModifier) {
      emit signalMiddleClicked(index);
      event->ignore();
      return;
    }
  } else if ((event->buttons() & Qt::MiddleButton)) {
    emit signalMiddleClicked(index);
    return;
  } else {
    if (selectionModel()->selectedRows(0).count() > 1)
      return;
  }
  QTreeView::mousePressEvent(event);
}

/*virtual*/ void NewsView::mouseMoveEvent(QMouseEvent *event)
{
  QModelIndex index = indexAt(QEVENT_POS(event));
  if (index.isValid()) {
    emit signalHoverRowChanged(index.row());
  } else {
    emit signalHoverRowChanged(-1);
  }
}

/*virtual*/ void NewsView::scrollContentsBy(int dx, int dy)
{
  QTreeView::scrollContentsBy(dx, dy);

  if (dy == 0) return;
  if (!model()) return;

  // Collect rows that have been scrolled out ABOVE the visible area
  // (scrolling down). Rows below the viewport are still unread and must
  // NOT be marked read. Report source-model row numbers so the grouping
  // proxy row numbers never reach the mark-read controller.
  QList<int> scrolledOutRows;
  QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel*>(model());
  std::function<void(const QModelIndex&)> collect =
      [&](const QModelIndex &parent)
  {
    int count = model()->rowCount(parent);
    for (int row = 0; row < count; row++) {
      QModelIndex index = model()->index(row, 0, parent);
      if (!index.isValid()) continue;
      QRect rect = visualRect(index);
      if (rect.isValid() && rect.bottom() < 0) {
        if (proxy) {
          QModelIndex src = proxy->mapToSource(index);
          if (src.isValid())
            scrolledOutRows.append(src.row());
        } else {
          scrolledOutRows.append(index.row());
        }
      }
      if (model()->rowCount(index) > 0)
        collect(index);
    }
  };
  collect(QModelIndex());

  if (!scrolledOutRows.isEmpty())
    emit signalRowsScrolledOut(scrolledOutRows);
}

/*virtual*/ void NewsView::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (!indexAt(QEVENT_POS(event)).isValid()) return;

  if (indexClicked_ == indexAt(QEVENT_POS(event)))
    emit signalDoubleClicked(indexAt(QEVENT_POS(event)));
  else
    mousePressEvent(event);
}

/*virtual*/ void NewsView::keyPressEvent(QKeyEvent *event)
{
  QTreeView::keyPressEvent(event);
  switch (event->key()) {
  case Qt::Key_Up:
  case Qt::Key_K:                        // vim-style navigation
    emit pressKeyUp(currentIndex());
    break;
  case Qt::Key_Down:
  case Qt::Key_J:
    emit pressKeyDown(currentIndex());
    break;
  case Qt::Key_Home:
    emit pressKeyHome(currentIndex());
    break;
  case Qt::Key_End:
    emit pressKeyEnd(currentIndex());
    break;
  case Qt::Key_PageUp:
    emit pressKeyPageUp(currentIndex());
    break;
  case Qt::Key_PageDown:
    emit pressKeyPageDown(currentIndex());
    break;
  case Qt::Key_N:                        // next unread
    emit pressKeyNextUnread(currentIndex());
    break;
  case Qt::Key_P:                        // previous unread
    emit pressKeyPrevUnread(currentIndex());
    break;
  default:
    break;
  }
}

/*virtual*/ void NewsView::paintEvent(QPaintEvent *event)
{
  QTreeView::paintEvent(event);

  // UI-4: empty-state hint when the current filter has no news items
  if (model() && (model()->rowCount() == 0)) {
    QPainter painter(viewport());
    painter.setPen(palette().color(QPalette::Disabled, QPalette::Text));

    QFont hintFont = font();
    hintFont.setPointSize(qMax(hintFont.pointSize() - 1, 9));
    painter.setFont(hintFont);

    QRect area = viewport()->rect().adjusted(12, 12, -12, -12);
    painter.drawText(area, Qt::AlignCenter | Qt::TextWordWrap,
                     tr("No news here yet"));
  }
}

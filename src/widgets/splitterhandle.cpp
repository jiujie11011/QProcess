/* ============================================================
 * QProcess SplitterHandle - 实现（骨架版）
 * 版本：v1.4
 * ============================================================ */
#include "splitterhandle.h"
#include <QPainter>
#include <QStyleOption>
#include <QApplication>
#include <QDebug>

SplitterHandle::SplitterHandle(Qt::Orientation orientation, QSplitter* parent)
    : QSplitterHandle(orientation, parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    // 手柄宽度由 QSS 控制（QSplitter::handle 宽度），这里不强制
}

void SplitterHandle::setConstraints(int minPx, int maxPx)
{
    minPx_ = minPx;
    maxPx_ = maxPx;
}

void SplitterHandle::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor c = handleColor();
    if (orientation() == Qt::Horizontal) {
        // 垂直分割线（左右面板之间）
        int x = width() / 2;
        p.fillRect(x - 1, 0, 2, height(), c);
    } else {
        // 水平分割线（上下面板之间）
        int y = height() / 2;
        p.fillRect(0, y - 1, width(), 2, c);
    }
}

void SplitterHandle::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        pressed_ = true;
        dragStartPos_ = event->globalPos();
        dragStartSplitterPos_ = pos().x(); // 简化：仅处理水平分割
        setCursor(Qt::SplitHCursor);
        event->accept();
    } else {
        QSplitterHandle::mousePressEvent(event);
    }
}

void SplitterHandle::mouseMoveEvent(QMouseEvent* event)
{
    if (pressed_ && (event->buttons() & Qt::LeftButton)) {
        int delta = event->globalPos().x() - dragStartPos_.x();
        int newPos = dragStartSplitterPos_ + delta;

        // 应用约束
        if (newPos < minPx_) newPos = minPx_;
        if (newPos > maxPx_) newPos = maxPx_;

        // 移动分割线（通过 splitter 的 setSizes 间接实现）
        if (splitter()) {
            QList<int> sizes = splitter()->sizes();
            if (orientation() == Qt::Horizontal && sizes.size() >= 2) {
                // 简化：假设左侧面板索引 0，右侧索引 1
                int total = sizes[0] + sizes[1];
                sizes[0] = newPos;
                sizes[1] = total - newPos;
                splitter()->setSizes(sizes);
                emit dragged(newPos);
            }
        }
        event->accept();
    } else {
        QSplitterHandle::mouseMoveEvent(event);
    }
}

void SplitterHandle::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && pressed_) {
        pressed_ = false;
        unsetCursor();
        update();
        event->accept();
    } else {
        QSplitterHandle::mouseReleaseEvent(event);
    }
}

void SplitterHandle::hoverEnterEvent(QHoverEvent* event)
{
    hovered_ = true;
    update();
    QSplitterHandle::hoverEnterEvent(event);
}

void SplitterHandle::hoverLeaveEvent(QHoverEvent* event)
{
    hovered_ = false;
    update();
    QSplitterHandle::hoverLeaveEvent(event);
}

QColor SplitterHandle::handleColor() const
{
    // TODO: 从 ThemeManager 获取 token 色值
    // 当前用硬编码占位，编译通过后替换
    if (pressed_) return QColor("#4C8DFF");      // accent
    if (hovered_) return QColor("#3F3F46");      // borderDefault
    return Qt::transparent;
}

#include "moc_splitterhandle.cpp"
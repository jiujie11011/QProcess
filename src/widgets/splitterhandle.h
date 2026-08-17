/* ============================================================
 * QProcess SplitterHandle
 * Custom splitter handle: hover highlight, drag feedback, min/max width limits
 * Version: v1.4 (report section 13.1, 4.2, 4.3)
 * ============================================================ */
#ifndef SPLITTERHANDLE_H
#define SPLITTERHANDLE_H

#include <QSplitterHandle>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QEvent>
#include <QColor>

class SplitterHandle : public QSplitterHandle
{
    Q_OBJECT
public:
    enum class Orientation { Horizontal, Vertical };

    explicit SplitterHandle(Qt::Orientation orientation, QSplitter* parent = nullptr);
    ~SplitterHandle() override = default;

    // Set panel width constraints (pixels)
    void setConstraints(int minPx, int maxPx);

    // Set whether this is the right panel splitter (affects max width: 70vw)
    void setRightPanelHandle(bool isRight) { isRightPanel_ = isRight; }

signals:
    // Emitted live during drag (for external panel syncing)
    void dragged(int pos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    int minPx_ = 200;
    int maxPx_ = 10000; // no-limit default
    bool isRightPanel_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
    QPoint dragStartPos_;
    int dragStartSplitterPos_ = 0;

    QColor handleColor() const;
};

#endif // SPLITTERHANDLE_H
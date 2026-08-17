/* ============================================================
 * QProcess SplitterHandle
 * 自定义分割线手柄：hover 高亮、拖拽实时反馈、最小/最大宽度约束
 * 版本：v1.4（对应报告 §13.1, §4.2, §4.3）
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

    // 设置面板宽度约束（像素）
    void setConstraints(int minPx, int maxPx);

    // 设置是否为右侧面板的分割线（影响最大宽度计算：70vw）
    void setRightPanelHandle(bool isRight) { isRightPanel_ = isRight; }

signals:
    // 拖拽过程中实时发射（供外部同步其他面板）
    void dragged(int pos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    int minPx_ = 200;
    int maxPx_ = 10000; // 无上限默认
    bool isRightPanel_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
    QPoint dragStartPos_;
    int dragStartSplitterPos_ = 0;

    QColor handleColor() const;
};

#endif // SPLITTERHANDLE_H
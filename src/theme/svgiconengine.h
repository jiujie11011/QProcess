/* ============================================================
 * QProcess SvgIconEngine
 * 运行时按主题色渲染 Lucide SVG（stroke/recolor）
 * 版本：v1.4（对应报告 §5.2, §6.1, §13.4）
 * ============================================================ */
#ifndef SVGICONENGINE_H
#define SVGICONENGINE_H

#include <QIconEngine>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QColor>
#include <QHash>

class SvgIconEngine : public QIconEngine
{
public:
    // 图标状态
    enum class State { Normal, Hover, Pressed, Disabled, Active, ActiveHover };

    // 构造：SVG 路径 + 状态色映射
    // colorMap: 状态 -> 颜色（十六进制或 QColor 名称）
    // 若 colorMap 为空，使用 ThemeManager 当前 tokens 的默认映射
    explicit SvgIconEngine(const QString& svgPath,
                           const QHash<State, QColor>& colorMap = {});

    ~SvgIconEngine() override = default;

    // QIconEngine 接口
    QIconEngine* clone() const override;
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode,
               QIcon::State state) override;
    QPixmap pixmap(const QSize& size, QIcon::Mode mode,
                   QIcon::State state) override;

    // 运行时更新颜色映射（主题切换时调用）
    void setColorMap(const QHash<State, QColor>& colorMap);

    // 设置 SVG 内容（用于动态加载）
    void setSvgContent(const QString& svgContent);

    // 静态工厂：按 Lucide 图标名快速创建 QIcon
    // 内部维护缓存，避免重复解析 SVG
    static QIcon fromLucide(const QString& iconName,
                            SvgIconEngine::State state = State::Normal);

    // 预加载所有 Lucide 图标到内存（应用启动时调用）
    static void preloadAll(const QString& iconsDir);

private:
    QString svgPath_;
    mutable QString svgContent_;  // 解析后的 SVG XML（已替换 stroke/fill），mutable 因 const 函数内延迟加载
    QHash<State, QColor> colorMap_;
    // QSize 哈希：Qt5 无 qHash(QSize)，Qt6 无 QSize::operator<（QMap 不可用）。
    // 统一用 (w<<32)|h 组合键，跨 Qt5/Qt6 均可哈希。
    mutable QHash<qint64, QPixmap> pixmapCache_;

    // 将当前状态映射到颜色
    QColor colorForMode(QIcon::Mode mode, QIcon::State state) const;

    // 渲染 SVG 到 QPixmap（带颜色替换）
    QPixmap renderPixmap(const QSize& size, QIcon::Mode mode,
                         QIcon::State state) const;

    // 对 SVG 字符串做 stroke/fill 颜色替换
    static QString recolorSvg(const QString& svg, const QColor& strokeColor,
                              const QColor& fillColor = Qt::transparent);
};

// Qt5 无内建 enum class 哈希，需提供 qHash 重载（QHash<State, QColor> 依赖）
// Qt6 已有通用 enum qHash 模板，此重载在 Qt6 下不参与重载决议（更特化）
inline uint qHash(SvgIconEngine::State s, uint seed = 0) noexcept
{
    return qHash(static_cast<int>(s), seed);
}

#endif // SVGICONENGINE_H
/* ============================================================
 * QProcess SvgIconEngine
 * Renders Lucide SVG at runtime with theme colors (stroke/recolor)
 * Version: v1.4 (report section 5.2, 6.1, 13.4)
 * ============================================================ */
#ifndef SVGICONENGINE_H
#define SVGICONENGINE_H

#include "../common/qt6compat.h"

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
    // Icon states
    enum class State { Normal, Hover, Pressed, Disabled, Active, ActiveHover };

    // Constructor: SVG path + state color map
    // colorMap: state -> color (hex or QColor name)
    // If colorMap is empty, use ThemeManager current tokens default mapping
    explicit SvgIconEngine(const QString& svgPath,
                           const QHash<State, QColor>& colorMap = {});

    ~SvgIconEngine() override = default;

    // QIconEngine interface
    QIconEngine* clone() const override;
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode,
               QIcon::State state) override;
    QPixmap pixmap(const QSize& size, QIcon::Mode mode,
                   QIcon::State state) override;

    // Runtime update of color map (called on theme switch)
    void setColorMap(const QHash<State, QColor>& colorMap);

    // Set SVG content (for dynamic loading)
    void setSvgContent(const QString& svgContent);

    // Static factory: quickly create QIcon by Lucide icon name
    // Maintains an internal cache to avoid repeated SVG parsing
    static QIcon fromLucide(const QString& iconName,
                            SvgIconEngine::State state = State::Normal);

    // Preload all Lucide icons into memory (called at app startup)
    static void preloadAll(const QString& iconsDir);

private:
    QString svgPath_;
    mutable QString svgContent_;  // parsed SVG XML (stroke/fill replaced); mutable for lazy load in const methods
    QHash<State, QColor> colorMap_;
    // QSize hashing: Qt5 lacks qHash(QSize), Qt6 lacks QSize::operator< (QMap unusable).
    // Unified (w<<32)|h composite key works across Qt5/Qt6.
    mutable QHash<qint64, QPixmap> pixmapCache_;

    // Map current state to color
    QColor colorForMode(QIcon::Mode mode, QIcon::State state) const;

    // Render SVG to QPixmap (with color replacement)
    QPixmap renderPixmap(const QSize& size, QIcon::Mode mode,
                         QIcon::State state) const;

    // Replace stroke/fill colors in SVG string
    static QString recolorSvg(const QString& svg, const QColor& strokeColor,
                              const QColor& fillColor = Qt::transparent);
};

// Qt5 lacks a built-in enum class hash; provide a qHash overload (QHash<State, QColor> depends on it)
// Qt6 already has a generic enum qHash template; this overload does not participate in Qt6 overload resolution
#if !QUIL_QT6
inline uint qHash(SvgIconEngine::State s, uint seed = 0) noexcept
{
    return qHash(static_cast<int>(s), seed);
}
#endif

#endif // SVGICONENGINE_H
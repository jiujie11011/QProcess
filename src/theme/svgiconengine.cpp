/* ============================================================
 * QProcess SvgIconEngine - 实现（骨架版）
 * 版本：v1.4
 * ============================================================ */
#include "svgiconengine.h"
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QApplication>
#include <QRegularExpression>
#include <QStringList>

static QHash<QString, QString> s_svgCache;
static QMutex s_cacheMutex;

SvgIconEngine::SvgIconEngine(const QString& svgPath,
                             const QHash<State, QColor>& colorMap)
    : svgPath_(svgPath)
    , colorMap_(colorMap)
{
    // 延迟加载：首次 paint/pixmap 时读取
}

// 析构在头文件声明为 = default（MSVC 不允许重复定义）

QIconEngine* SvgIconEngine::clone() const
{
    auto* copy = new SvgIconEngine(svgPath_, colorMap_);
    copy->svgContent_ = svgContent_;
    return copy;
}

void SvgIconEngine::setColorMap(const QHash<State, QColor>& colorMap)
{
    colorMap_ = colorMap;
    pixmapCache_.clear(); // 颜色变了，缓存失效
}

void SvgIconEngine::setSvgContent(const QString& svgContent)
{
    svgContent_ = svgContent;
    pixmapCache_.clear();
}

void SvgIconEngine::paint(QPainter* painter, const QRect& rect,
                          QIcon::Mode mode, QIcon::State state)
{
    QPixmap pm = pixmap(rect.size(), mode, state);
    painter->drawPixmap(rect, pm);
}

QPixmap SvgIconEngine::pixmap(const QSize& size, QIcon::Mode mode,
                              QIcon::State state)
{
    // Cache key: size occupies (width << 32) | height; the resolved color is
    // mixed in with a golden-ratio hash so different colors never alias the
    // same entry (works identically on Qt5/Qt6, see pixmapCache_ comment).
    const QColor color = colorForMode(mode, state);
    qint64 cacheKey = (qint64(quint32(size.width())) << 32) | quint32(size.height());
    cacheKey ^= qint64(quint32(color.rgba())) * Q_INT64_C(0x9E3779B97F4A7C15);

    QPixmap cached = pixmapCache_.value(cacheKey);
    if (!cached.isNull())
        return cached;

    cached = renderPixmap(size, mode, state);
    pixmapCache_.insert(cacheKey, cached);
    return cached;
}

QColor SvgIconEngine::colorForMode(QIcon::Mode mode, QIcon::State state) const
{
    // 默认映射（若 colorMap_ 为空）
    static const QHash<State, QColor> defaultMap = [] {
        QHash<State, QColor> m;
        m[State::Normal]      = QColor("#A0A0AA"); // textSecondary
        m[State::Hover]       = QColor("#E8E8EA"); // textPrimary
        m[State::Pressed]     = QColor("#4C8DFF"); // accent
        m[State::Disabled]    = QColor("#5C5C64"); // textDisabled
        m[State::Active]      = QColor("#4C8DFF"); // accent
        m[State::ActiveHover] = QColor("#6BA1FF"); // accentHover
        return m;
    }();

    if (colorMap_.isEmpty()) {
        // 根据 mode/state 映射到 State 枚举
        SvgIconEngine::State s = State::Normal;
        if (mode == QIcon::Disabled) s = State::Disabled;
        else if (mode == QIcon::Active) {
            if (state == QIcon::On) s = State::Active;
            else s = State::Normal;
        } else if (mode == QIcon::Selected) {
            if (state == QIcon::On) s = State::Active;
            else s = State::Hover;
        } else { // Normal
            s = State::Normal;
        }
        return defaultMap.value(s, QColor("#A0A0AA"));
    }

    // 使用自定义映射（简化：仅按 mode 区分）
    if (mode == QIcon::Disabled) return colorMap_.value(State::Disabled);
    if (mode == QIcon::Active) return colorMap_.value(State::Active);
    if (mode == QIcon::Selected) return colorMap_.value(State::Active);
    return colorMap_.value(State::Normal);
}

QPixmap SvgIconEngine::renderPixmap(const QSize& size, QIcon::Mode mode,
                                    QIcon::State state) const
{
    // 确保 SVG 内容已加载
    if (svgContent_.isEmpty() && !svgPath_.isEmpty()) {
        QFile f(svgPath_);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            svgContent_ = QString::fromUtf8(f.readAll());
            f.close();
        }
    }

    QColor strokeColor = colorForMode(mode, state);
    QString coloredSvg = recolorSvg(svgContent_, strokeColor);

    // 使用 QSvgRenderer 渲染
    QSvgRenderer renderer(coloredSvg.toUtf8());
    QPixmap pm(size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    renderer.render(&p, QRectF(QPointF(0,0), size));
    p.end();

    return pm;
}

QString SvgIconEngine::recolorSvg(const QString& svg, const QColor& strokeColor,
                                  const QColor& fillColor)
{
    // Lucide SVG 特征：stroke="currentColor" 或 stroke="#xxx" fill="none"
    // 我们统一替换 stroke 属性为目标颜色，fill 保持 none 或替换为目标色
    QString result = svg;

    // 替换 stroke="currentColor" -> stroke="#RRGGBB"
    QString strokeHex = strokeColor.name(QColor::HexRgb);
    result.replace("stroke=\"currentColor\"", QString("stroke=\"%1\"").arg(strokeHex));
    result.replace("stroke='currentColor'", QString("stroke='%1'").arg(strokeHex));

    // 替换已有的 stroke="#xxxxxx" 或 stroke="rgb(...)"
    QRegularExpression strokeRe(R"(stroke\s*=\s*["'][^"']*["'])");
    result.replace(strokeRe, QString("stroke=\"%1\"").arg(strokeHex));

    // fill="none" 保持不变，如果有 fill="currentColor" 或具体颜色则替换
    if (fillColor != Qt::transparent) {
        QString fillHex = fillColor.name(QColor::HexRgb);
        result.replace("fill=\"currentColor\"", QString("fill=\"%1\"").arg(fillHex));
        result.replace("fill='currentColor'", QString("fill='%1'").arg(fillHex));
        // 注意：QString::replace(QRegularExpression, lambda) 是 Qt6-only 重载，
        // Qt5 需手动遍历替换（仅替换非 none 的 fill）。
        QRegularExpression fillRe(R"(fill\s*=\s*["'][^"']*["'])");
        QString out;
        QRegularExpressionMatchIterator it = fillRe.globalMatch(result);
        int lastPos = 0;
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += result.mid(lastPos, m.capturedStart() - lastPos);
            const QString matched = m.captured(0);
            if (matched.contains("none", Qt::CaseInsensitive))
                out += matched;
            else
                out += QString("fill=\"%1\"").arg(fillHex);
            lastPos = m.capturedEnd();
        }
        out += result.mid(lastPos);
        result = out;
    }

    return result;
}

QIcon SvgIconEngine::fromLucide(const QString& iconName, State state)
{
    // 从缓存或磁盘加载 SVG，创建 QIcon
    QString svgContent;

    s_cacheMutex.lock();
    if (s_svgCache.contains(iconName)) {
        svgContent = s_svgCache[iconName];
    }
    s_cacheMutex.unlock();

    if (svgContent.isEmpty()) {
        // 从资源目录加载
        QString path = QString(":/icons/%1.svg").arg(iconName);
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            svgContent = QString::fromUtf8(f.readAll());
            f.close();
            s_cacheMutex.lock();
            s_svgCache[iconName] = svgContent;
            s_cacheMutex.unlock();
        } else {
            qWarning() << "[SvgIconEngine] 图标未找到:" << path;
            return QIcon(); // 空图标
        }
    }

    auto* engine = new SvgIconEngine(QString(), QHash<State, QColor>());
    engine->setSvgContent(svgContent);
    // 可根据 state 预设色映射
    return QIcon(engine);
}

void SvgIconEngine::preloadAll(const QString& iconsDir)
{
    QDir dir(iconsDir);
    if (!dir.exists()) {
        qWarning() << "[SvgIconEngine] 图标目录不存在:" << iconsDir;
        return;
    }

    QStringList filters;
    filters << "*.svg";
    QStringList files = dir.entryList(filters, QDir::Files);

    for (const QString& file : files) {
        QString name = file;
        name.chop(4); // 去掉 .svg
        QString path = dir.absoluteFilePath(file);
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = f.readAll();
            f.close();
            s_cacheMutex.lock();
            s_svgCache[name] = content;
            s_cacheMutex.unlock();
        }
    }
    qDebug() << "[SvgIconEngine] 预加载完成，共" << s_svgCache.size() << "个图标";
}


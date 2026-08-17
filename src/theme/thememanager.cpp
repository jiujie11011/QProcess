/* ============================================================
 * QProcess ThemeManager - 完整实现
 * 版本：v1.4（对应报告 §4.1 Phase 0/0.5, §6.1）
 * ============================================================ */
#include "thememanager.h"
#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QDebug>
#include <QTimer>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace {
    QString mixColor(const QString& hexColor, const QString& bgColor, qreal opacity) {
        // 简单混合：返回带透明度的 rgba 字符串
        // hexColor: #RRGGBB, bgColor: #RRGGBB, opacity: 0.0-1.0
        if (hexColor.startsWith('#') && hexColor.length() == 7) {
            int r = hexColor.mid(1, 2).toInt(nullptr, 16);
            int g = hexColor.mid(3, 2).toInt(nullptr, 16);
            int b = hexColor.mid(5, 2).toInt(nullptr, 16);
            return QString("rgba(%1, %2, %3, %4)").arg(r).arg(g).arg(b).arg(opacity);
        }
        return hexColor;
    }
}

ThemeManager* ThemeManager::instance()
{
    static ThemeManager* inst = nullptr;
    if (!inst) {
        inst = new ThemeManager(qApp);
    }
    return inst;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
    , currentTokens_(TOK_LIGHT)
{
    QSettings settings;
    int saved = settings.value("appearance/themeMode", static_cast<int>(Type::System)).toInt();
    current_ = static_cast<Type>(saved);

    if (current_ == Type::System) {
        currentTokens_ = resolveSystemTheme() == Type::Dark ? TOK_DARK : TOK_LIGHT;
    } else {
        currentTokens_ = current_ == Type::Dark ? TOK_DARK : TOK_LIGHT;
    }

    qApp->installNativeEventFilter(this);
}

const ThemeTokens& ThemeManager::tokens() const
{
    return currentTokens_;
}

void ThemeManager::apply(Type type)
{
    Type targetType = type;

    if (type == Type::System) {
        targetType = resolveSystemTheme();
    }

    current_ = type;
    currentTokens_ = (targetType == Type::Dark) ? TOK_DARK : TOK_LIGHT;

    QString qssPath;
    if (targetType == Type::Dark) {
        qssPath = ":/style/codex_dark.qss";
        if (!QFile::exists(qssPath)) {
            qssPath = "style/codex_dark.qss";
        }
    } else {
        qssPath = ":/style/codex_light.qss";
        if (!QFile::exists(qssPath)) {
            qssPath = "style/codex_light.qss";
        }
    }

    QString qss = renderQss(qssPath);
    if (!qss.isEmpty()) {
        qApp->setStyleSheet(qss);
        lastRenderedQss_ = qss;
    }

    QSettings settings;
    settings.setValue("appearance/themeMode", static_cast<int>(type));

    emit themeChanged(current_);
    qDebug() << "[ThemeManager] Applied theme:" << (targetType == Type::Dark ? "Dark" : "Light");
}

void ThemeManager::refresh()
{
    apply(current_);
}

ThemeManager::Type ThemeManager::resolveSystemTheme() const
{
#if defined(Q_OS_WIN)
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    int appsUseLight = reg.value("AppsUseLightTheme", 1).toInt();
    return appsUseLight ? Type::Light : Type::Dark;
#else
    return Type::Light;
#endif
}

#if defined(QT6)
bool ThemeManager::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
#else
bool ThemeManager::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
#endif
{
#if defined(Q_OS_WIN)
    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_SETTINGCHANGE) {
        if (current_ == Type::System) {
            QTimer::singleShot(100, this, [this] {
                Type sys = resolveSystemTheme();
                Type currentEffective = (currentTokens_.bgApp == TOK_DARK.bgApp) ? Type::Dark : Type::Light;
                if (sys != currentEffective) {
                    refresh();
                }
            });
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
#endif
    return QAbstractNativeEventFilter::nativeEventFilter(eventType, message, result);
}

QString ThemeManager::renderQss(const QString& templatePath) const
{
    QFile f(templatePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ThemeManager] Cannot open QSS template:" << templatePath;
        return QString();
    }
    QString tpl = f.readAll();
    f.close();

    const ThemeTokens& tk = currentTokens_;

    tpl.replace("%BG_APP%", tk.bgApp)
       .replace("%BG_SURFACE%", tk.bgSurface)
       .replace("%BG_SURFACE_ALT%", tk.bgSurfaceAlt)
       .replace("%BG_HOVER%", tk.bgHover)
       .replace("%BG_SELECTED%", tk.bgSelected)
       .replace("%BORDER_SUBTLE%", tk.borderSubtle)
       .replace("%BORDER_DEFAULT%", tk.borderDefault)
       .replace("%TEXT_PRIMARY%", tk.textPrimary)
       .replace("%TEXT_SECONDARY%", tk.textSecondary)
       .replace("%TEXT_TERTIARY%", tk.textTertiary)
       .replace("%TEXT_DISABLED%", tk.textDisabled)
       .replace("%TEXT_ON_ACCENT%", tk.textOnAccent)
       .replace("%ACCENT%", tk.accent)
       .replace("%ACCENT_HOVER%", tk.accentHover)
       .replace("%STATUS_UNREAD%", tk.statusUnread)
       .replace("%STATUS_STARRED%", tk.statusStarred)
       .replace("%STATUS_ERROR%", tk.statusError)
       .replace("%STATUS_SUCCESS%", tk.statusSuccess)
       .replace("%PLAYER_BAR_BG%", tk.playerBarBg)
       .replace("%RADIUS_SM%", QString::number(tk.radiusSm))
       .replace("%RADIUS_MD%", QString::number(tk.radiusMd))
       .replace("%RADIUS_LG%", QString::number(tk.radiusLg))
       .replace("%RADIUS_FULL%", QString::number(tk.radiusFull))
       .replace("%SHADOW_CARD%", tk.shadowCard)
       .replace("%SHADOW_OVERLAY%", tk.shadowOverlay);

    // 计算 accentSoft 混合色
    QString accentSoft = mixColor(tk.accent, tk.bgSurface, 0.12);
    tpl.replace("%ACCENT_SOFT%", accentSoft);

    QString accentSoftActive = mixColor(tk.accent, tk.bgSelected, 0.20);
    tpl.replace("%ACCENT_SOFT_ACTIVE%", accentSoftActive);

    // 字号档位替换（按 fontSizeCombo 选择）
    QSettings settings;
    int fontSizeIdx = settings.value("appearance/fontSize", 1).toInt();
    int baseFontSize = tk.fontSizeBase;
    int titleFontSize = tk.fontSizeTitle;
    int captionFontSize = tk.fontSizeCaption;
    int readerFontSize = tk.readerFontSize;

    if (fontSizeIdx == 0) { // Small/Compact
        baseFontSize -= 1;
        titleFontSize -= 1;
        captionFontSize -= 1;
        readerFontSize -= 2;
    } else if (fontSizeIdx == 2) { // Large
        baseFontSize += 1;
        titleFontSize += 1;
        captionFontSize += 1;
        readerFontSize += 2;
    }

    tpl.replace("%FONT_SIZE_BASE%", QString::number(baseFontSize))
       .replace("%FONT_SIZE_TITLE%", QString::number(titleFontSize))
       .replace("%FONT_SIZE_CAPTION%", QString::number(captionFontSize))
       .replace("%READER_FONT_SIZE%", QString::number(readerFontSize));

    // 列表密度替换
    int densityIdx = settings.value("appearance/listDensity", 1).toInt();
    int listItemHeight = (densityIdx == 0) ? tk.listItemHeightCompact : tk.listItemHeight;
    tpl.replace("%LIST_ITEM_HEIGHT%", QString::number(listItemHeight));

    // 动画时长（考虑 reduceMotion）
    bool reduceMotion = settings.value("interface/reduceMotion", false).toBool();
    int motionFast = reduceMotion ? 0 : tk.motionFast;
    int motionBase = reduceMotion ? 0 : tk.motionBase;
    int motionSlow = reduceMotion ? 0 : tk.motionSlow;
    tpl.replace("%MOTION_FAST%", QString::number(motionFast))
       .replace("%MOTION_BASE%", QString::number(motionBase))
       .replace("%MOTION_SLOW%", QString::number(motionSlow));

    return tpl;
}


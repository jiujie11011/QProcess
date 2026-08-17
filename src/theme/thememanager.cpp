/* ============================================================
 * QProcess ThemeManager - 实现（骨架版，空实现）
 * 版本：v1.4
 * 说明：编译修复期间提供声明与空实现，不接线 mainwindow.cpp
 * 编译通过后再填充 renderQss/apply/refresh/system 跟随逻辑
 * ============================================================ */
#include "thememanager.h"
#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QDebug>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

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
{
    // 初始化：读取保存的主题设置，默认 System
    QSettings settings;
    int saved = settings.value("appearance/themeMode", static_cast<int>(Type::System)).toInt();
    current_ = static_cast<Type>(saved);
    // 不立即 apply，等主窗口构造完成后由 MainWindow 显式调用
}

const ThemeTokens& ThemeManager::tokens() const
{
    return currentTokens_;
}

void ThemeManager::apply(Type type)
{
    // TODO: 实现完整逻辑
    // 1. current_ = type
    // 2. 根据 type 选择 TOK_DARK / TOK_LIGHT / resolveSystemTheme()
    // 3. currentTokens_ = 对应 tokens
    // 4. 渲染两套 QSS（codex_dark.qss / codex_light.qss）
    // 5. qApp->setStyleSheet(渲染后的字符串)
    // 6. 保存设置
    // 7. emit themeChanged(current_)

    Q_UNUSED(type);
    qDebug() << "[ThemeManager] apply() called - skeleton implementation";
}

void ThemeManager::refresh()
{
    // TODO: 重新渲染当前主题的 QSS 并应用
    apply(current_);
}

ThemeManager::Type ThemeManager::resolveSystemTheme() const
{
#if defined(Q_OS_WIN)
    // Windows: 读注册表 HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    int appsUseLight = reg.value("AppsUseLightTheme", 1).toInt();
    return appsUseLight ? Type::Light : Type::Dark;
#else
    // Linux/macOS: 简化处理，默认 Light（可后续扩展检测 GTK/Qt 主题）
    return Type::Light;
#endif
}

bool ThemeManager::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
{
#if defined(Q_OS_WIN)
    // 监听 WM_SETTINGCHANGE (0x001A)，wParam = SPI_SETNONCLIENTMETRICS 或 "ImmersiveColorSet"
    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_SETTINGCHANGE) {
        // 仅在 System 模式下响应
        if (current_ == Type::System) {
            // 延迟一点再检测，避免注册表尚未更新
            QTimer::singleShot(100, this, [this] {
                Type sys = resolveSystemTheme();
                if (sys != current_) { // 实际上 current_ 还是 System，但 tokens 变了
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
    return QObject::nativeEventFilter(eventType, message, result);
}

QString ThemeManager::renderQss(const QString& templatePath) const
{
    // TODO: 实现完整占位符替换
    // 读取文件 -> 逐个 %TOKEN% 替换为 currentTokens_ 对应值
    // 特别处理：accentSoft/accentSoftActive 需要按比例混合（当前 tokens 存的是纯 accent 色，
    // ThemeManager 计算混合后的十六进制字符串再替换）
    QFile f(templatePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ThemeManager] 无法打开 QSS 模板:" << templatePath;
        return QString();
    }
    QString tpl = f.readAll();
    f.close();

    // 占位符映射表（示例，实际需全量覆盖 tokens.h 所有字段）
    // tpl.replace("%BG_APP%", currentTokens_.bgApp).replace(...)...

    Q_UNUSED(tpl);
    return QString(); // 骨架返回空，编译通过后填充
}

#include "moc_thememanager.cpp"
/* ============================================================
 * Quill - unit tests for ThemeManager
 *
 * 说明：
 *  - ThemeManager 是 QApplication 级单例（构造时 installNativeEventFilter），
 *    因此本套件要求 QApplication 环境（由 tests/main.cpp 提供）。
 *  - QSS 模板在测试二进制中不存在（未编译资源），renderQss 静默返回空，
 *    测试不依赖文件系统。
 * ============================================================ */
#include <QtTest>
#include <QSettings>

#include "tst_thememanager.h"
#include "thememanager.h"
#include "tokens.h"

void TestThemeManager::initTestCase()
{
    // 隔离 QSettings，避免污染真实用户配置
    QCoreApplication::setOrganizationName(QStringLiteral("QuillTest"));
    QCoreApplication::setApplicationName(QStringLiteral("ThemeManagerTest"));
}

void TestThemeManager::defaultTypeIsSystem()
{
    ThemeManager* tm = ThemeManager::instance();
    QVERIFY(tm != nullptr);
    QCOMPARE(int(tm->currentType()), int(ThemeManager::Type::System));
}

void TestThemeManager::darkAndLightTokensDiffer()
{
    ThemeManager* tm = ThemeManager::instance();

    tm->apply(ThemeManager::Type::Dark);
    const ThemeTokens dark = tm->tokens();
    QCOMPARE(dark.bgApp, TOK_DARK.bgApp);
    QCOMPARE(dark.accent, TOK_DARK.accent);

    tm->apply(ThemeManager::Type::Light);
    const ThemeTokens light = tm->tokens();
    QCOMPARE(light.bgApp, TOK_LIGHT.bgApp);

    // 明暗两套色板必须有实质差异，否则主题切换是空转
    QVERIFY(dark.bgApp != light.bgApp);
    QVERIFY(dark.textPrimary != light.textPrimary);
    QVERIFY(dark.accent != light.accent);
}

void TestThemeManager::applyPersistsToSettings()
{
    ThemeManager* tm = ThemeManager::instance();

    tm->apply(ThemeManager::Type::Dark);
    QCOMPARE(int(tm->currentType()), int(ThemeManager::Type::Dark));

    QSettings settings;
    QCOMPARE(settings.value("appearance/themeMode",
                            int(ThemeManager::Type::System)).toInt(),
             int(ThemeManager::Type::Dark));
}

void TestThemeManager::refreshKeepsTokens()
{
    ThemeManager* tm = ThemeManager::instance();

    tm->apply(ThemeManager::Type::Dark);
    const QString bgBefore = tm->tokens().bgApp;

    tm->refresh();
    QCOMPARE(tm->tokens().bgApp, bgBefore);
    QCOMPARE(int(tm->currentType()), int(ThemeManager::Type::Dark));
}

void TestThemeManager::systemModeResolvesSafely()
{
    ThemeManager* tm = ThemeManager::instance();

    // System 模式在测试环境（非 Windows）解析为 Light，不应崩溃且色板有效
    tm->apply(ThemeManager::Type::System);
    QCOMPARE(int(tm->currentType()), int(ThemeManager::Type::System));
    QVERIFY(!tm->tokens().bgApp.isEmpty());
}

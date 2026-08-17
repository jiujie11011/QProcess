/* ============================================================
 * Quill - unit tests unified entry point
 *
 * 依次执行三套测试（单 target，CI 步骤无需改动）：
 *   1. TestCommon          - HtmlSanitizer / FtsSearch（纯逻辑）
 *   2. TestThemeManager    - 主题切换/tokens/QSettings（需 QApplication）
 *   3. TestNewsCardDelegate- 卡片数据序列化/sizeHint（需 QApplication）
 *
 * Linux 无头 CI 下自动启用 offscreen 平台插件。
 * ============================================================ */
#include <QApplication>
#include <QtTest>

#include "tst_common.h"
#include "tst_thememanager.h"
#include "tst_newscarddelegate.h"

int main(int argc, char* argv[])
{
#if defined(Q_OS_LINUX)
    // CI 无显示服务器：QTest + widgets 需要 offscreen 平台插件
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
#endif

    QApplication app(argc, argv);

    int status = 0;
    {
        TestCommon tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestThemeManager tm;
        status |= QTest::qExec(&tm, argc, argv);
    }
    {
        TestNewsCardDelegate nd;
        status |= QTest::qExec(&nd, argc, argv);
    }
    return status;
}

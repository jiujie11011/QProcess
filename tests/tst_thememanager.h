/* ============================================================
 * Quill - unit tests for ThemeManager
 * 覆盖：单例、明暗 tokens 切换、QSettings 持久化、refresh 稳定性
 * ============================================================ */
#ifndef TST_THEMEMANAGER_H
#define TST_THEMEMANAGER_H

#include <QObject>

class TestThemeManager : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase();
  void defaultTypeIsSystem();
  void darkAndLightTokensDiffer();
  void applyPersistsToSettings();
  void refreshKeepsTokens();
  void systemModeResolvesSafely();
};

#endif // TST_THEMEMANAGER_H

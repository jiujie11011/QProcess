/* ============================================================
 * Quill - unit tests for NewsCardDelegate
 * 覆盖：ArticleData QVariant 往返序列化（含 feedId 回归点）、
 *       视觉层级访问器、sizeHint 合理性
 * ============================================================ */
#ifndef TST_NEWSCARDDELEGATE_H
#define TST_NEWSCARDDELEGATE_H

#include <QObject>

class TestNewsCardDelegate : public QObject
{
  Q_OBJECT
private slots:
  void roundTripPreservesAllFields();
  void feedIdRoundTrip();
  void emptyDataDefaults();
  void visualLevelAccessors();
  void sizeHintNeverNegative();
};

#endif // TST_NEWSCARDDELEGATE_H

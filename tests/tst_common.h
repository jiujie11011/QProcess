/* ============================================================
 * Quill - unit tests for the pure-logic helper modules
 * 类声明（实现见 tst_common.cpp）
 * ============================================================ */
#ifndef TST_COMMON_H
#define TST_COMMON_H

#include <QObject>

class TestCommon : public QObject
{
  Q_OBJECT
private slots:
  // HtmlSanitizer
  void stripsScriptBlocks();
  void stripsStyleBlocks();
  void stripsEventHandlers();
  void stripsDangerousSchemes();
  void stripsIframeObjectEmbedLinkBase();
  void stripsMetaRefresh();
  void preservesContent();
  void preservesFileAndDataUrls();
  void handlesEmptyInput();

  // FtsSearch
  void asciiDetection();
  void matchTermEscaping();
  void matchTermEmpty();
};

#endif // TST_COMMON_H

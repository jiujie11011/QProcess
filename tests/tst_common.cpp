/* ============================================================
* Quill - unit tests for the pure-logic helper modules
*
* Covers:
*  - HtmlSanitizer: script/style/event-handler/dangerous-URL removal
*  - FtsSearch: ASCII detection and MATCH-term escaping
*
* Built as a standalone QTest target (tests/tests.pro) so the CI
* loop can run "build + test" without needing the full application.
* ============================================================ */
#include <QtTest>

#include "htmlsanitizer.h"
#include "ftssearch.h"

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

// ----------------------------------------------------------------- HtmlSanitizer
void TestCommon::stripsScriptBlocks()
{
  QCOMPARE(HtmlSanitizer::sanitize(QString("<p>hi</p><script>alert(1)</script>")),
           QString("<p>hi</p>"));

  // Case-insensitive, attributes, multiline body.
  QCOMPARE(HtmlSanitizer::sanitize(
             QString("<SCRIPT type=\"text/javascript\">\nvar x = 1;\n</SCRIPT>x")),
           QString("x"));
}

void TestCommon::stripsStyleBlocks()
{
  QCOMPARE(HtmlSanitizer::sanitize(
             QString("<div><style>body{display:none}</style>ok</div>")),
           QString("<div>ok</div>"));
}

void TestCommon::stripsEventHandlers()
{
  QCOMPARE(HtmlSanitizer::sanitize(QString("<img src=\"a.png\" onerror=\"alert(1)\">")),
           QString("<img src=\"a.png\">"));
  QCOMPARE(HtmlSanitizer::sanitize(QString("<a href='#' onclick='evil()'>x</a>")),
           QString("<a href='#'>x</a>"));
  QCOMPARE(HtmlSanitizer::sanitize(QString("<img src=\"a.png\" onload=evil()>")),
           QString("<img src=\"a.png\">"));
}

void TestCommon::stripsDangerousSchemes()
{
  QCOMPARE(HtmlSanitizer::sanitize(QString("<a href=\"javascript:alert(1)\">x</a>")),
           QString("<a href=\"#\">x</a>"));
  QCOMPARE(HtmlSanitizer::sanitize(QString("<img src='vbscript:msgbox(1)'>")),
           QString("<img src='#'>"));
  QCOMPARE(HtmlSanitizer::sanitize(QString("<a href=\"JaVaScRiPt:alert(1)\">x</a>")),
           QString("<a href=\"#\">x</a>"));
}

void TestCommon::stripsIframeObjectEmbedLinkBase()
{
  QCOMPARE(HtmlSanitizer::sanitize(QString("<iframe src=\"x\"></iframe>ok")),
           QString("ok"));
  QCOMPARE(HtmlSanitizer::sanitize(QString("<object data=\"x\"></object>ok")),
           QString("ok"));
  QCOMPARE(HtmlSanitizer::sanitize(QString("<embed src=\"x\">ok")),
           QString("ok"));
  QCOMPARE(HtmlSanitizer::sanitize(QString("<link rel=\"stylesheet\" href=\"x.css\">ok")),
           QString("ok"));
  QCOMPARE(HtmlSanitizer::sanitize(QString("<base href=\"http://evil/\">ok")),
           QString("ok"));
}

void TestCommon::stripsMetaRefresh()
{
  QCOMPARE(HtmlSanitizer::sanitize(
             QString("<meta http-equiv=\"refresh\" content=\"0;url=http://evil/\">ok")),
           QString("ok"));
  QCOMPARE(HtmlSanitizer::sanitize(
             QString("<meta http-equiv='refresh' content='5'>ok")),
           QString("ok"));
}

void TestCommon::preservesContent()
{
  const QString in = QString("<p>Hello <b>world</b></p><img src=\"http://x/a.jpg\">"
                             "<a href=\"https://example.com/\">link</a>");
  QCOMPARE(HtmlSanitizer::sanitize(in), in);
}

void TestCommon::preservesFileAndDataUrls()
{
  const QString file = QString("<img src=\"file:///C:/x/a.jpg\">");
  QCOMPARE(HtmlSanitizer::sanitize(file), file);

  const QString data = QString("<img src=\"data:image/png;base64,AAAA\">");
  QCOMPARE(HtmlSanitizer::sanitize(data), data);
}

void TestCommon::handlesEmptyInput()
{
  QCOMPARE(HtmlSanitizer::sanitize(QString()), QString());
  QCOMPARE(HtmlSanitizer::sanitize(QString("plain text")),
           QString("plain text"));
}

// ------------------------------------------------------------------ FtsSearch
void TestCommon::asciiDetection()
{
  QVERIFY(FtsSearch::isAsciiOnly(QString("hello world")));
  QVERIFY(FtsSearch::isAsciiOnly(QString("Qt-5.15 C++")));
  QVERIFY(!FtsSearch::isAsciiOnly(QString("新闻")));
  QVERIFY(!FtsSearch::isAsciiOnly(QString("hello 世界")));
}

void TestCommon::matchTermEscaping()
{
  QCOMPARE(FtsSearch::matchTerm(QString("hello")), QString("\"hello\""));
  // Embedded quotes are doubled per the FTS5 phrase grammar.
  QCOMPARE(FtsSearch::matchTerm(QString("say \"hi\"")),
           QString("\"say \"\"hi\"\"\""));
  // Apostrophes are literal inside a phrase and safe for SQL embedding
  // (the caller already escaped single quotes for the SQL layer).
  QCOMPARE(FtsSearch::matchTerm(QString("it's")), QString("\"it's\""));
}

void TestCommon::matchTermEmpty()
{
  QCOMPARE(FtsSearch::matchTerm(QString("   ")), QString("*"));
  QCOMPARE(FtsSearch::matchTerm(QString()), QString("*"));
}

QTEST_APPLESS_MAIN(TestCommon)
#include "tst_common.moc"

/* ============================================================
* Quill is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 Quill Team <quillteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "xpathfeedparser.h"

#include <QWebEnginePage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

XPathFeedParser::XPathFeedParser(QObject *parent)
  : QObject(parent)
  , page_(new QWebEnginePage(this))
{
  page_->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
}

XPathFeedParser::~XPathFeedParser()
{
  delete page_;
}

static QString jsString(const QString &s)
{
  return QJsonDocument(QJsonArray() << s).toJson(QJsonDocument::Compact).
      trimmed().mid(1).chopped(1);
}

QString XPathFeedParser::buildScript() const
{
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(fetchRule_.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    return QString();

  QJsonObject rule = doc.object();
  QString itemExpr = rule.value("item").toString();
  QString titleExpr = rule.value("title").toString();
  QString linkExpr = rule.value("link").toString();
  QString descExpr = rule.value("description").toString();
  QString dateExpr = rule.value("date").toString();
  QString authorExpr = rule.value("author").toString();

  if (itemExpr.isEmpty())
    return QString();

  return QString(
    "(function() {"
    "  var itemExpr = %1;"
    "  var titleExpr = %2;"
    "  var linkExpr = %3;"
    "  var descExpr = %4;"
    "  var dateExpr = %5;"
    "  var authorExpr = %6;"
    "  var result = [];"
    "  var r = document.evaluate(itemExpr, document, null, "
    "    XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);"
    "  for (var i = 0; i < r.snapshotLength; i++) {"
    "    var ctx = r.snapshotItem(i);"
    "    var item = {};"
    "    if (titleExpr) {"
    "      var n = document.evaluate(titleExpr, ctx, null, "
    "        XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;"
    "      item.title = n ? n.textContent.trim() : '';"
    "    }"
    "    if (linkExpr) {"
    "      var n = document.evaluate(linkExpr, ctx, null, "
    "        XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;"
    "      item.link = n ? (n.href ? n.href : n.textContent.trim()) : '';"
    "    }"
    "    if (descExpr) {"
    "      var n = document.evaluate(descExpr, ctx, null, "
    "        XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;"
    "      item.description = n ? n.textContent.trim() : '';"
    "    }"
    "    if (dateExpr) {"
    "      var n = document.evaluate(dateExpr, ctx, null, "
    "        XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;"
    "      item.date = n ? (n.getAttribute ? "
    "        (n.getAttribute('datetime') || n.getAttribute('pubdate') || n.textContent.trim()) "
    "        : n.textContent.trim()) : '';"
    "    }"
    "    if (authorExpr) {"
    "      var n = document.evaluate(authorExpr, ctx, null, "
    "        XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue;"
    "      item.author = n ? n.textContent.trim() : '';"
    "    }"
    "    if (item.title || item.link) result.push(item);"
    "  }"
    "  return JSON.stringify(result);"
    "})();"
  ).arg(jsString(itemExpr), jsString(titleExpr), jsString(linkExpr),
        jsString(descExpr), jsString(dateExpr), jsString(authorExpr));
}

QList<XPathNewsItem> XPathFeedParser::decodeResult(const QVariant &result)
{
  QList<XPathNewsItem> items;
  QJsonDocument doc = QJsonDocument::fromJson(result.toString().toUtf8());
  QJsonArray arr = doc.array();
  for (int i = 0; i < arr.count(); ++i) {
    QJsonObject obj = arr.at(i).toObject();
    XPathNewsItem item;
    item.title = obj.value("title").toString().trimmed();
    item.link = obj.value("link").toString().trimmed();
    item.description = obj.value("description").toString().trimmed();
    item.date = obj.value("date").toString().trimmed();
    item.author = obj.value("author").toString().trimmed();
    items.append(item);
  }
  return items;
}

QList<XPathNewsItem> XPathFeedParser::parse(const QString &html, const QString &fetchRule)
{
  QList<XPathNewsItem> items;
  fetchRule_ = fetchRule;

  QString js = buildScript();
  if (js.isEmpty())
    return items;

  QEventLoop loop;

  bool loaded = false;
  connect(page_, &QWebEnginePage::loadFinished, &loop, [&](bool ok) {
    loaded = ok;
    loop.quit();
  });
  page_->setHtml(html);
  loop.exec();
  disconnect(page_, &QWebEnginePage::loadFinished, &loop, nullptr);

  if (!loaded)
    return items;

  bool done = false;
  page_->runJavaScript(js, [&](const QVariant &result) {
    items = decodeResult(result);
    done = true;
    loop.quit();
  });
  if (!done)
    loop.exec();

  return items;
}

void XPathFeedParser::parseAsync(const QString &html, const QString &fetchRule)
{
  fetchRule_ = fetchRule;
  connect(page_, &QWebEnginePage::loadFinished, this, &XPathFeedParser::onHtmlLoaded, Qt::UniqueConnection);
  page_->setHtml(html);
}

void XPathFeedParser::onHtmlLoaded(bool ok)
{
  disconnect(page_, &QWebEnginePage::loadFinished, this, &XPathFeedParser::onHtmlLoaded);
  if (!ok) {
    emit parseFinished(QList<XPathNewsItem>());
    return;
  }

  QString js = buildScript();
  if (js.isEmpty()) {
    emit parseFinished(QList<XPathNewsItem>());
    return;
  }

  page_->runJavaScript(js, [this](const QVariant& result) {
    emit parseFinished(decodeResult(result));
  });
}

/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 QuiteRSS Team <quiterssteam@gmail.com>
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

#include <QWebPage>
#include <QWebFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

XPathFeedParser::XPathFeedParser(QObject *parent)
  : QObject(parent)
{
  page_ = new QWebPage(this);
  page_->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
}

XPathFeedParser::~XPathFeedParser()
{
  delete page_;
}

/*! Quote a string as a JS string literal (safe to embed in JS source). */
static QString jsString(const QString &s)
{
  return QJsonDocument(QJsonArray() << s).toJson(QJsonDocument::Compact).
      trimmed().mid(1).chopped(1);
}

bool XPathFeedParser::load(const QString &html)
{
  page_->mainFrame()->setHtml(html);
  return true;
}

QString XPathFeedParser::evalString(const QString &expression)
{
  if (expression.isEmpty()) return QString();

  QString js = QString(
    "var r = document.evaluate(%1, document, null, "
    "XPathResult.FIRST_ORDERED_NODE_TYPE, null);"
    "r.singleNodeValue ? r.singleNodeValue.textContent : ''").
      arg(jsString(expression));
  QVariant result = page_->mainFrame()->evaluateJavaScript(js);
  return result.toString().trimmed();
}

QStringList XPathFeedParser::evalNodeList(const QString &expression)
{
  QStringList list;
  if (expression.isEmpty()) return list;

  QString js = QString(
    "var out = [];"
    "var r = document.evaluate(%1, document, null, "
    "XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);"
    "for (var i = 0; i < r.snapshotLength; i++) {"
    "  out.push(r.snapshotItem(i).textContent);"
    "}"
    "out;").arg(jsString(expression));

  QVariant result = page_->mainFrame()->evaluateJavaScript(js);
  if (result.type() == QVariant::StringList) {
    list = result.toStringList();
  } else if (result.canConvert(QVariant::List)) {
    foreach (const QVariant &v, result.toList()) {
      list << v.toString().trimmed();
    }
  }
  return list;
}

QList<XPathNewsItem> XPathFeedParser::parse(const QString &html,
                                           const QString &fetchRule)
{
  QList<XPathNewsItem> items;

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(fetchRule.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    return items;

  QJsonObject rule = doc.object();
  QString itemExpr = rule.value("item").toString();
  QString titleExpr = rule.value("title").toString();
  QString linkExpr = rule.value("link").toString();
  QString descExpr = rule.value("description").toString();
  QString dateExpr = rule.value("date").toString();
  QString authorExpr = rule.value("author").toString();

  if (itemExpr.isEmpty())
    return items;

  load(html);

  // Snapshot all item nodes once; each field expression is then evaluated
  // with the corresponding item node as context.
  int count = 0;
  QString snapshotJs = QString(
    "var r = document.evaluate(%1, document, null, "
    "XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);"
    "window.__xpathCount = r.snapshotLength;"
    "r.snapshotLength;").arg(jsString(itemExpr));
  bool ok = false;
  count = page_->mainFrame()->evaluateJavaScript(snapshotJs).toInt(&ok);
  if (!ok || count <= 0)
    return items;

  for (int i = 0; i < count; ++i) {
    XPathNewsItem item;
    QString ctxJs = QString(
      "document.evaluate(%1, document, null, "
      "XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null).snapshotItem(%2);")
        .arg(jsString(itemExpr)).arg(i);

    if (!titleExpr.isEmpty()) {
      QString js = QString(
        "var r = document.evaluate(%1, %2, null, "
        "XPathResult.FIRST_ORDERED_NODE_TYPE, null);"
        "r.singleNodeValue ? r.singleNodeValue.textContent : '';")
          .arg(jsString(titleExpr), ctxJs);
      item.title = page_->mainFrame()->evaluateJavaScript(js).toString().trimmed();
    }

    if (!linkExpr.isEmpty()) {
      QString js = QString(
        "var r = document.evaluate(%1, %2, null, "
        "XPathResult.FIRST_ORDERED_NODE_TYPE, null);"
        "var n = r.singleNodeValue;"
        "n ? (n.href ? n.href : n.textContent) : '';")
          .arg(jsString(linkExpr), ctxJs);
      item.link = page_->mainFrame()->evaluateJavaScript(js).toString().trimmed();
    }

    if (!descExpr.isEmpty()) {
      QString js = QString(
        "var r = document.evaluate(%1, %2, null, "
        "XPathResult.FIRST_ORDERED_NODE_TYPE, null);"
        "r.singleNodeValue ? r.singleNodeValue.textContent : '';")
          .arg(jsString(descExpr), ctxJs);
      item.description = page_->mainFrame()->evaluateJavaScript(js).toString().trimmed();
    }

    if (!dateExpr.isEmpty()) {
      QString js = QString(
        "var r = document.evaluate(%1, %2, null, "
        "XPathResult.FIRST_ORDERED_NODE_TYPE, null);"
        "var n = r.singleNodeValue;"
        "n ? (n.getAttribute ? (n.getAttribute('datetime') || "
        "n.getAttribute('pubdate') || n.textContent) : n.textContent) : '';")
          .arg(jsString(dateExpr), ctxJs);
      item.date = page_->mainFrame()->evaluateJavaScript(js).toString().trimmed();
    }

    if (!authorExpr.isEmpty()) {
      QString js = QString(
        "var r = document.evaluate(%1, %2, null, "
        "XPathResult.FIRST_ORDERED_NODE_TYPE, null);"
        "r.singleNodeValue ? r.singleNodeValue.textContent : '';")
          .arg(jsString(authorExpr), ctxJs);
      item.author = page_->mainFrame()->evaluateJavaScript(js).toString().trimmed();
    }

    if (!item.title.isEmpty() || !item.link.isEmpty())
      items.append(item);
  }

  return items;
}

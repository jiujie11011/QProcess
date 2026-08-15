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
#ifndef XPATHFEEDPARSER_H
#define XPATHFEEDPARSER_H

#include <QObject>
#include <QStringList>

class QWebPage;

struct XPathNewsItem {
  QString title;
  QString link;
  QString description;
  QString date;
  QString author;
};

/*! Parse an HTML/XML document using XPath expressions.
 *
 *  fetchRule is a JSON object mapping field names to XPath expressions,
 *  e.g.:
 *    {"item":"//div[contains(@class,'item')]",
 *     "title":".//h2/a",
 *     "link":".//h2/a/@href",
 *     "description":".//div[contains(@class,'desc')]",
 *     "date":".//time/@datetime"}
 *
 *  The document is loaded into a hidden QWebPage and evaluated via
 *  document.evaluate() (JavaScript XPath), which supports HTML
 *  documents that a pure XML parser would reject.
 */
class XPathFeedParser : public QObject
{
  Q_OBJECT
public:
  explicit XPathFeedParser(QObject *parent = 0);
  ~XPathFeedParser();

  /*! Parse html content using the expressions in fetchRule.
   *  Returns a list of extracted news items. */
  QList<XPathNewsItem> parse(const QString &html, const QString &fetchRule);

  /*! Load the document once (needed before parse()). */
  bool load(const QString &html);

private:
  QString evalString(const QString &expression);
  QStringList evalNodeList(const QString &expression);

  QWebPage *page_;
};

#endif // XPATHFEEDPARSER_H

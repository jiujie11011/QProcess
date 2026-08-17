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
#ifndef XPATHFEEDPARSER_H
#define XPATHFEEDPARSER_H

#include <QObject>
#include <QStringList>
#include <QWebEnginePage>

struct XPathNewsItem {
  QString title;
  QString link;
  QString description;
  QString date;
  QString author;
};

class XPathFeedParser : public QObject
{
  Q_OBJECT
public:
  explicit XPathFeedParser(QObject *parent = 0);
  ~XPathFeedParser();

  void parseAsync(const QString &html, const QString &fetchRule);
  QList<XPathNewsItem> parse(const QString &html, const QString &fetchRule);

signals:
  void parseFinished(const QList<XPathNewsItem> &items);

private:
  void onHtmlLoaded(bool ok);
  QString buildScript() const;
  static QList<XPathNewsItem> decodeResult(const QVariant &result);

  QString fetchRule_;
  QWebEnginePage *page_;
};

#endif // XPATHFEEDPARSER_H
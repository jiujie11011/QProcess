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
#ifndef FEEDURLDETECTOR_H
#define FEEDURLDETECTOR_H

#include <QObject>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

/** @brief Auto-detects RSS/Atom feed URLs for a given website, mirroring
 *         MrRSS's discovery logic.
 *
 *  1. Fetch the site page and extract <link rel="alternate"
 *     type="application/rss+xml|atom+xml"> references.
 *  2. If none is found, probe common feed paths (/feed, /rss.xml,
 *     /atom.xml, ...) and sniff the content for XML feed markers.
 *
 *  All found feed URLs are reported via signalFeedFound; when nothing
 *  matches, signalNoFeedFound is emitted. The whole flow is asynchronous.
 *----------------------------------------------------------------------------*/
class FeedUrlDetector : public QObject
{
  Q_OBJECT
public:
  explicit FeedUrlDetector(QObject *parent = 0);
  ~FeedUrlDetector();

  void discover(const QString &siteUrl);
  void stop();

signals:
  void signalFeedFound(const QStringList &feedUrls);
  void signalNoFeedFound();

private slots:
  void slotReplyFinished(QNetworkReply *reply);

private:
  void probeNext();
  void finishDiscovery(bool found);
  void extractFeedLinks(const QByteArray &html, const QString &pageUrl);
  QString resolveUrl(const QString &baseUrl, const QString &href);
  bool isFeedContent(const QByteArray &data);

  QNetworkAccessManager *networkManager_;
  QNetworkReply *currentReply_;
  QString baseUrl_;
  QStringList candidateUrls_;
  QStringList foundFeeds_;
  bool running_;

};

#endif // FEEDURLDETECTOR_H

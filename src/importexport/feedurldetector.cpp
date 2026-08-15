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
#include "feedurldetector.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

// ----------------------------------------------------------------------------
FeedUrlDetector::FeedUrlDetector(QObject *parent)
  : QObject(parent)
  , networkManager_(0)
  , currentReply_(0)
  , running_(false)
{
}
// ----------------------------------------------------------------------------
FeedUrlDetector::~FeedUrlDetector()
{
  stop();
  delete networkManager_;
}
// ----------------------------------------------------------------------------
void FeedUrlDetector::discover(const QString &siteUrl)
{
  if (running_)
    return;
  running_ = true;
  foundFeeds_.clear();

  QUrl url = QUrl::fromUserInput(siteUrl);
  if (url.scheme().isEmpty())
    url.setScheme("http");

  baseUrl_ = url.toString(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment);
  if (baseUrl_.endsWith('/'))
    baseUrl_.chop(1);

  if (!networkManager_) {
    networkManager_ = new QNetworkAccessManager(this);
    connect(networkManager_, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(slotReplyFinished(QNetworkReply*)));
  }

  // First probe is the site page itself.
  candidateUrls_.clear();
  candidateUrls_ << url.toString();

  probeNext();
}
// ----------------------------------------------------------------------------
void FeedUrlDetector::finishDiscovery(bool found)
{
  running_ = false;
  if (found)
    emit signalFeedFound(foundFeeds_);
  else
    emit signalNoFeedFound();
}

void FeedUrlDetector::probeNext()
{
  if (candidateUrls_.isEmpty()) {
    finishDiscovery(!foundFeeds_.isEmpty());
    return;
  }

  QString urlStr = candidateUrls_.takeFirst();
  QNetworkRequest request(QUrl(urlStr));
  request.setRawHeader("User-Agent", "QuiteRSS/0.19 (feed discovery)");
  request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                       QNetworkRequest::AlwaysNetwork);
  request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
  currentReply_ = networkManager_->get(request);
}
// ----------------------------------------------------------------------------
void FeedUrlDetector::slotReplyFinished(QNetworkReply *reply)
{
  if (reply != currentReply_)
    return;
  currentReply_ = 0;

  QString urlStr = reply->url().toString();
  QByteArray data = reply->readAll();
  bool error = (reply->error() != QNetworkReply::NoError);
  reply->deleteLater();

  if (error) {
    probeNext();
    return;
  }

  // The response itself is a feed.
  if (isFeedContent(data)) {
    if (!foundFeeds_.contains(urlStr))
      foundFeeds_ << urlStr;
    finishDiscovery(true);
    return;
  }

  if (candidateUrls_.isEmpty()) {
    // The homepage fetched: look for feed <link> tags first...
    extractFeedLinks(data, urlStr);
    if (!foundFeeds_.isEmpty()) {
      finishDiscovery(true);
      return;
    }
    // ...then probe common feed paths (MrRSS-style fallback).
    QStringList paths;
    paths << "/feed" << "/rss" << "/rss.xml" << "/atom.xml" << "/feed.xml"
          << "/index.xml" << "/feed/" << "/rss/" << "/atom/"
          << "/feeds/posts/default" << "/?feed=rss2" << "/blog/feed";
    foreach (const QString &path, paths)
      candidateUrls_ << baseUrl_ + path;
  }
  probeNext();
}
// ----------------------------------------------------------------------------
void FeedUrlDetector::extractFeedLinks(const QByteArray &html, const QString &pageUrl)
{
  QString str = QString::fromUtf8(html);
  QRegularExpression linkRx(
        "<link\\b[^>]*\\b(?:atom|rss)\\+xml\\b[^>]*>",
        QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatchIterator it = linkRx.globalMatch(str);
  while (it.hasNext()) {
    QString tag = it.next().captured(0);
    QRegularExpression hrefRx(
          "href\\s*=\\s*[\"']([^\"']+)[\"']",
          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch hrefMatch = hrefRx.match(tag);
    if (!hrefMatch.hasMatch())
      continue;
    QString feedUrl = resolveUrl(pageUrl, hrefMatch.captured(1));
    if (!feedUrl.isEmpty() && !foundFeeds_.contains(feedUrl))
      foundFeeds_ << feedUrl;
  }
}
// ----------------------------------------------------------------------------
QString FeedUrlDetector::resolveUrl(const QString &baseUrl, const QString &href)
{
  QString h = href.trimmed();
  if (h.isEmpty())
    return QString();
  QUrl u = QUrl(baseUrl).resolved(QUrl(h));
  if (!u.isValid() || u.host().isEmpty())
    return QString();
  return u.toString();
}
// ----------------------------------------------------------------------------
bool FeedUrlDetector::isFeedContent(const QByteArray &data)
{
  QString s = QString::fromUtf8(data);
  return s.contains(QLatin1String("<?xml")) ||
         s.contains(QLatin1String("<rss")) ||
         s.contains(QLatin1String("<feed")) ||
         s.contains(QLatin1String("<atom")) ||
         s.contains(QLatin1String("<rdf:RDF"));
}
// ----------------------------------------------------------------------------
void FeedUrlDetector::stop()
{
  if (currentReply_) {
    QNetworkReply *reply = currentReply_;
    currentReply_ = 0;
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
  }
  running_ = false;
}
// ----------------------------------------------------------------------------

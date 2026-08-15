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
#include "requestfeed.h"
#include "VersionNo.h"
#include "mainapplication.h"
#include "globals.h"

#include <QDebug>
#include <QtSql>
#include <qzregexp.h>

#define REPLY_MAX_COUNT 10

RequestFeed::RequestFeed(int timeoutRequest, int numberRequests,
                         int numberRepeats, QObject *parent)
  : QObject(parent)
  , timeoutRequest_(timeoutRequest)
  , numberRequests_(numberRequests)
  , numberRepeats_(numberRepeats)
  , tasksDone_(0)
  , tasksFailed_(0)
{
  setObjectName("requestFeed_");

  timeout_ = new QTimer(this);
  timeout_->setInterval(1000);
  connect(timeout_, SIGNAL(timeout()), this, SLOT(slotRequestTimeout()));

  getUrlTimer_ = new QTimer(this);
  getUrlTimer_->setSingleShot(true);
  getUrlTimer_->setInterval(50);
  connect(getUrlTimer_, SIGNAL(timeout()), this, SLOT(getQueuedUrl()));

  connect(this, SIGNAL(signalHead(QUrl,int,QString,QDateTime,int,QString)),
          SLOT(slotHead(QUrl,int,QString,QDateTime,int,QString)),
          Qt::QueuedConnection);
  connect(this, SIGNAL(signalGet(QUrl,int,QString,QDateTime,int,QString)),
          SLOT(slotGet(QUrl,int,QString,QDateTime,int,QString)),
          Qt::QueuedConnection);
}

RequestFeed::~RequestFeed()
{

}

void RequestFeed::disconnectObjects()
{
  disconnect(this);
  QHash<QString, NetworkManager*>::const_iterator it = networkManagers_.constBegin();
  for (; it != networkManagers_.constEnd(); ++it) {
    if (it.value())
      it.value()->disconnect(it.value());
  }
}

/** @brief Put URL in request queue
 *----------------------------------------------------------------------------*/
void RequestFeed::requestUrl(int id, QString urlString,
                              QDateTime date, QString userInfo,
                              QString proxyUrl, bool highPriority)
{
  getNetworkManager(proxyUrl);

  if (!timeout_->isActive())
    timeout_->start();

  // Task manager: replace an existing queued entry for the same feed so the
  // newest request wins. High-priority (manual) requests jump to queue head.
  int queueIndex = idsQueue_.indexOf(id);
  if (queueIndex >= 0) {
    idsQueue_.removeAt(queueIndex);
    feedsQueue_.removeAt(queueIndex);
    dateQueue_.removeAt(queueIndex);
    userInfo_.removeAt(queueIndex);
    proxyQueue_.removeAt(queueIndex);
  }

  if (highPriority) {
    idsQueue_.prepend(id);
    feedsQueue_.prepend(urlString);
    dateQueue_.prepend(date);
    userInfo_.prepend(userInfo);
    proxyQueue_.prepend(proxyUrl);
  } else {
    idsQueue_.enqueue(id);
    feedsQueue_.enqueue(urlString);
    dateQueue_.enqueue(date);
    userInfo_.enqueue(userInfo);
    proxyQueue_.enqueue(proxyUrl);
  }

  if (!getUrlTimer_->isActive())
    getUrlTimer_->start();

  emitTaskStats();
  qDebug() << "requestUrl() <<" << urlString << "countQueue=" << feedsQueue_.count();
}

/** @brief Get or create a NetworkManager for the given proxy URL
 *----------------------------------------------------------------------------*/
NetworkManager *RequestFeed::getNetworkManager(const QString &proxyUrl)
{
  if (networkManagers_.contains(proxyUrl))
    return networkManagers_.value(proxyUrl);

  NetworkManager *manager = new NetworkManager(true, this);
  connect(manager, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(finished(QNetworkReply*)));

  if (!proxyUrl.isEmpty()) {
    QUrl url(proxyUrl);
    QNetworkProxy proxy;
    if ((url.scheme() == "socks5") || (url.scheme() == "socks"))
      proxy.setType(QNetworkProxy::Socks5Proxy);
    else
      proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(url.host());
    proxy.setPort(url.port((proxy.type() == QNetworkProxy::Socks5Proxy) ? 1080 : 8080));
    proxy.setUser(url.userName());
    proxy.setPassword(url.password());
    manager->setProxy(proxy);
  }

  networkManagers_.insert(proxyUrl, manager);
  return manager;
}

/** @brief Adjust the worker pool size (dynamic concurrency)
 *----------------------------------------------------------------------------*/
void RequestFeed::setNumberRequests(int number)
{
  if (number < 1)
    number = 1;
  if (number > REPLY_MAX_COUNT)
    number = REPLY_MAX_COUNT;
  if (numberRequests_ != number) {
    numberRequests_ = number;
    if (!getUrlTimer_->isActive())
      getUrlTimer_->start();
  }
}

int RequestFeed::numberRequests() const
{
  return numberRequests_;
}

/** @brief Emit current task manager statistics
 *----------------------------------------------------------------------------*/
void RequestFeed::emitTaskStats()
{
  emit signalTaskStats(feedsQueue_.count(), currentFeeds_.count(),
                       tasksDone_, tasksFailed_);
}

/** @brief Count a finished task: result >= 0 means success
 *----------------------------------------------------------------------------*/
void RequestFeed::countTask(int result)
{
  if (result < 0)
    ++tasksFailed_;
  else
    ++tasksDone_;
  emitTaskStats();
}

void RequestFeed::stopRequest()
{
  dateQueue_.clear();
  userInfo_.clear();
  proxyQueue_.clear();
  while (!feedsQueue_.isEmpty()) {
    int feedId = idsQueue_.dequeue();
    QString feedUrl = feedsQueue_.dequeue();
    ++tasksFailed_;
    emit getUrlDone(feedsQueue_.count(), feedId, feedUrl);
  }
  emitTaskStats();
}

/** @brief Process request queue on timer timeouts
 *----------------------------------------------------------------------------*/
void RequestFeed::getQueuedUrl()
{
  if ((currentFeeds_.count() >= numberRequests_) ||
      (currentFeeds_.count() >= REPLY_MAX_COUNT)) {
    getUrlTimer_->start();
    return;
  }

  if (!feedsQueue_.isEmpty()) {
    getUrlTimer_->start();

    // Scan the queue for the first entry that can be sent right now.
    // A single host in hostList_ that still has an in-flight request must
    // not stall the whole queue, so blocked entries are skipped here and
    // retried on the next timer tick.
    int index = 0;
    while (index < feedsQueue_.count()) {
      QString feedUrl = feedsQueue_.at(index);
      if (hostList_.contains(QUrl(feedUrl).host())) {
        bool hostBusy = false;
        foreach (QString url, currentFeeds_) {
          if (QUrl(url).host() == QUrl(feedUrl).host()) {
            hostBusy = true;
            break;
          }
        }
        if (hostBusy) {
          ++index;
          continue;
        }
      }
      break;
    }

    if (index >= feedsQueue_.count())
      return;  // everything remaining is blocked behind active requests

    int feedId = idsQueue_.takeAt(index);
    QString feedUrl = feedsQueue_.takeAt(index);
    QString userInfo = userInfo_.takeAt(index);
    QString proxyUrl = proxyQueue_.takeAt(index);
    QDateTime currentDate = dateQueue_.takeAt(index);

    emit setStatusFeed(feedId, "1 Update");
    emit signalCurrentFeed(feedId, feedUrl);

    QUrl getUrl = QUrl::fromEncoded(feedUrl.toUtf8());
    if (!userInfo.isEmpty()) {
      getUrl.setUserInfo(userInfo);
    }

    qDebug() << "getQueuedUrl() >>" << feedUrl << "countQueue=" << feedsQueue_.count();
    if (currentDate.isValid())
      emit signalHead(getUrl, feedId, feedUrl, currentDate, 0, proxyUrl);
    else
      emit signalGet(getUrl, feedId, feedUrl, currentDate, 0, proxyUrl);
  }
}

/** @brief Prepare and send network request to get head
 *----------------------------------------------------------------------------*/
/** @brief Prepare and send network request (head or get)
 *----------------------------------------------------------------------------*/
void RequestFeed::sendRequest(const QUrl &getUrl, int id, const QString &feedUrl,
                              const QDateTime &date, int count, const QString &proxyUrl,
                              bool head)
{
  if (count)
    Common::sleep(30);

  QNetworkRequest request(getUrl);
  request.setRawHeader("User-Agent", globals.userAgent().toUtf8());
  if (!head) {
    request.setRawHeader("Accept", "application/atom+xml,application/rss+xml;q=0.9,application/xml;q=0.8,text/xml;q=0.7,*/*;q=0.6");
  }

  qDebug() << objectName() << (head ? "::head" : "::get") << ":" << getUrl.toEncoded()
           << "feed:" << feedUrl << "countRepeats:" << count;

  currentUrls_.append(getUrl);
  currentIds_.append(id);
  currentFeeds_.append(feedUrl);
  currentDates_.append(date);
  currentCount_.append(count);
  currentHead_.append(head);
  currentTime_.append(timeoutRequest_);
  currentProxy_.append(proxyUrl);

  QNetworkReply *reply = head
    ? getNetworkManager(proxyUrl)->head(request)
    : getNetworkManager(proxyUrl)->get(request);
  reply->setProperty("feedReply", QVariant(true));
  requestUrl_.append(reply->request().url());
  networkReply_.append(reply);
  emitTaskStats();
}

void RequestFeed::slotHead(const QUrl &getUrl, const int &id, const QString &feedUrl,
                            const QDateTime &date, const int &count,
                            const QString &proxyUrl)
{
  sendRequest(getUrl, id, feedUrl, date, count, proxyUrl, true);
}

/** @brief Prepare and send network request to get all data
 *----------------------------------------------------------------------------*/
void RequestFeed::slotGet(const QUrl &getUrl, const int &id, const QString &feedUrl,
                           const QDateTime &date, const int &count,
                           const QString &proxyUrl)
{
  sendRequest(getUrl, id, feedUrl, date, count, proxyUrl, false);
}

/** @brief Process network reply
 *----------------------------------------------------------------------------*/
void RequestFeed::finished(QNetworkReply *reply)
{
  // Use the *request* URL for matching below: reply->url() can differ after
  // an automatically followed redirect, which would leave the entry stuck
  // in the pool until its 15-second timeout.
  QUrl replyUrl = reply->request().url();

  qDebug() << "reply.finished():" << replyUrl.toString();
  qDebug() << reply->header(QNetworkRequest::ContentTypeHeader);
  qDebug() << reply->header(QNetworkRequest::ContentLengthHeader);
  qDebug() << reply->header(QNetworkRequest::LocationHeader);
  qDebug() << reply->header(QNetworkRequest::LastModifiedHeader);
  qDebug() << reply->header(QNetworkRequest::CookieHeader);
  qDebug() << reply->header(QNetworkRequest::SetCookieHeader);

  int currentReplyIndex = currentUrls_.indexOf(replyUrl);

  if (currentReplyIndex >= 0) {
    currentTime_.removeAt(currentReplyIndex);
    currentUrls_.removeAt(currentReplyIndex);
    int feedId    = currentIds_.takeAt(currentReplyIndex);
    QString feedUrl    = currentFeeds_.takeAt(currentReplyIndex);
    QDateTime feedDate = currentDates_.takeAt(currentReplyIndex);
    int count = currentCount_.takeAt(currentReplyIndex) + 1;
    bool headOk = currentHead_.takeAt(currentReplyIndex);
    QString proxyUrl = currentProxy_.takeAt(currentReplyIndex);

    if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "  error retrieving RSS feed:" << reply->error() << reply->errorString();
      if (!headOk) {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
          emit getUrlDone(-2, feedId, feedUrl, tr("Server requires authentication!"));
          countTask(-2);
        } else if (reply->error() == QNetworkReply::ContentNotFoundError) {
          emit getUrlDone(-5, feedId, feedUrl, tr("Server replied: Not Found!"));
          countTask(-5);
        }
        else {
          if (reply->errorString().contains("Service Temporarily Unavailable")) {
            if (!hostList_.contains(QUrl(feedUrl).host())) {
              hostList_.append(QUrl(feedUrl).host());
              count--;
            }
          }

          if (count < numberRepeats_) {
            emit signalGet(replyUrl, feedId, feedUrl, feedDate, count, proxyUrl);
          } else {
            emit getUrlDone(-1, feedId, feedUrl, QString("%1 (%2)").arg(reply->errorString()).arg(reply->error()));
            countTask(-1);
          }
        }
      } else {
        emit signalGet(replyUrl, feedId, feedUrl, feedDate, 0, proxyUrl);
      }
    } else {
      QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      if (redirectionTarget.isValid()) {
        if (count < (numberRepeats_ + 3)) {
          if (headOk && (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302)) {
            emit signalGet(replyUrl, feedId, feedUrl, feedDate, 0, proxyUrl);
          } else {
            QString host(QUrl::fromEncoded(feedUrl.toUtf8()).host());
            if (redirectionTarget.host().isEmpty()) {
              if (redirectionTarget.path() == ".") {
                if (redirectionTarget.hasQuery()) {
#if QT_VERSION >= 0x050000
                  QString query = redirectionTarget.query();
                  redirectionTarget.setUrl(replyUrl.scheme() + "://" + host + replyUrl.path());
                  redirectionTarget.setQuery(query);
#else
                  QByteArray query = redirectionTarget.encodedQuery();
                  redirectionTarget.setUrl(replyUrl.scheme() + "://" + host + replyUrl.path());
                  redirectionTarget.setEncodedQuery(query);
#endif
                }
              } else {
                redirectionTarget.setUrl(replyUrl.scheme() + "://" + host + redirectionTarget.toString());
              }
            }
            if (redirectionTarget.scheme().isEmpty())
              redirectionTarget.setScheme(QUrl(feedUrl).scheme());
            if (reply->operation() == QNetworkAccessManager::HeadOperation) {
              qDebug() << objectName() << "  head redirect..." << redirectionTarget.toString();
              emit signalHead(redirectionTarget, feedId, feedUrl, feedDate, count, proxyUrl);
            }
            else {
              qDebug() << objectName() << "  get redirect..." << redirectionTarget.toString();
              emit signalGet(redirectionTarget, feedId, feedUrl, feedDate, count, proxyUrl);
            }
          }
        } else {
          emit getUrlDone(-4, feedId, feedUrl, tr("Redirect error!"));
          countTask(-4);
        }
      } else {
        QDateTime replyDate = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
        QDateTime replyLocalDate = QDateTime(replyDate.date(), replyDate.time());

        qDebug() << feedDate << replyDate << replyLocalDate;
        qDebug() << feedDate.toMSecsSinceEpoch() << replyDate.toMSecsSinceEpoch() << replyLocalDate.toMSecsSinceEpoch();
        if ((reply->operation() == QNetworkAccessManager::HeadOperation) &&
            ((!feedDate.isValid()) || (!replyLocalDate.isValid()) ||
             (feedDate != replyLocalDate) || !replyDate.toMSecsSinceEpoch())) {
          emit signalGet(replyUrl, feedId, feedUrl, feedDate, 0, proxyUrl);
        }
        else {
          QString codecName;
          QzRegExp rx("charset=([^\t]+)$", Qt::CaseInsensitive);
          int pos = rx.indexIn(reply->header(QNetworkRequest::ContentTypeHeader).toString());
          if (pos > -1) {
            codecName = rx.cap(1);
          }

          QByteArray data = reply->readAll();
          data = data.trimmed();

          rx.setPattern("&(?!([a-z0-9#]+;))");
          pos = 0;
          while ((pos = rx.indexIn(QString::fromLatin1(data), pos)) != -1) {
            data.replace(pos, 1, "&amp;");
            pos += 1;
          }

          data.replace("<br>", "<br/>");

          if (data.indexOf("</rss>") > 0)
            data.resize(data.indexOf("</rss>") + 6);
          if (data.indexOf("</feed>") > 0)
            data.resize(data.indexOf("</feed>") + 7);
          if (data.indexOf("</rdf:RDF>") > 0)
            data.resize(data.indexOf("</rdf:RDF>") + 10);

          emit getUrlDone(feedsQueue_.count(), feedId, feedUrl, "", data, replyLocalDate, codecName);
          countTask(feedsQueue_.count());
        }
      }
    }
  } else {
    qCritical() << "Request Url error: " << replyUrl.toString() << reply->errorString();
  }

  int replyIndex = requestUrl_.indexOf(replyUrl);
  if (replyIndex >= 0) {
    requestUrl_.removeAt(replyIndex);
    networkReply_.removeAt(replyIndex);
  }

  reply->abort();
  reply->deleteLater();
}

/** @brief Timeout to delete network requests which has no answer
 *----------------------------------------------------------------------------*/
void RequestFeed::slotRequestTimeout()
{
  for (int i = currentTime_.count() - 1; i >= 0; i--) {
    int time = currentTime_.at(i) - 1;
    if (time <= 0) {
      // Read all values first, then remove them: consecutive takeAt(i)
      // calls would shift the lists and mix up feed id/url/proxy.
      QUrl url = currentUrls_.at(i);
      int feedId    = currentIds_.at(i);
      QString feedUrl = currentFeeds_.at(i);
      QDateTime feedDate = currentDates_.at(i);
      int count = currentCount_.at(i) + 1;
      QString proxyUrl = currentProxy_.at(i);

      currentUrls_.removeAt(i);
      currentIds_.removeAt(i);
      currentFeeds_.removeAt(i);
      currentDates_.removeAt(i);
      currentCount_.removeAt(i);
      currentTime_.removeAt(i);
      currentHead_.removeAt(i);
      currentProxy_.removeAt(i);

      int replyIndex = requestUrl_.indexOf(url);
      if (replyIndex >= 0) {
        QUrl replyUrl = requestUrl_.takeAt(replyIndex);
        QNetworkReply *reply = networkReply_.takeAt(replyIndex);
        reply->deleteLater();

        if (count < numberRepeats_) {
          emit signalGet(replyUrl, feedId, feedUrl, feedDate, count, proxyUrl);
        } else {
          emit getUrlDone(-3, feedId, feedUrl, tr("Request timeout!"));
          countTask(-3);
        }
      }
    } else {
      currentTime_.replace(i, time);
    }
  }
}

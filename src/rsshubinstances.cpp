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
#include "rsshubinstances.h"

#include <QEventLoop>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QSettings>
#include <QDateTime>
#include <algorithm>
#include <QSqlQuery>
#include <QTimer>
#include <QUrl>

QMutex RssHubInstances::mutex_;
QStringList RssHubInstances::healthyCache_;
bool RssHubInstances::healthyCacheValid_ = false;

/** Static map: instance base -> consecutive failure count. */
static QHash<QString, int> &failureCounts()
{
  static QHash<QString, int> counts;
  return counts;
}

/** Static set: instances already confirmed dead. */
static QSet<QString> &badInstances()
{
  static QSet<QString> set;
  return set;
}

QStringList RssHubInstances::defaultInstances()
{
  QStringList list;
  list << "https://rsshub.app"
       << "https://rsshub.rssforever.com"
       << "https://rss.injahow.cn"
       << "https://i.scnu.edu.cn/sub";
  return list;
}

/** Normalise a single instance base URL: trim whitespace, strip a single
 *  trailing slash and lowercase the host so that duplicates that differ only
 *  in casing/trailing slash collapse together. Returns an empty string for
 *  values that are clearly not usable as an instance base. */
QString RssHubInstances::normalizeBase(const QString &raw)
{
  QString s = raw.trimmed();
  if (s.isEmpty())
    return QString();
  if (!s.startsWith("http://", Qt::CaseInsensitive) &&
      !s.startsWith("https://", Qt::CaseInsensitive))
    return QString();

  QUrl url(s, QUrl::TolerantMode);
  if (!url.isValid() || url.host().isEmpty())
    return QString();

  // Code-hosting sites are never RSSHub instances; users sometimes paste the
  // source repository URL (e.g. github.com/DIYgod/RSSHub) by mistake.
  {
    QString host = url.host().toLower();
    if (host == "github.com" || host == "gitlab.com" ||
        host == "gitee.com" || host == "bitbucket.org")
      return QString();
  }

  // Rebuild from the parsed components to normalise casing/encoding. Keep a
  // path only when it looks like a reverse-proxy mount (e.g.
  // "https://i.scnu.edu.cn/sub"); drop query/fragment which are meaningless
  // for an instance base.
  QString result = url.scheme().toLower() + "://" + url.host().toLower();
  if (url.port() != -1)
    result += ":" + QString::number(url.port());
  QString path = url.path();
  while (path.endsWith('/'))
    path.chop(1);
  if (!path.isEmpty())
    result += path;
  return result;
}

QStringList RssHubInstances::loadInstances()
{
  QSettings settings;
  QStringList raw = settings.value("RSSHub/instances").toStringList();
  if (raw.isEmpty())
    raw = defaultInstances();

  // Normalise and de-duplicate (case/trailing-slash differences), preserving
  // first-seen order.
  QStringList list;
  QSet<QString> seen;
  foreach (const QString &item, raw) {
    QString base = normalizeBase(item);
    if (base.isEmpty() || seen.contains(base))
      continue;
    seen.insert(base);
    list << base;
  }
  return list;
}

void RssHubInstances::saveInstances(const QStringList &instances)
{
  // Normalise + de-dupe before persisting so duplicate entries and
  // non-URL garbage the user pasted do not accumulate.
  QStringList cleaned;
  QSet<QString> seen;
  foreach (const QString &item, instances) {
    QString base = normalizeBase(item);
    if (base.isEmpty() || seen.contains(base))
      continue;
    seen.insert(base);
    cleaned << base;
  }

  QSettings settings;
  settings.setValue("RSSHub/instances", cleaned);
  QMutexLocker locker(&mutex_);
  badInstances().clear();
  failureCounts().clear();
  healthyCacheValid_ = false;
}

bool RssHubInstances::autoSwapEnabled()
{
  QSettings settings;
  return settings.value("RSSHub/enabled", true).toBool();
}

void RssHubInstances::setAutoSwapEnabled(bool enabled)
{
  QSettings settings;
  settings.setValue("RSSHub/enabled", enabled);
}

QString RssHubInstances::instanceOfUrl(const QString &url)
{
  foreach (const QString &base, loadInstances()) {
    QString baseUrl = base;
    if (baseUrl.endsWith('/'))
      baseUrl.chop(1);
    if (url.startsWith(baseUrl + "/") || url == baseUrl)
      return baseUrl;
  }
  return QString();
}

QString RssHubInstances::swapInstance(const QString &url, const QString &newBase)
{
  QString base = instanceOfUrl(url);
  if (base.isEmpty())
    return url;
  QString newUrl = newBase;
  if (newUrl.endsWith('/'))
    newUrl.chop(1);
  return newUrl + url.mid(base.length());
}

bool RssHubInstances::isAlive(const QString &base, int timeoutMs)
{
  QNetworkAccessManager manager;
  // Probe the base itself (don't force a trailing slash, which can 404 on
  // path-mounted instances like https://i.scnu.edu.cn/sub). The server
  // handles a missing trailing slash with a redirect or the index page.
  // NB: use an intermediate variable; "QNetworkRequest request(QUrl(base))"
  // is parsed as a function declaration (most vexing parse) by GCC.
  QUrl probeUrl(base);
  QNetworkRequest request(probeUrl);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QString("Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                            "AppleWebKit/537.36 (KHTML, like Gecko) "
                            "Chrome/126.0.0.0 Safari/537.36"));
  request.setRawHeader("Accept", "application/xml,text/html,*/*");

  QNetworkReply *reply = manager.get(request);

  QEventLoop loop;
  QTimer timer;
  timer.setSingleShot(true);
  bool timedOut = false;
  QObject::connect(&timer, &QTimer::timeout, [&]() {
    timedOut = true;
    reply->abort();
    loop.quit();
  });
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  timer.start(timeoutMs);
  loop.exec();

  bool alive = false;
  if (!timedOut && reply->isFinished()) {
    // A reachable instance is any 2xx/3xx. TLS failures, DNS errors and the
    // like leave the status code at 0, so they correctly read as "down".
    int code = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    alive = (code >= 200 && code < 400);
  }
  reply->abort();
  reply->deleteLater();
  return alive;
}

QStringList RssHubInstances::checkAlive(const QStringList &instances)
{
  QStringList healthy;
  foreach (const QString &base, instances) {
    if (isAlive(base))
      healthy << base;
  }
  return healthy;
}

QStringList RssHubInstances::fetchRemote(const QString &remoteUrl)
{
  QStringList list;
  QUrl url(remoteUrl);
  if (!url.isValid() || !url.scheme().startsWith("http"))
    return list;

  QNetworkAccessManager manager;
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QString("Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                            "AppleWebKit/537.36 (KHTML, like Gecko) "
                            "Chrome/126.0.0.0 Safari/537.36"));

  QNetworkReply *reply = manager.get(request);

  QEventLoop loop;
  QTimer timer;
  timer.setSingleShot(true);
  bool timedOut = false;
  QObject::connect(&timer, &QTimer::timeout, [&]() {
    timedOut = true;
    reply->abort();
    loop.quit();
  });
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  timer.start(15000);
  loop.exec();

  if (!timedOut && reply->isFinished() && reply->error() == QNetworkReply::NoError) {
    QByteArray body = reply->readAll();
    QSet<QString> seen;
    foreach (const QString &line, QString::fromUtf8(body).split('\n')) {
      QString base = normalizeBase(line);
      if (!base.isEmpty() && !seen.contains(base)) {
        seen.insert(base);
        list << base;
      }
    }
  }
  reply->abort();
  reply->deleteLater();
  return list;
}

void RssHubInstances::updateHealthyCache()
{
  QStringList healthy = checkAlive(loadInstances());
  QMutexLocker locker(&mutex_);
  healthyCache_ = healthy;
  healthyCacheValid_ = true;
}

QStringList RssHubInstances::cachedHealthy()
{
  QMutexLocker locker(&mutex_);
  if (!healthyCacheValid_)
    return QStringList();
  return healthyCache_;
}

QString RssHubInstances::pickHealthy(const QStringList &instances,
                                     const QString &exceptBase)
{
  // Prefer a base already known to be alive from the cached health check.
  // This runs on the feed update thread and must NOT block on the network,
  // so we never fall back to a live isAlive() probe here.
  QMutexLocker locker(&mutex_);
  if (healthyCacheValid_) {
    foreach (const QString &base, healthyCache_) {
      if (base != exceptBase)
        return base;
    }
  }
  return QString();
}

QString RssHubInstances::handleFeedFailure(int feedId, const QString &feedUrl)
{
  Q_UNUSED(feedId);
  if (!autoSwapEnabled())
    return QString();

  QString base = instanceOfUrl(feedUrl);
  if (base.isEmpty())
    return QString();

  // A frozen instance (monthly failure threshold exceeded) is never swapped
  // to — it gets parked automatically.
  if (isFrozen(base))
    return QString();

  // Record this failure. If it just crossed the monthly threshold, freeze it
  // and don't swap (let the user decide / wait for next manual check).
  recordFailure(base);
  if (isFrozen(base)) {
    qWarning() << "RSSHub instance frozen after exceeding monthly failures:" << base;
    return QString();
  }

  // If we don't yet know which instances are healthy (cache not populated),
  // do not perform a blocking health check here — that would stall / crash the
  // update thread via a nested event loop. Let the user run "Check
  // Availability" in Settings, which populates the cache on the GUI thread.
  QString newBase = pickHealthy(loadInstances(), base);
  if (newBase.isEmpty() || newBase == base)
    return QString();

  QString newUrl = swapInstance(feedUrl, newBase);
  if (newUrl == feedUrl)
    return QString();

  return newUrl;
}

bool RssHubInstances::recordFailure(const QString &base)
{
  QMutexLocker locker(&mutex_);
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  QList<qint64> &list = failureTimestamps()[base];
  list.append(now);
  // Keep only failures within the rolling 30-day window.
  list.erase(std::remove_if(list.begin(), list.end(),
             [now](qint64 t) { return (now - t) > MONTH_MS; }),
             list.end());
  if (list.size() >= MAX_FAILURES_PER_MONTH) {
    frozenInstancesSet().insert(base);
    return true;
  }
  return false;
}

bool RssHubInstances::isFrozen(const QString &base)
{
  QMutexLocker locker(&mutex_);
  return frozenInstancesSet().contains(base);
}

QStringList RssHubInstances::frozenInstances()
{
}

void RssHubInstances::unfreezeInstance(const QString &base)
{
  QMutexLocker locker(&mutex_);
  frozenInstancesSet().remove(base);
  failureTimestamps().remove(base);
}

QSet<QString> &RssHubInstances::frozenInstancesSet()
{
  static QSet<QString> set;
  return set;
}

QHash<QString, QList<qint64>> &RssHubInstances::failureTimestamps()
{
  static QHash<QString, QList<qint64>> map;
  return map;
}

QStringList RssHubInstances::frozenInstances()
{
  QMutexLocker locker(&mutex_);
  return frozenInstancesSet().values();
}

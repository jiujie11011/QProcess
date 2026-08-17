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
#include <QVariant>
#include <QCoreApplication>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <mutex>

QMutex RssHubInstances::mutex_;
QStringList RssHubInstances::healthyCache_;
bool RssHubInstances::healthyCacheValid_ = false;
QStringList RssHubInstances::cachedInstances_;
bool RssHubInstances::instancesCacheValid_ = false;
bool RssHubInstances::stateDirty_ = false;

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
  QMutexLocker locker(&mutex_);
  if (instancesCacheValid_)
    return cachedInstances_;

  // Read + normalise the settings file without holding the mutex (the work is
  // idempotent), then publish the result into the cache.
  locker.unlock();
  QStringList list;
  {
    QSettings settings;
    QStringList raw = settings.value("RSSHub/instances").toStringList();
    if (raw.isEmpty())
      raw = defaultInstances();

    // Normalise and de-duplicate (case/trailing-slash differences),
    // preserving first-seen order.
    QSet<QString> seen;
    foreach (const QString &item, raw) {
      QString base = normalizeBase(item);
      if (base.isEmpty() || seen.contains(base))
        continue;
      seen.insert(base);
      list << base;
    }
  }
  locker.relock();
  // A concurrent saveInstances() may have published a newer list while we
  // were reading; do not clobber it.
  if (!instancesCacheValid_) {
    cachedInstances_ = list;
    instancesCacheValid_ = true;
  }
  return cachedInstances_;
}

QStringList RssHubInstances::loadActiveInstances()
{
  // loadInstances() takes the mutex itself, so it must be called without the
  // lock held (QMutex is not recursive).
  const QStringList all = loadInstances();
  QMutexLocker locker(&mutex_);
  startupLoad();
  QStringList list;
  foreach (const QString &base, all) {
    if (!frozenInstancesSet().contains(base))
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
  cachedInstances_ = cleaned;     // refresh the cache immediately
  instancesCacheValid_ = true;
  badInstances().clear();
  failureCounts().clear();
  // Failure records for instances that are no longer in the list are stale.
  failureTimestamps().clear();
  // Drop frozen flags for instances that were removed from the list.
  QSet<QString> cleanedSet;
  foreach (const QString &base, cleaned)
    cleanedSet.insert(base);
  QSet<QString> &frozen = frozenInstancesSet();
  foreach (const QString &base, frozen) {
    if (!cleanedSet.contains(base))
      frozen.remove(base);
  }
  // Persist the cleaned-up state (empty frozen set / timestamps included).
  persistState();
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
  // Frozen instances are never probed or cached, so automatic switching
  // cannot pick them.
  QStringList healthy = checkAlive(loadActiveInstances());
  QMutexLocker locker(&mutex_);
  healthyCache_ = healthy;
  healthyCacheValid_ = true;
}

void RssHubInstances::updateHealthyCacheAsync()
{
  // Run the blocking probes on a worker thread so the GUI stays responsive
  // (each instance can take up to its timeout before being declared down).
  QFutureWatcher<QStringList> *watcher =
      new QFutureWatcher<QStringList>(qApp);
  QObject::connect(watcher, &QFutureWatcher<QStringList>::finished, qApp,
                   [watcher]() {
    const QStringList healthy = watcher->result();
    QMutexLocker locker(&mutex_);
    healthyCache_ = healthy;
    healthyCacheValid_ = true;
    watcher->deleteLater();
  });
  watcher->setFuture(QtConcurrent::run([]() -> QStringList {
    return checkAlive(loadActiveInstances());
  }));
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
      // Never swap to an instance that exceeded the monthly failure
      // threshold, even if it happens to sit in the healthy cache.
      if (base != exceptBase && !frozenInstancesSet().contains(base))
        return base;
    }
  }
  return QString();
}

QString RssHubInstances::handleFeedFailure(int feedId, const QString &feedUrl,
                                           int result)
{
  Q_UNUSED(feedId);
  if (!autoSwapEnabled())
    return QString();

  QString base = instanceOfUrl(feedUrl);
  if (base.isEmpty())
    return QString();

  // Feed-side errors are not the instance's fault: a dead link (404) or a
  // feed that requires authentication would fail identically on every
  // instance. Swapping is pointless and the failure must not count against
  // the instance's health.
  if (result == FAIL_NOT_FOUND || result == FAIL_AUTH)
    return QString();

  // A frozen instance (monthly failure threshold exceeded) is never swapped
  // to — it gets parked automatically.
  if (isFrozen(base))
    return QString();

  // Record this failure. If it just crossed the monthly threshold, freeze it
  // and don't swap (let the user decide / wait for next manual check).
  recordFailure(base);
  if (isFrozen(base)) {
    qWarning() << "RSSHub instance" << base
               << "is frozen; feed is parked, no automatic swap.";
    return QString();
  }

  // If we don't yet know which instances are healthy (cache not populated),
  // do not perform a blocking health check here — that would stall / crash the
  // update thread via a nested event loop. Let the user run "Check
  // Availability" in Settings, which populates the cache on the GUI thread.
  // Only active (non-frozen) instances are swap candidates.
  QString newBase = pickHealthy(loadActiveInstances(), base);
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
  startupLoad();
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  QList<qint64> &list = failureTimestamps()[base];
  // Deduplicate: when many feeds fail concurrently on the same instance (a
  // single outage), they should count as one failure — otherwise one
  // incident could instantly hit the monthly threshold and freeze it.
  if (!list.isEmpty() && (now - list.last()) < FAILURE_DEDUP_MS)
    return frozenInstancesSet().contains(base);
  list.append(now);
  // Keep only failures within the rolling 30-day window.
  list.erase(std::remove_if(list.begin(), list.end(),
             [now](qint64 t) { return (now - t) > MONTH_MS; }),
             list.end());
  const bool frozen = list.size() >= MAX_FAILURES_PER_MONTH;
  if (frozen && !frozenInstancesSet().contains(base)) {
    // Newly frozen: persist so the instance stays disabled across restarts
    // ("startup freeze") until the user manually re-enables it or its
    // failures age out of the 30-day window.
    frozenInstancesSet().insert(base);
    qWarning() << "RSSHub instance frozen after exceeding monthly failures:"
               << base;
  }
  // Persist both the rolling failure window and the frozen set so the
  // monthly counter and the freeze survive application restarts.
  persistState();
  return frozen;
}

bool RssHubInstances::isFrozen(const QString &base)
{
  QMutexLocker locker(&mutex_);
  startupLoad();
  return frozenInstancesSet().contains(base);
}

void RssHubInstances::unfreezeInstance(const QString &base)
{
  QMutexLocker locker(&mutex_);
  startupLoad();
  bool changed = frozenInstancesSet().remove(base);
  failureTimestamps().remove(base);
  if (changed)
    persistState();
}

QSet<QString> &RssHubInstances::frozenInstancesSet()
{
  static QSet<QString> set;
  // std::call_once makes the lazy load safe even if the GUI thread and the
  // feed-update thread first touch this state concurrently.
  static std::once_flag flag;
  // MSVC rejects a simple capture of a static-storage variable (C3495), so
  // use the default reference capture instead.
  std::call_once(flag, [&]() {
    // Startup freeze: restore the frozen instances persisted by a previous
    // run so that instances over the monthly failure threshold stay disabled
    // after an application restart.
    QSettings settings;
    const QStringList stored =
        settings.value("RSSHub/frozenInstances").toStringList();
    foreach (const QString &base, stored) {
      QString normalized = normalizeBase(base);
      if (!normalized.isEmpty())
        set.insert(normalized);
    }
  });
  return set;
}

QHash<QString, QList<qint64>> &RssHubInstances::failureTimestamps()
{
  static QHash<QString, QList<qint64>> map;
  // std::call_once makes the lazy load safe even if the GUI thread and the
  // feed-update thread first touch this state concurrently.
  static std::once_flag flag;
  // See frozenInstancesSet(): MSVC rejects simple captures of statics (C3495).
  std::call_once(flag, [&]() {
    // Restore the persisted rolling failure window so that failures counted
    // in a previous run still contribute to the monthly threshold.
    QSettings settings;
    const QVariantMap stored =
        settings.value("RSSHub/failureTimestamps").toMap();
    QVariantMap::const_iterator it = stored.constBegin();
    for (; it != stored.constEnd(); ++it) {
      QString base = normalizeBase(it.key());
      if (base.isEmpty())
        continue;
      QList<qint64> times;
      foreach (const QVariant &v, it.value().toList()) {
        bool ok = false;
        const qint64 t = v.toLongLong(&ok);
        if (ok)
          times << t;
      }
      if (!times.isEmpty())
        map.insert(base, times);
    }
  });
  return map;
}

QStringList RssHubInstances::frozenInstances()
{
  QMutexLocker locker(&mutex_);
  startupLoad();
  return frozenInstancesSet().values();
}

void RssHubInstances::startupLoad()
{
  static bool done = false;
  if (done)
    return;
  done = true;
  frozenInstancesSet();    // ensure the persisted frozen set is loaded
  failureTimestamps();     // ensure the persisted failure window is loaded
  pruneExpired();          // unfreeze instances whose failures aged out
}

void RssHubInstances::pruneExpired()
{
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  QHash<QString, QList<qint64>> &map = failureTimestamps();
  QHash<QString, QList<qint64>>::iterator it = map.begin();
  bool mapChanged = false;
  while (it != map.end()) {
    QList<qint64> &list = it.value();
    const int before = list.size();
    list.erase(std::remove_if(list.begin(), list.end(),
               [now](qint64 t) { return (now - t) > MONTH_MS; }),
               list.end());
    if (list.size() != before)
      mapChanged = true;
    if (list.isEmpty()) {
      it = map.erase(it);
      mapChanged = true;
    } else {
      ++it;
    }
  }

  // An instance is frozen because it reached the monthly failure threshold.
  // Once its failures age out of the rolling 30-day window it becomes usable
  // again automatically.
  bool frozenChanged = false;
  QSet<QString> &frozen = frozenInstancesSet();
  foreach (const QString &base, frozen) {
    if (!map.contains(base) ||
        map.value(base).size() < MAX_FAILURES_PER_MONTH) {
      frozen.remove(base);
      frozenChanged = true;
    }
  }

  if (mapChanged || frozenChanged)
    persistState();

  if (!frozen.isEmpty()) {
    QStringList names = frozen.values();
    names.sort();
    qWarning() << "RSSHub frozen instances (not used for auto-swap):"
               << names;
  }
}

void RssHubInstances::persistState()
{
  // Only mark the state dirty. The actual settings write happens in
  // flushState() on the GUI thread, so the feed-update thread never blocks
  // on disk I/O.
  stateDirty_ = true;
}

void RssHubInstances::flushState()
{
  QMutexLocker locker(&mutex_);
  if (!stateDirty_)
    return;
  stateDirty_ = false;

  QSettings settings;
  settings.setValue("RSSHub/frozenInstances",
                    QStringList(frozenInstancesSet().values()));
  QVariantMap tsMap;
  const QHash<QString, QList<qint64>> &map = failureTimestamps();
  QHash<QString, QList<qint64>>::const_iterator it = map.constBegin();
  for (; it != map.constEnd(); ++it) {
    QVariantList times;
    foreach (qint64 t, it.value())
      times << QVariant(t);
    tsMap.insert(it.key(), times);
  }
  settings.setValue("RSSHub/failureTimestamps", tsMap);
  settings.sync();
}

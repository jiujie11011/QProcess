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
#ifndef RSSHUBINSTANCES_H
#define RSSHUBINSTANCES_H

#include <QMutex>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

/** @brief RSSHub public instance pool manager.
 *
 * Tracks the list of known RSSHub instances (editable in settings),
 * performs health checks, and can automatically swap a failed instance
 * to a healthy one when a feed update fails.
 */
class RssHubInstances
{
public:
    /** Default instance list used when no user list is saved. */
    static QStringList defaultInstances();

    /** Normalises a single instance base URL (trim, lower-case host, strip
     *  trailing slash/query/fragment). Returns an empty string for values
     *  that are not usable as an instance base. */
    static QString normalizeBase(const QString &raw);

    /** Instances stored in settings (falls back to defaults). Includes frozen
     *  instances — use loadActiveInstances() when you need candidates for
     *  swapping or health checks. */
    static QStringList loadInstances();

    /** Like loadInstances() but excludes frozen instances, so automatic
     *  instance switching never picks an instance that has exceeded the
     *  monthly failure threshold. */
    static QStringList loadActiveInstances();
    static void saveInstances(const QStringList &instances);

    static bool autoSwapEnabled();
    static void setAutoSwapEnabled(bool enabled);

    /** If url belongs to a known instance returns the instance base
     *  (e.g. "https://rsshub.app"), otherwise returns empty string. */
    static QString instanceOfUrl(const QString &url);

    /** Replaces the instance part of url with newBase. */
    static QString swapInstance(const QString &url, const QString &newBase);

    /** Synchronous health check (blocking, timeoutMs per instance). */
    static bool isAlive(const QString &base, int timeoutMs = 8000);

    /** Synchronous health check for a whole list; returns healthy bases. */
    static QStringList checkAlive(const QStringList &instances);

    /** Fetches an instance list from a remote URL (one base per line). */
    static QStringList fetchRemote(const QString &remoteUrl);

    /** Refreshes the cached healthy instance list (synchronous, blocking).
     *  Must be called from the GUI thread (e.g. the "Check Availability"
     *  button) and never from the feed update thread, because it performs
     *  blocking network requests. */
    static void updateHealthyCache();

    /** Refreshes the cached healthy instance list asynchronously. The blocking
     *  network probes run on a worker thread; the cache is updated on the GUI
     *  thread when they finish, so the UI stays responsive. Safe to call from
     *  the GUI thread (e.g. shortly after startup). */
    static void updateHealthyCacheAsync();

    /** Returns the cached healthy instance list, if available. */
    static QStringList cachedHealthy();

    /** Handles a failed feed update. When the feed belongs to an instance
     *  that has failed repeatedly, the feed URL is swapped to a healthy
     *  instance and the database is updated. Returns the new URL, or the
     *  original url when no swap is needed.
     *
     *  This method must be safe to call from the feed update thread: it
     *  performs NO blocking network requests, only uses the cached health
     *  information, and guards its static state with a mutex.
     *
     *  @param result  network result code from the update thread (see
     *                 requestfeed.cpp). Feed-side errors such as 404 or an
     *                 authentication requirement fail identically on every
     *                 instance, so they do not trigger a swap and are not
     *                 counted as instance failures.
     */
    static QString handleFeedFailure(int feedId, const QString &feedUrl,
                                     int result);

    /** Record a failure for an instance. Returns true if the instance has now
     *  exceeded the monthly threshold (5 failures in 30 days) and should be
     *  frozen. */
    static bool recordFailure(const QString &base);

    /** Check if an instance is frozen (exceeded monthly failure threshold). */
    static bool isFrozen(const QString &base);

    /** Get frozen instances list (for UI). */
    static QStringList frozenInstances();

    /** Unfreeze an instance (manual override). */
    static void unfreezeInstance(const QString &base);

    /** Persists the frozen set / failure timestamps that were marked dirty by
     *  update-thread calls. Safe and cheap to call periodically from the GUI
     *  thread (e.g. a timer) or on application exit. */
    static void flushState();

private:
    static QString pickHealthy(const QStringList &instances,
                               const QString &exceptBase);
    static QMutex mutex_;
    static QStringList healthyCache_;
    static bool healthyCacheValid_;

    /** Cached instance list (avoids re-reading the settings file on the
     *  feed-update thread for every failed request). */
    static QStringList cachedInstances_;
    static bool instancesCacheValid_;
    /** Set when frozen state / failure timestamps changed in memory but were
     *  not yet written to settings; cleared by flushState(). */
    static bool stateDirty_;

    /** Failure record: instance base -> list of failure timestamps (ms since
     *  epoch). Persisted to settings so the monthly window survives restarts. */
    static QHash<QString, QList<qint64>> &failureTimestamps();
    /** Frozen instances set (persisted to settings, loaded on startup). */
    static QSet<QString> &frozenInstancesSet();

    /** One-time startup initialisation: loads the persisted frozen-instance
     *  set and failure timestamps from settings, then prunes expired failure
     *  records so that instances whose failures have aged out of the 30-day
     *  window are unfrozen automatically. Must be called with mutex_ held. */
    static void startupLoad();

    /** Drops failure timestamps older than MONTH_MS and unfreezes instances
     *  whose remaining failures no longer reach the monthly threshold.
     *  Persists the state if anything changed. Must be called with mutex_
     *  held. */
    static void pruneExpired();

    /** Marks the frozen set / failure timestamps as dirty. The actual settings
     *  write is deferred to flushState() (GUI thread) so the feed-update
     *  thread never blocks on disk I/O. Must be called with mutex_ held. */
    static void persistState();

    /** getUrlDone() result codes (see requestfeed.cpp). Feed-side errors are
     *  never treated as instance failures. */
    static const int FAIL_OTHER = -1;     // DNS / TLS / timeout / refused
    static const int FAIL_AUTH = -2;      // server requires authentication
    static const int FAIL_REDIRECT = -4;  // redirect loop / error
    static const int FAIL_NOT_FOUND = -5; // HTTP 404

    static const int MAX_FAILURES_PER_MONTH = 5;
    // 30 days in ms overflows a 32-bit int (2,592,000,000 > INT_MAX), so the
    // constant must be 64-bit. It is compared against qint64 timestamps.
    static const qint64 MONTH_MS = qint64(30) * 24 * 60 * 60 * 1000;
    /** Concurrent failures of the same instance within this window count as
     *  one failure (dedup). */
    static const int FAILURE_DEDUP_MS = 5 * 60 * 1000;
};

#endif // RSSHUBINSTANCES_H

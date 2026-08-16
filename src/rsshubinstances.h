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

    /** Instances stored in settings (falls back to defaults). */
    static QStringList loadInstances();
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

    /** Returns the cached healthy instance list, if available. */
    static QStringList cachedHealthy();

    /** Handles a failed feed update. When the feed belongs to an instance
     *  that has failed repeatedly, the feed URL is swapped to a healthy
     *  instance and the database is updated. Returns the new URL, or the
     *  original url when no swap is needed.
     *
     *  This method must be safe to call from the feed update thread: it
     *  performs NO blocking network requests, only uses the cached health
     *  information, and guards its static state with a mutex. */
    /** Determine if a feed URL should be swapped to a healthy instance.
     *
     *  This is a PURE function — NO database access, NO blocking I/O.
     *  Returns the new feed URL (with instance part replaced), or an empty
     *  string if no swap is needed/possible. The caller (on the update thread)
     *  must emit a signal to the GUI thread to persist the change.
     */
    static QString handleFeedFailure(int feedId, const QString &feedUrl);

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

private:
    static QString pickHealthy(const QStringList &instances,
                               const QString &exceptBase);
    static QMutex mutex_;
    static QStringList healthyCache_;
    static bool healthyCacheValid_;

    /** Failure record: instance base -> list of failure timestamps (ms since epoch). */
    static QHash<QString, QList<qint64>> &failureTimestamps();
    /** Frozen instances set (persisted to settings). */
    static QSet<QString> &frozenInstancesSet();
    static const int MAX_FAILURES_PER_MONTH = 5;
    static const int MONTH_MS = 30 * 24 * 60 * 60 * 1000;
};

#endif // RSSHUBINSTANCES_H

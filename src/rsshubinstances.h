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
    static QString handleFeedFailure(int feedId, const QString &feedUrl,
                                     QSqlDatabase db);

private:
    static QString pickHealthy(const QStringList &instances,
                               const QString &exceptBase);
    static QMutex mutex_;
    static QStringList healthyCache_;
    static bool healthyCacheValid_;
};

#endif // RSSHUBINSTANCES_H

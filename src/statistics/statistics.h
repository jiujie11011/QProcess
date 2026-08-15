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
#ifndef STATISTICS_H
#define STATISTICS_H

#include <QObject>
#include <QSqlDatabase>
#include <QDate>
#include <QStringList>

/*! Event types stored in the stats table. */
namespace StatType {
  const char *const FeedRefresh = "feed_refresh";
  const char *const NewsRead    = "news_read";
  const char *const NewsView    = "news_view";
  const char *const NewsStar    = "news_star";
  const char *const AIChat      = "ai_chat";
  const char *const AISummary   = "ai_summary";
}

/*! Aggregated statistics accessor.
 *
 * Stores counters in the stats table keyed by (date, type) and provides
 * aggregated queries for the statistics dialog plus CSV/JSON export.
 */
class StatisticsService
{
public:
  explicit StatisticsService(QSqlDatabase db);

  /*! Increment counter for the given event type on the given date.
   *  Defaults to today. */
  void addEvent(const QString &type, const QDate &date = QDate());

  /*! Sum of counters for the given type between fromDate and toDate
   *  (inclusive). If type is empty, sums all types. */
  int count(const QString &type, const QDate &fromDate, const QDate &toDate) const;

  /*! Per-day counts for the given type between fromDate and toDate.
   *  Returns a map dateString(YYYY-MM-DD) -> count. */
  QMap<QString, int> dailyCounts(const QString &type,
                                 const QDate &fromDate, const QDate &toDate) const;

  /*! Total for all types between fromDate and toDate. */
  int total(const QDate &fromDate, const QDate &toDate) const;

  /*! Types that have at least one record between fromDate and toDate. */
  QStringList availableTypes(const QDate &fromDate, const QDate &toDate) const;

  /*! Export aggregated rows as CSV. */
  QString toCsv(const QDate &fromDate, const QDate &toDate) const;

  /*! Export aggregated rows as JSON. */
  QString toJson(const QDate &fromDate, const QDate &toDate) const;

private:
  QSqlDatabase db_;
};

#endif // STATISTICS_H

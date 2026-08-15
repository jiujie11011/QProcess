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
#ifndef INTELLIGENTREFRESHCALCULATOR_H
#define INTELLIGENTREFRESHCALCULATOR_H

#include <QDateTime>
#include <QSqlDatabase>

/*! Optimal refresh interval calculator (port of MrRSS intelligent_refresh).
 *
 *  Computes a feed's ideal auto-update interval from the recent article
 *  publication frequency: refresh twice as often as the average time
 *  between articles, clamped to [5 minutes, 24 hours].
 */
class IntelligentRefreshCalculator
{
public:
  explicit IntelligentRefreshCalculator(const QSqlDatabase &db);

  /*! Optimal interval (in seconds) for \a feedId, based on the last
   *  \a sampleSize articles' publication times (default 100). */
  int calculateInterval(int feedId, int sampleSize = 100) const;

  static const int MinIntervalSec = 5 * 60;       // 5 minutes
  static const int MaxIntervalSec = 24 * 60 * 60; // 24 hours
  static const int DefaultIntervalSec = 30 * 60;  // 30 minutes

private:
  QSqlDatabase db_;
};

#endif // INTELLIGENTREFRESHCALCULATOR_H

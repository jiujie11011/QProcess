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
#include "intelligentrefreshcalculator.h"

#include <QSqlQuery>
#include <QVariant>
#include <QList>
#include <QtMath>

IntelligentRefreshCalculator::IntelligentRefreshCalculator(const QSqlDatabase &db)
  : db_(db)
{
}

int IntelligentRefreshCalculator::calculateInterval(int feedId, int sampleSize) const
{
  QSqlQuery q(db_);
  q.setForwardOnly(true);
  q.prepare("SELECT published FROM news "
            "WHERE feedId=? AND deleted=0 AND published!='' "
            "ORDER BY published DESC LIMIT ?");
  q.addBindValue(feedId);
  q.addBindValue(sampleSize);
  if (!q.exec())
    return DefaultIntervalSec;

  QList<QDateTime> times;
  while (q.next()) {
    QDateTime dt = QDateTime::fromString(q.value(0).toString(),
                                         "yyyy-MM-ddTHH:mm:ss");
    if (dt.isValid())
      times.append(dt);
  }

  if (times.count() < 2)
    return DefaultIntervalSec;

  // Average interval between consecutive publications (sorted DESC).
  qint64 totalMs = 0;
  int validIntervals = 0;
  for (int i = 0; i < times.count() - 1; ++i) {
    qint64 interval = times.at(i).toMSecsSinceEpoch() -
                      times.at(i + 1).toMSecsSinceEpoch();
    if (interval > 0) {
      totalMs += interval;
      ++validIntervals;
    }
  }

  if (validIntervals == 0)
    return DefaultIntervalSec;

  qint64 avgMs = totalMs / validIntervals;
  qint64 optimalMs = avgMs / 2; // refresh twice as often as publication rate

  int optimalSec = (int)qRound((double)optimalMs / 1000.0);
  if (optimalSec < MinIntervalSec)
    return MinIntervalSec;
  if (optimalSec > MaxIntervalSec)
    return MaxIntervalSec;
  return optimalSec;
}

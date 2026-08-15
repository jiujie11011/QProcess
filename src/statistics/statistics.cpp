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
#include "statistics.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

// ----------------------------------------------------------------------------
StatisticsService::StatisticsService(QSqlDatabase db)
  : db_(db)
{
}

// ----------------------------------------------------------------------------
void StatisticsService::addEvent(const QString &type, const QDate &date)
{
  if (type.isEmpty()) return;
  QString dateStr = (date.isValid() ? date : QDate::currentDate()).
      toString(Qt::ISODate);

  QSqlQuery q(db_);
  q.prepare("INSERT INTO stats(date, type, count) VALUES(?, ?, 1)");
  q.addBindValue(dateStr);
  q.addBindValue(type);
  if (q.exec()) return;

  // Row already exists - increment
  q.prepare("UPDATE stats SET count = count + 1 "
            "WHERE date = ? AND type = ?");
  q.addBindValue(dateStr);
  q.addBindValue(type);
  if (!q.exec())
    qDebug() << "StatisticsService::addEvent error:" << q.lastError().text();
}

// ----------------------------------------------------------------------------
int StatisticsService::count(const QString &type,
                             const QDate &fromDate, const QDate &toDate) const
{
  QSqlQuery q(db_);
  if (type.isEmpty()) {
    q.prepare("SELECT COALESCE(SUM(count), 0) FROM stats "
              "WHERE date >= ? AND date <= ?");
  } else {
    q.prepare("SELECT COALESCE(SUM(count), 0) FROM stats "
              "WHERE date >= ? AND date <= ? AND type = ?");
    q.addBindValue(type);
  }
  q.addBindValue(fromDate.toString(Qt::ISODate));
  q.addBindValue(toDate.toString(Qt::ISODate));
  q.exec();
  if (q.next()) return q.value(0).toInt();
  return 0;
}

// ----------------------------------------------------------------------------
QMap<QString, int> StatisticsService::dailyCounts(const QString &type,
                                                  const QDate &fromDate,
                                                  const QDate &toDate) const
{
  QMap<QString, int> result;
  QSqlQuery q(db_);
  if (type.isEmpty()) {
    q.prepare("SELECT date, SUM(count) FROM stats "
              "WHERE date >= ? AND date <= ? GROUP BY date");
  } else {
    q.prepare("SELECT date, SUM(count) FROM stats "
              "WHERE date >= ? AND date <= ? AND type = ? GROUP BY date");
    q.addBindValue(type);
  }
  q.addBindValue(fromDate.toString(Qt::ISODate));
  q.addBindValue(toDate.toString(Qt::ISODate));
  q.exec();
  while (q.next())
    result.insert(q.value(0).toString(), q.value(1).toInt());
  return result;
}

// ----------------------------------------------------------------------------
int StatisticsService::total(const QDate &fromDate, const QDate &toDate) const
{
  return count(QString(), fromDate, toDate);
}

// ----------------------------------------------------------------------------
QStringList StatisticsService::availableTypes(const QDate &fromDate,
                                              const QDate &toDate) const
{
  QStringList result;
  QSqlQuery q(db_);
  q.prepare("SELECT DISTINCT type FROM stats "
            "WHERE date >= ? AND date <= ? ORDER BY type");
  q.addBindValue(fromDate.toString(Qt::ISODate));
  q.addBindValue(toDate.toString(Qt::ISODate));
  q.exec();
  while (q.next())
    result << q.value(0).toString();
  return result;
}

// ----------------------------------------------------------------------------
QString StatisticsService::toCsv(const QDate &fromDate, const QDate &toDate) const
{
  QString csv = "date,type,count\n";
  QSqlQuery q(db_);
  q.prepare("SELECT date, type, count FROM stats "
            "WHERE date >= ? AND date <= ? ORDER BY date, type");
  q.addBindValue(fromDate.toString(Qt::ISODate));
  q.addBindValue(toDate.toString(Qt::ISODate));
  q.exec();
  while (q.next()) {
    csv += q.value(0).toString() + "," +
           q.value(1).toString() + "," +
           QString::number(q.value(2).toInt()) + "\n";
  }
  return csv;
}

// ----------------------------------------------------------------------------
QString StatisticsService::toJson(const QDate &fromDate, const QDate &toDate) const
{
  QJsonArray rows;
  QSqlQuery q(db_);
  q.prepare("SELECT date, type, count FROM stats "
            "WHERE date >= ? AND date <= ? ORDER BY date, type");
  q.addBindValue(fromDate.toString(Qt::ISODate));
  q.addBindValue(toDate.toString(Qt::ISODate));
  q.exec();
  while (q.next()) {
    QJsonObject row;
    row.insert("date", q.value(0).toString());
    row.insert("type", q.value(1).toString());
    row.insert("count", q.value(2).toInt());
    rows.append(row);
  }
  QJsonObject root;
  root.insert("type", "quiterss-stats");
  root.insert("from", fromDate.toString(Qt::ISODate));
  root.insert("to", toDate.toString(Qt::ISODate));
  root.insert("rows", rows);
  return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

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
#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QDate>

class QLabel;
class QComboBox;
class QTreeWidget;
class StatisticsService;

/*! Statistics dialog: shows aggregated counters for each event type
 *  within a selectable time range, plus CSV/JSON export. */
class StatisticsDialog : public QDialog
{
  Q_OBJECT
public:
  explicit StatisticsDialog(QWidget *parent, QSqlDatabase db);
  ~StatisticsDialog();

private slots:
  void updateStatistics();
  void exportCsv();
  void exportJson();

private:
  QDate rangeStart() const;
  QDate rangeEnd() const;

  QSqlDatabase db_;
  StatisticsService *service_;
  QComboBox *rangeCombo_;
  QTreeWidget *statsTree_;
  QLabel *totalLabel_;
};

#endif // STATISTICSDIALOG_H

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
#ifndef KEYWORDTRENDSDIALOG_H
#define KEYWORDTRENDSDIALOG_H

#include <QDialog>
#include <QList>

class QComboBox;
class QDateTimeEdit;
class QTableWidget;
class QPushButton;
class QLineEdit;

/*! Keyword trend analysis.
 *
 * Local, AI-free: counts how often the given keywords occur in the titles /
 * descriptions of the articles of the selected feeds within a time range and
 * groups the counts by day so the user can spot rising / falling topics.
 */
class KeywordTrendsDialog : public QDialog
{
  Q_OBJECT
public:
  explicit KeywordTrendsDialog(QWidget *parent, const QList<int> &feedIds,
                               const QString &groupName);

private slots:
  void slotAnalyze();
  void slotTimeRangeChanged(int index);

private:
  struct KeywordDayCount {
    QString day;
    QString keyword;
    int mentions;
    int articles;
  };

  QList<int> feedIds_;
  QString groupName_;

  QLineEdit *keywordEdit_;
  QComboBox *timeRangeCombo_;
  QDateTimeEdit *fromEdit_;
  QDateTimeEdit *toEdit_;
  QTableWidget *table_;
  QPushButton *analyzeButton_;
};

#endif // KEYWORDTRENDSDIALOG_H

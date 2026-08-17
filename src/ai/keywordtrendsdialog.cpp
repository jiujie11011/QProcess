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
#include "keywordtrendsdialog.h"

#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDebug>
#include <QHash>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

// ----------------------------------------------------------------------------
KeywordTrendsDialog::KeywordTrendsDialog(QWidget *parent,
                                         const QList<int> &feedIds,
                                         const QString &groupName)
  : QDialog(parent)
  , feedIds_(feedIds)
  , groupName_(groupName)
{
  setWindowTitle(tr("Keyword trends in \"%1\"").arg(groupName_));
  setMinimumSize(760, 500);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  QHBoxLayout *keywordLayout = new QHBoxLayout;
  keywordLayout->addWidget(new QLabel(tr("Keywords (comma separated):")));
  keywordEdit_ = new QLineEdit;
  keywordEdit_->setPlaceholderText(tr("e.g. AI, Qt, security"));
  keywordLayout->addWidget(keywordEdit_);
  mainLayout->addLayout(keywordLayout);

  QHBoxLayout *rangeLayout = new QHBoxLayout;
  rangeLayout->addWidget(new QLabel(tr("Time range:")));
  timeRangeCombo_ = new QComboBox;
  timeRangeCombo_->addItem(tr("Last 7 days"), 0);
  timeRangeCombo_->addItem(tr("Last 30 days"), 1);
  timeRangeCombo_->addItem(tr("Last 90 days"), 2);
  timeRangeCombo_->addItem(tr("All time"), 3);
  timeRangeCombo_->addItem(tr("Custom"), 4);
  rangeLayout->addWidget(timeRangeCombo_);

  fromEdit_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-7));
  fromEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
  fromEdit_->setCalendarPopup(true);
  fromEdit_->setEnabled(false);
  rangeLayout->addWidget(new QLabel(tr("From:")));
  rangeLayout->addWidget(fromEdit_);

  toEdit_ = new QDateTimeEdit(QDateTime::currentDateTime());
  toEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
  toEdit_->setCalendarPopup(true);
  toEdit_->setEnabled(false);
  rangeLayout->addWidget(new QLabel(tr("To:")));
  rangeLayout->addWidget(toEdit_);
  rangeLayout->addStretch();
  mainLayout->addLayout(rangeLayout);

  table_ = new QTableWidget;
  table_->setColumnCount(4);
  table_->setHorizontalHeaderLabels(QStringList()
      << tr("Day") << tr("Keyword") << tr("Mentions") << tr("Articles"));
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->setAlternatingRowColors(true);
  mainLayout->addWidget(table_);

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  analyzeButton_ = new QPushButton(tr("Analyze"));
  QPushButton *closeButton = new QPushButton(tr("Close"));
  buttonLayout->addWidget(analyzeButton_);
  buttonLayout->addStretch();
  buttonLayout->addWidget(closeButton);
  mainLayout->addLayout(buttonLayout);

  connect(analyzeButton_, SIGNAL(clicked()), this, SLOT(slotAnalyze()));
  connect(timeRangeCombo_, SIGNAL(currentIndexChanged(int)),
          this, SLOT(slotTimeRangeChanged(int)));
  connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
}

// ----------------------------------------------------------------------------
void KeywordTrendsDialog::slotTimeRangeChanged(int index)
{
  bool custom = (timeRangeCombo_->itemData(index).toInt() == 4);
  fromEdit_->setEnabled(custom);
  toEdit_->setEnabled(custom);
}

// ----------------------------------------------------------------------------
void KeywordTrendsDialog::slotAnalyze()
{
  QStringList keywords;
  foreach (const QString &k,
           keywordEdit_->text().split(',', Qt::SkipEmptyParts))
    keywords << k.trimmed();
  keywords.removeAll(QString());
  if (keywords.isEmpty()) {
    table_->setRowCount(0);
    return;
  }

  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isOpen() || feedIds_.isEmpty())
    return;

  QString placeholders;
  QStringList binds;
  foreach (int id, feedIds_) {
    if (!placeholders.isEmpty())
      placeholders += ",";
    placeholders += "?";
    binds << QString::number(id);
  }

  int range = timeRangeCombo_->currentData().toInt();
  QString from;
  QString to;
  if (range == 4) {
    from = fromEdit_->dateTime().toString(Qt::ISODate);
    to = toEdit_->dateTime().toString(Qt::ISODate);
  } else {
    QDateTime now = QDateTime::currentDateTime();
    if (range == 0) from = now.addDays(-7).toString(Qt::ISODate);
    else if (range == 1) from = now.addDays(-30).toString(Qt::ISODate);
    else if (range == 2) from = now.addDays(-90).toString(Qt::ISODate);
    to = now.toString(Qt::ISODate);
  }

  QHash<QString, KeywordDayCount> counts;   // "day|keyword" -> stats
  foreach (const QString &kw, keywords) {
    QString sql = "SELECT date(received), title, description FROM news "
                  "WHERE feedId IN (" + placeholders + ") AND deleted==0";
    QStringList kwBinds = binds;
    if (!from.isEmpty()) {
      sql += " AND received >= ?";
      kwBinds << from;
    }
    if (!to.isEmpty()) {
      sql += " AND received <= ?";
      kwBinds << to;
    }

    QSqlQuery q(db);
    q.prepare(sql);
    foreach (const QString &b, kwBinds)
      q.addBindValue(b);
    if (!q.exec()) {
      qWarning() << "KeywordTrendsDialog::slotAnalyze error:"
                 << q.lastError().text();
      continue;
    }
    while (q.next()) {
      QString day = q.value(0).toString();
      QString title = q.value(1).toString();
      QString description = q.value(2).toString();
      int mentions = title.count(kw, Qt::CaseInsensitive)
                     + description.count(kw, Qt::CaseInsensitive);
      if (mentions <= 0)
        continue;
      QString key = day + "|" + kw;
      KeywordDayCount &c = counts[key];
      c.day = day;
      c.keyword = kw;
      c.mentions += mentions;
      ++c.articles;
    }
  }

  QList<KeywordDayCount> rows = counts.values();
  std::sort(rows.begin(), rows.end(), [](const KeywordDayCount &a,
                                         const KeywordDayCount &b) {
    if (a.day != b.day)
      return a.day > b.day;
    return a.keyword < b.keyword;
  });

  table_->setRowCount(rows.size());
  for (int i = 0; i < rows.size(); ++i) {
    const KeywordDayCount &c = rows.at(i);
    table_->setItem(i, 0, new QTableWidgetItem(c.day));
    table_->setItem(i, 1, new QTableWidgetItem(c.keyword));
    table_->setItem(i, 2, new QTableWidgetItem(QString::number(c.mentions)));
    table_->setItem(i, 3, new QTableWidgetItem(QString::number(c.articles)));
  }
}

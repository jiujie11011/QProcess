/* ============================================================
* Quill is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 Quill Team <quillteam@gmail.com>
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
#include "statisticsdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QHeaderView>

#include "statistics.h"

// ----------------------------------------------------------------------------
StatisticsDialog::StatisticsDialog(QWidget *parent, QSqlDatabase db)
  : QDialog(parent)
  , db_(db)
{
  service_ = new StatisticsService(db_);

  setWindowTitle(tr("Statistics"));
  setMinimumSize(560, 420);

  rangeCombo_ = new QComboBox();
  rangeCombo_->addItem(tr("Total"), 0);
  rangeCombo_->addItem(tr("Last 7 days"), 7);
  rangeCombo_->addItem(tr("Last 30 days"), 30);
  rangeCombo_->addItem(tr("Last 365 days"), 365);
  connect(rangeCombo_, SIGNAL(currentIndexChanged(int)),
          this, SLOT(updateStatistics()));

  QHBoxLayout *rangeLayout = new QHBoxLayout();
  rangeLayout->addWidget(new QLabel(tr("Range:")));
  rangeLayout->addWidget(rangeCombo_);
  rangeLayout->addStretch();

  totalLabel_ = new QLabel();
  totalLabel_->setStyleSheet("font-weight: bold;");

  statsTree_ = new QTreeWidget();
  statsTree_->setColumnCount(2);
  QStringList headers;
  headers << tr("Event") << tr("Count");
  statsTree_->setHeaderLabels(headers);
  statsTree_->setRootIsDecorated(false);
  statsTree_->header()->setStretchLastSection(false);
  statsTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);

  QPushButton *csvButton = new QPushButton(tr("Export CSV..."));
  connect(csvButton, SIGNAL(clicked()), this, SLOT(exportCsv()));
  QPushButton *jsonButton = new QPushButton(tr("Export JSON..."));
  connect(jsonButton, SIGNAL(clicked()), this, SLOT(exportJson()));

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QHBoxLayout *exportLayout = new QHBoxLayout();
  exportLayout->addWidget(csvButton);
  exportLayout->addWidget(jsonButton);
  exportLayout->addStretch();

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(rangeLayout);
  mainLayout->addWidget(totalLabel_);
  mainLayout->addWidget(statsTree_);
  mainLayout->addLayout(exportLayout);
  mainLayout->addWidget(buttonBox);

  updateStatistics();
}

// ----------------------------------------------------------------------------
StatisticsDialog::~StatisticsDialog()
{
  delete service_;
}

// ----------------------------------------------------------------------------
QDate StatisticsDialog::rangeStart() const
{
  int days = rangeCombo_->currentData().toInt();
  if (days <= 0)
    return QDate(2000, 1, 1);
  return QDate::currentDate().addDays(-(days - 1));
}

// ----------------------------------------------------------------------------
QDate StatisticsDialog::rangeEnd() const
{
  return QDate::currentDate();
}

// ----------------------------------------------------------------------------
void StatisticsDialog::updateStatistics()
{
  QDate from = rangeStart();
  QDate to = rangeEnd();

  statsTree_->clear();
  struct Entry { QString type; QString label; };
  Entry entries[] = {
    { StatType::FeedRefresh, tr("Feed refreshes") },
    { StatType::NewsRead,    tr("News read") },
    { StatType::NewsView,    tr("News views") },
    { StatType::NewsStar,    tr("News starred") },
    { StatType::AIChat,      tr("AI chats") },
    { StatType::AISummary,   tr("AI summaries") }
  };

  int grandTotal = 0;
  for (int i = 0; i < 6; i++) {
    int value = service_->count(entries[i].type, from, to);
    grandTotal += value;
    QTreeWidgetItem *item = new QTreeWidgetItem(statsTree_);
    item->setText(0, entries[i].label);
    item->setText(1, QString::number(value));
    item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
  }

  totalLabel_->setText(tr("Total events: %1").arg(grandTotal));
}

// ----------------------------------------------------------------------------
void StatisticsDialog::exportCsv()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Export Statistics CSV"),
                                                  QDir::homePath(),
                                                  tr("CSV-Files (*.csv)"));
  if (fileName.isNull()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, tr("Export"), tr("Can't open file for writing."));
    return;
  }
  file.write(service_->toCsv(rangeStart(), rangeEnd()).toUtf8());
  file.close();
}

// ----------------------------------------------------------------------------
void StatisticsDialog::exportJson()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Export Statistics JSON"),
                                                  QDir::homePath(),
                                                  tr("JSON-Files (*.json)"));
  if (fileName.isNull()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, tr("Export"), tr("Can't open file for writing."));
    return;
  }
  file.write(service_->toJson(rangeStart(), rangeEnd()).toUtf8());
  file.close();
}

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
#include "groupsummarydialog.h"

#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextBrowser>
#include <QTextEdit>
#include <QVBoxLayout>

#include "settings.h"

// ----------------------------------------------------------------------------
GroupSummaryDialog::GroupSummaryDialog(QWidget *parent, AIAssistant *assistant,
                                       const QList<int> &feedIds,
                                       const QString &groupName,
                                       bool onlyUnread, int initialRange)
  : QDialog(parent)
  , assistant_(assistant)
  , feedIds_(feedIds)
  , groupName_(groupName)
  , onlyUnread_(onlyUnread)
{
  setWindowTitle(onlyUnread_
                 ? tr("AI summary of unread in \"%1\"").arg(groupName_)
                 : tr("AI summary of \"%1\"").arg(groupName_));
  setMinimumWidth(640);
  setMinimumHeight(520);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // Time range
  QHBoxLayout *timeLayout = new QHBoxLayout;
  timeLayout->addWidget(new QLabel(tr("Time range:")));
  timeRangeCombo_ = new QComboBox;
  timeRangeCombo_->addItem(tr("Last 24 hours"), 0);
  timeRangeCombo_->addItem(tr("Last 7 days"), 1);
  timeRangeCombo_->addItem(tr("Last 30 days"), 2);
  timeRangeCombo_->addItem(tr("All time"), 3);
  timeRangeCombo_->addItem(tr("Custom"), 4);
  timeRangeCombo_->addItem(tr("Today"), 5);
  if (initialRange >= 0) {
    int idx = timeRangeCombo_->findData(initialRange);
    if (idx >= 0)
      timeRangeCombo_->setCurrentIndex(idx);
  }
  timeLayout->addWidget(timeRangeCombo_);

  fromEdit_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-7));
  fromEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
  fromEdit_->setCalendarPopup(true);
  fromEdit_->setEnabled(false);
  timeLayout->addWidget(new QLabel(tr("From:")));
  timeLayout->addWidget(fromEdit_);

  toEdit_ = new QDateTimeEdit(QDateTime::currentDateTime());
  toEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
  toEdit_->setCalendarPopup(true);
  toEdit_->setEnabled(false);
  timeLayout->addWidget(new QLabel(tr("To:")));
  timeLayout->addWidget(toEdit_);
  timeLayout->addStretch();
  mainLayout->addLayout(timeLayout);

  // Summary length
  QHBoxLayout *lengthLayout = new QHBoxLayout;
  lengthLayout->addWidget(new QLabel(tr("Summary length:")));
  lengthCombo_ = new QComboBox;
  lengthCombo_->addItem(tr("Short (~150 chars)"), 150);
  lengthCombo_->addItem(tr("Standard (~300 chars)"), 300);
  lengthCombo_->addItem(tr("Detailed (~600 chars)"), 600);
  lengthLayout->addWidget(lengthCombo_);
  lengthLayout->addStretch();
  mainLayout->addLayout(lengthLayout);

  // Prompt
  mainLayout->addWidget(new QLabel(tr("Prompt:")));
  promptEdit_ = new QTextEdit;
  promptEdit_->setPlaceholderText(tr("Custom instructions. "
      "Available placeholders: {count} {articles} {length} {from} {to}"));
  promptEdit_->setMaximumHeight(90);
  mainLayout->addWidget(promptEdit_);

  // Result
  mainLayout->addWidget(new QLabel(tr("Result:")));
  resultView_ = new QTextBrowser;
  resultView_->setOpenExternalLinks(true);
  mainLayout->addWidget(resultView_);

  // Buttons
  QHBoxLayout *buttonLayout = new QHBoxLayout;
  generateButton_ = new QPushButton(tr("Generate"));
  copyButton_ = new QPushButton(tr("Copy"));
  QPushButton *closeButton = new QPushButton(tr("Close"));
  buttonLayout->addWidget(generateButton_);
  buttonLayout->addWidget(copyButton_);
  buttonLayout->addStretch();
  buttonLayout->addWidget(closeButton);
  mainLayout->addLayout(buttonLayout);

  connect(timeRangeCombo_, SIGNAL(currentIndexChanged(int)),
          this, SLOT(slotTimeRangeChanged(int)));
  connect(generateButton_, SIGNAL(clicked()), this, SLOT(slotGenerate()));
  connect(copyButton_, SIGNAL(clicked()), this, SLOT(slotCopyResult()));
  connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

  if (assistant_) {
    connect(assistant_, SIGNAL(responseReady(QString)),
            this, SLOT(slotResponseReady(QString)));
    connect(assistant_, SIGNAL(requestFailed(QString)),
            this, SLOT(slotRequestFailed(QString)));
  }

  promptEdit_->setPlainText(defaultPromptTemplate());
}

// ----------------------------------------------------------------------------
void GroupSummaryDialog::slotTimeRangeChanged(int index)
{
  bool custom = (timeRangeCombo_->itemData(index).toInt() == 4);
  fromEdit_->setEnabled(custom);
  toEdit_->setEnabled(custom);
}

// ----------------------------------------------------------------------------
QString GroupSummaryDialog::resolveFrom() const
{
  int range = timeRangeCombo_->currentData().toInt();
  if (range == 3) return QString();                 // all time
  if (range == 4) return fromEdit_->dateTime().toString(Qt::ISODate);
  QDateTime now = QDateTime::currentDateTime();
  if (range == 5)                                  // today (from 00:00)
    return QDateTime(QDate::currentDate(), QTime(0, 0)).toString(Qt::ISODate);
  if (range == 0) return now.addSecs(-24 * 3600).toString(Qt::ISODate);
  if (range == 1) return now.addDays(-7).toString(Qt::ISODate);
  return now.addDays(-30).toString(Qt::ISODate);
}

// ----------------------------------------------------------------------------
QString GroupSummaryDialog::resolveTo() const
{
  if (timeRangeCombo_->currentData().toInt() == 4)
    return toEdit_->dateTime().toString(Qt::ISODate);
  return QDateTime::currentDateTime().toString(Qt::ISODate);
}

// ----------------------------------------------------------------------------
QList<GroupSummaryDialog::ArticleInfo>
GroupSummaryDialog::loadArticles(const QString &from, const QString &to) const
{
  QList<ArticleInfo> result;
  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isOpen() || feedIds_.isEmpty())
    return result;

  QString placeholders;
  QStringList binds;
  foreach (int id, feedIds_) {
    if (!placeholders.isEmpty())
      placeholders += ",";
    placeholders += "?";
    binds << QString::number(id);
  }

  QString sql = "SELECT id, title, link_href, published FROM news "
                "WHERE feedId IN (" + placeholders + ") AND deleted==0";
  if (onlyUnread_)
    sql += " AND read==0";
  if (!from.isEmpty())
    sql += " AND received >= ?";
  if (!to.isEmpty())
    sql += " AND received <= ?";
  sql += " ORDER BY received DESC LIMIT 200";

  QSqlQuery q(db);
  q.prepare(sql);
  foreach (const QString &b, binds)
    q.addBindValue(b);
  if (!from.isEmpty())
    q.addBindValue(from);
  if (!to.isEmpty())
    q.addBindValue(to);
  if (!q.exec()) {
    qWarning() << "GroupSummaryDialog::loadArticles error:" << q.lastError().text();
    return result;
  }
  while (q.next()) {
    ArticleInfo a;
    a.id = q.value(0).toInt();
    a.title = q.value(1).toString();
    a.link = q.value(2).toString();
    a.published = q.value(3).toString();
    result.append(a);
  }
  return result;
}

// ----------------------------------------------------------------------------
QString GroupSummaryDialog::buildArticleList(
    const QList<ArticleInfo> &articles) const
{
  QString list;
  foreach (const ArticleInfo &a, articles) {
    QString title = a.title.trimmed();
    if (title.isEmpty())
      title = tr("(untitled)");
    QString line = QString("- %1").arg(title);
    if (!a.link.isEmpty())
      line += QString(" (%2)").arg(a.link);
    if (!a.published.isEmpty())
      line += QString(" [%3]").arg(a.published);
    list += line + "\n";
  }
  return list;
}

// ----------------------------------------------------------------------------
QString GroupSummaryDialog::defaultPromptTemplate() const
{
  return tr("You are a news reading assistant. The following is a list of "
            "%1 articles collected from the subscription group \"%2\" between "
            "%3 and %4:\n\n%5\n\n"
            "Please summarize the main topics, key developments and notable "
            "changes covered by these articles in Chinese, output as a "
            "Markdown bullet list, within %6 characters.")
      .arg("{count}", groupName_, "{from}", "{to}", "{articles}", "{length}");
}

// ----------------------------------------------------------------------------
void GroupSummaryDialog::slotGenerate()
{
  if (!assistant_ || !assistant_->isConfigured()) {
    resultView_->setPlainText(tr("AI is not configured. "
        "Open Settings -> AI, enable the assistant and fill in the API key."));
    return;
  }

  QString from = resolveFrom();
  QString to = resolveTo();
  QList<ArticleInfo> articles = loadArticles(from, to);
  if (articles.isEmpty()) {
    resultView_->setPlainText(tr("No articles found in the selected period."));
    return;
  }

  QString prompt = promptEdit_->toPlainText();
  if (prompt.trimmed().isEmpty())
    prompt = defaultPromptTemplate();
  prompt.replace("{count}", QString::number(articles.count()));
  prompt.replace("{articles}", buildArticleList(articles));
  prompt.replace("{length}", lengthCombo_->currentData().toString());
  prompt.replace("{from}", from);
  prompt.replace("{to}", to);

  AIAssistant::ArticleContext context;
  context.category = groupName_;

  generateButton_->setEnabled(false);
  resultView_->setPlainText(tr("Generating..."));
  assistant_->sendMessage(prompt, context);
}

// ----------------------------------------------------------------------------
void GroupSummaryDialog::slotResponseReady(const QString &text)
{
  generateButton_->setEnabled(true);
  resultView_->setMarkdown(text);
}

// ----------------------------------------------------------------------------
void GroupSummaryDialog::slotRequestFailed(const QString &error)
{
  generateButton_->setEnabled(true);
  resultView_->setPlainText(tr("Failed: %1").arg(error));
}

// ----------------------------------------------------------------------------
void GroupSummaryDialog::slotCopyResult()
{
  resultView_->selectAll();
  resultView_->copy();
}

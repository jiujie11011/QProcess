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
#include "recommendationdialog.h"

#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSpinBox>
#include <QTextBrowser>
#include <QVBoxLayout>

// ----------------------------------------------------------------------------
RecommendationDialog::RecommendationDialog(QWidget *parent, AIAssistant *assistant,
                                           const QList<int> &feedIds)
  : QDialog(parent)
  , assistant_(assistant)
  , feedIds_(feedIds)
{
  setWindowTitle(tr("AI recommendations"));
  setMinimumWidth(640);
  setMinimumHeight(480);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  QHBoxLayout *sourceLayout = new QHBoxLayout;
  sourceLayout->addWidget(new QLabel(tr("Based on:")));
  sourceCombo_ = new QComboBox;
  sourceCombo_->addItem(tr("Recent unread articles"), 0);
  sourceCombo_->addItem(tr("Recently read articles"), 1);
  sourceCombo_->addItem(tr("Starred articles"), 2);
  sourceLayout->addWidget(sourceCombo_);

  sourceLayout->addWidget(new QLabel(tr("Limit:")));
  limitSpin_ = new QSpinBox;
  limitSpin_->setRange(5, 100);
  limitSpin_->setValue(20);
  sourceLayout->addWidget(limitSpin_);
  sourceLayout->addStretch();
  mainLayout->addLayout(sourceLayout);

  mainLayout->addWidget(new QLabel(tr("Recommendations:")));
  resultView_ = new QTextBrowser;
  resultView_->setOpenExternalLinks(true);
  mainLayout->addWidget(resultView_);

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  generateButton_ = new QPushButton(tr("Generate"));
  copyButton_ = new QPushButton(tr("Copy"));
  QPushButton *closeButton = new QPushButton(tr("Close"));
  buttonLayout->addWidget(generateButton_);
  buttonLayout->addWidget(copyButton_);
  buttonLayout->addStretch();
  buttonLayout->addWidget(closeButton);
  mainLayout->addLayout(buttonLayout);

  connect(generateButton_, SIGNAL(clicked()), this, SLOT(slotGenerate()));
  connect(copyButton_, SIGNAL(clicked()), this, SLOT(slotCopyResult()));
  connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

  if (assistant_) {
    connect(assistant_, SIGNAL(responseReady(QString)),
            this, SLOT(slotResponseReady(QString)));
    connect(assistant_, SIGNAL(requestFailed(QString)),
            this, SLOT(slotRequestFailed(QString)));
  }
}

// ----------------------------------------------------------------------------
QList<RecommendationDialog::ArticleInfo>
RecommendationDialog::loadRecentArticles(int limit) const
{
  QList<ArticleInfo> result;
  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isOpen() || feedIds_.isEmpty())
    return result;

  int source = sourceCombo_->currentData().toInt();
  QString where = (source == 0) ? "read==0"
                 : (source == 1) ? "read==1"
                 : "starred==1";

  QString placeholders;
  QStringList binds;
  foreach (int id, feedIds_) {
    if (!placeholders.isEmpty())
      placeholders += ",";
    placeholders += "?";
    binds << QString::number(id);
  }

  QString sql = "SELECT id, title, link_href, published FROM news "
                "WHERE feedId IN (" + placeholders + ") AND deleted==0"
                " AND " + where +
                " ORDER BY received DESC LIMIT " + QString::number(limit);

  QSqlQuery q(db);
  q.prepare(sql);
  foreach (const QString &b, binds)
    q.addBindValue(b);
  if (!q.exec()) {
    qWarning() << "RecommendationDialog::loadRecentArticles error:"
               << q.lastError().text();
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
QString RecommendationDialog::buildArticleList(
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
QString RecommendationDialog::buildPrompt(
    const QList<ArticleInfo> &articles) const
{
  int source = sourceCombo_->currentData().toInt();
  QString sourceName = (source == 0) ? tr("recent unread")
                       : (source == 1) ? tr("recently read")
                       : tr("starred");
  return tr("You are a personal news recommendation assistant. The following "
            "is a list of %1 %2 articles the user is interested in:\n\n"
            "%3\n\n"
            "Based on these interests, recommend 5-8 topics or specific "
            "articles the user should follow next. Answer in Chinese as a "
            "Markdown list, each item with a short reason.")
      .arg(QString::number(articles.count()), sourceName,
           buildArticleList(articles));
}

// ----------------------------------------------------------------------------
void RecommendationDialog::slotGenerate()
{
  if (!assistant_ || !assistant_->isConfigured()) {
    resultView_->setPlainText(tr("AI is not configured. "
        "Open Settings -> AI, enable the assistant and fill in the API key."));
    return;
  }

  QList<ArticleInfo> articles = loadRecentArticles(limitSpin_->value());
  if (articles.isEmpty()) {
    resultView_->setPlainText(tr("No articles found for the selected source."));
    return;
  }

  QString prompt = buildPrompt(articles);
  AIAssistant::ArticleContext context;
  context.category = tr("recommendations");

  generateButton_->setEnabled(false);
  resultView_->setPlainText(tr("Generating..."));
  assistant_->sendMessage(prompt, context);
}

// ----------------------------------------------------------------------------
void RecommendationDialog::slotResponseReady(const QString &text)
{
  generateButton_->setEnabled(true);
  resultView_->setMarkdown(text);
}

// ----------------------------------------------------------------------------
void RecommendationDialog::slotRequestFailed(const QString &error)
{
  generateButton_->setEnabled(true);
  resultView_->setPlainText(tr("Failed: %1").arg(error));
}

// ----------------------------------------------------------------------------
void RecommendationDialog::slotCopyResult()
{
  resultView_->selectAll();
  resultView_->copy();
}

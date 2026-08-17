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
#include "aidialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QListWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlQuery>
#include <QDesktopServices>

// ----------------------------------------------------------------------------
AIDialog::AIDialog(QWidget *parent, AIAssistant *assistant)
  : QDialog(parent)
  , assistant_(assistant)
{
  setWindowTitle(tr("AI Assistant"));
  setMinimumSize(720, 520);

  connect(assistant_, SIGNAL(responseReady(QString)),
          this, SLOT(slotResponseReady(QString)));
  connect(assistant_, SIGNAL(requestFailed(QString)),
          this, SLOT(slotRequestFailed(QString)));

  conversationList_ = new QListWidget();
  conversationList_->setMaximumWidth(220);
  conversationList_->setMinimumWidth(160);

  chatView_ = new QTextBrowser();
  chatView_->setOpenExternalLinks(false);
  chatView_->setStyleSheet("QTextBrowser { border: 1px solid palette(mid); }");
  connect(chatView_, SIGNAL(anchorClicked(QUrl)),
          this, SLOT(slotAnchorClicked(QUrl)));

  summaryButton_ = new QPushButton(tr("Summarize article"));
  recommendButton_ = new QPushButton(tr("Recommend related"));
  connect(summaryButton_, SIGNAL(clicked()), this, SLOT(requestSummary()));
  connect(recommendButton_, SIGNAL(clicked()), this, SLOT(requestRecommendations()));

  inputEdit_ = new QTextEdit();
  inputEdit_->setMaximumHeight(80);
  inputEdit_->setPlaceholderText(tr("Ask a question about the article..."));

  sendButton_ = new QPushButton(tr("Send"));
  connect(sendButton_, SIGNAL(clicked()), this, SLOT(sendMessage()));

  QHBoxLayout *shortcutLayout = new QHBoxLayout();
  shortcutLayout->addWidget(summaryButton_);
  shortcutLayout->addWidget(recommendButton_);
  shortcutLayout->addStretch();

  QHBoxLayout *inputLayout = new QHBoxLayout();
  inputLayout->addWidget(inputEdit_, 1);
  inputLayout->addWidget(sendButton_);

  QVBoxLayout *rightLayout = new QVBoxLayout();
  rightLayout->addWidget(chatView_, 1);
  rightLayout->addLayout(shortcutLayout);
  rightLayout->addLayout(inputLayout);

  QSplitter *splitter = new QSplitter(Qt::Horizontal);
  splitter->addWidget(conversationList_);
  splitter->addWidget(new QWidget());

  splitter->widget(1)->setLayout(rightLayout);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(splitter, 1);
  mainLayout->addWidget(buttonBox);

  loadHistory();
}

// ----------------------------------------------------------------------------
void AIDialog::setArticleContext(const AIAssistant::ArticleContext &context)
{
  context_ = context;
  loadRecommendations();
}

// ----------------------------------------------------------------------------
void AIDialog::loadHistory()
{
  chatView_->clear();
  conversationList_->clear();

  QList<QJsonObject> history = assistant_->loadHistory(
        context_.feedId, context_.newsId);
  for (const QJsonObject &item : history) {
    QString role = item.value("role").toString();
    QString content = item.value("content").toString();
    appendMessage(role, content);
  }
}

// ----------------------------------------------------------------------------
void AIDialog::loadRecommendations()
{
  if (context_.newsId <= 0) return;

  QSqlQuery q;
  q.prepare("SELECT date, content FROM recommendations "
            "WHERE newsId=? ORDER BY id DESC LIMIT 1");
  q.addBindValue(context_.newsId);
  if (!q.exec() || !q.next())
    return;

  QString content = q.value(1).toString();
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &err);
  if (err.error != QJsonParseError::NoError)
    return;
  QJsonArray arr = doc.array();
  if (arr.isEmpty())
    return;

  chatView_->append(QString("<b>%1</b>").arg(tr("Related articles:")));
  for (const QJsonValue &value : arr) {
    QJsonObject item = value.toObject();
    QString title = item.value("title").toString();
    QString link = item.value("link").toString();
    if (title.isEmpty())
      continue;
    if (link.isEmpty())
      chatView_->append(QString("&bull; %1").arg(title.toHtmlEscaped()));
    else
      chatView_->append(QString("&bull; <a href=\"%1\">%2</a>")
                        .arg(link.toHtmlEscaped(), title.toHtmlEscaped()));
  }
  chatView_->append("");
}

// ----------------------------------------------------------------------------
void AIDialog::slotAnchorClicked(const QUrl &url)
{
  if (url.isEmpty() || !url.isValid())
    return;
  QDesktopServices::openUrl(url);
}

// ----------------------------------------------------------------------------
void AIDialog::appendMessage(const QString &role, const QString &text)
{
  if (text.isEmpty()) return;

  QString name = (role == "user")
      ? tr("You") : tr("Assistant");
  chatView_->append(QString("<b>%1:</b>").arg(name));
  chatView_->append(text.toHtmlEscaped());
  chatView_->append("");
}

// ----------------------------------------------------------------------------
void AIDialog::sendMessage()
{
  QString text = inputEdit_->toPlainText().trimmed();
  if (text.isEmpty()) return;
  inputEdit_->clear();

  appendMessage("user", text);
  assistant_->sendMessage(text, context_);
}

// ----------------------------------------------------------------------------
void AIDialog::slotResponseReady(const QString &text)
{
  appendMessage("assistant", text);
}

// ----------------------------------------------------------------------------
void AIDialog::slotRequestFailed(const QString &error)
{
  chatView_->append(QString("<i style=\"color:red\">%1</i>")
                    .arg(error.toHtmlEscaped()));
  chatView_->append("");
}

// ----------------------------------------------------------------------------
void AIDialog::requestSummary()
{
  appendMessage("user", tr("[Summarize current article]"));
  assistant_->requestSummary(context_);
}

// ----------------------------------------------------------------------------
void AIDialog::requestRecommendations()
{
  appendMessage("user", tr("[Recommend related content]"));
  assistant_->requestRecommendations(context_);
}

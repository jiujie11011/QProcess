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
#include "autotagdialog.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSpinBox>
#include <QTextBrowser>
#include <QVBoxLayout>

// ----------------------------------------------------------------------------
AutoTagDialog::AutoTagDialog(QWidget *parent, AIAssistant *assistant,
                             const QList<int> &feedIds,
                             const QString &groupName)
  : QDialog(parent)
  , assistant_(assistant)
  , feedIds_(feedIds)
  , groupName_(groupName)
{
  setWindowTitle(tr("AI auto labeling of \"%1\"").arg(groupName_));
  setMinimumSize(640, 480);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  QHBoxLayout *limitLayout = new QHBoxLayout;
  limitLayout->addWidget(new QLabel(tr("Number of recent articles to analyze:")));
  limitSpin_ = new QSpinBox;
  limitSpin_->setRange(5, 100);
  limitSpin_->setValue(20);
  limitLayout->addWidget(limitSpin_);
  limitLayout->addStretch();
  mainLayout->addLayout(limitLayout);

  mainLayout->addWidget(new QLabel(tr("Suggested labels (from AI):")));
  resultView_ = new QTextBrowser;
  resultView_->setOpenExternalLinks(false);
  mainLayout->addWidget(resultView_);

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  generateButton_ = new QPushButton(tr("Generate"));
  applyButton_ = new QPushButton(tr("Apply to unlabeled articles"));
  copyButton_ = new QPushButton(tr("Copy"));
  QPushButton *closeButton = new QPushButton(tr("Close"));
  applyButton_->setEnabled(false);
  buttonLayout->addWidget(generateButton_);
  buttonLayout->addWidget(applyButton_);
  buttonLayout->addWidget(copyButton_);
  buttonLayout->addStretch();
  buttonLayout->addWidget(closeButton);
  mainLayout->addLayout(buttonLayout);

  connect(generateButton_, SIGNAL(clicked()), this, SLOT(slotGenerate()));
  connect(applyButton_, SIGNAL(clicked()), this, SLOT(slotApply()));
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
QString AutoTagDialog::buildPrompt() const
{
  QSqlDatabase db = QSqlDatabase::database();
  if (!db.isOpen() || feedIds_.isEmpty())
    return QString();

  QString placeholders;
  QStringList binds;
  foreach (int id, feedIds_) {
    if (!placeholders.isEmpty())
      placeholders += ",";
    placeholders += "?";
    binds << QString::number(id);
  }

  QSqlQuery q(db);
  q.prepare("SELECT title FROM news "
            "WHERE feedId IN (" + placeholders + ") AND deleted==0 "
            "ORDER BY received DESC LIMIT " +
            QString::number(limitSpin_->value()));
  foreach (const QString &b, binds)
    q.addBindValue(b);
  if (!q.exec()) {
    qWarning() << "AutoTagDialog::buildPrompt error:" << q.lastError().text();
    return QString();
  }

  QStringList titles;
  while (q.next()) {
    QString t = q.value(0).toString().trimmed();
    if (!t.isEmpty())
      titles << t;
  }
  if (titles.isEmpty())
    return QString();

  return tr("You are an article classification assistant. Analyze the "
            "following %1 article titles from the subscription \"%2\":\n\n"
            "%3\n\n"
            "Suggest 5-8 short category labels (each 1-4 words, comma "
            "separated) that best describe the common topics. Answer in "
            "Chinese with a single line of labels only, e.g. "
            "\"科技, 人工智能, 产品\".")
      .arg(QString::number(titles.size()), groupName_, titles.join("\n"));
}

// ----------------------------------------------------------------------------
void AutoTagDialog::slotGenerate()
{
  if (!assistant_ || !assistant_->isConfigured()) {
    resultView_->setPlainText(tr("AI is not configured. "
        "Open Settings -> AI, enable the assistant and fill in the API key."));
    return;
  }

  QString prompt = buildPrompt();
  if (prompt.isEmpty()) {
    resultView_->setPlainText(tr("No articles found to analyze."));
    return;
  }

  AIAssistant::ArticleContext context;
  context.category = groupName_;

  generateButton_->setEnabled(false);
  applyButton_->setEnabled(false);
  resultView_->setPlainText(tr("Generating..."));
  assistant_->sendMessage(prompt, context);
}

// ----------------------------------------------------------------------------
void AutoTagDialog::slotResponseReady(const QString &text)
{
  generateButton_->setEnabled(true);
  applyButton_->setEnabled(true);
  resultView_->setPlainText(text);
}

// ----------------------------------------------------------------------------
void AutoTagDialog::slotRequestFailed(const QString &error)
{
  generateButton_->setEnabled(true);
  applyButton_->setEnabled(false);
  resultView_->setPlainText(tr("Failed: %1").arg(error));
}

// ----------------------------------------------------------------------------
void AutoTagDialog::slotApply()
{
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

  QString labels = resultView_->toPlainText().trimmed();
  if (labels.isEmpty())
    return;

  if (QMessageBox::question(this, tr("Apply labels"),
        tr("Write the suggested labels to all currently unlabeled articles "
           "of this group?")) != QMessageBox::Yes)
    return;

  QSqlQuery q(db);
  q.prepare("UPDATE news SET label=? WHERE feedId IN (" + placeholders +
            ") AND deleted==0 AND (label IS NULL OR label=='')");
  q.addBindValue(labels);
  foreach (const QString &b, binds)
    q.addBindValue(b);
  if (!q.exec())
    qWarning() << "AutoTagDialog::slotApply error:" << q.lastError().text();
  else
    QMessageBox::information(this, tr("Apply labels"),
                             tr("Labels have been applied."));
}

// ----------------------------------------------------------------------------
void AutoTagDialog::slotCopyResult()
{
  resultView_->selectAll();
  resultView_->copy();
}

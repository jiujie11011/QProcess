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
#ifndef AIDIALOG_H
#define AIDIALOG_H

#include <QDialog>

#include "aiassistant.h"

class QListWidget;
class QTextBrowser;
class QTextEdit;
class QPushButton;

/*! AI assistant chat window.
 *
 * Left: local conversation list. Right: message history, input box and
 * shortcut buttons ("Summarize current article", "Recommend related").
 */
class AIDialog : public QDialog
{
  Q_OBJECT
public:
  explicit AIDialog(QWidget *parent, AIAssistant *assistant);

  /*! Set the current article context used for context injection. */
  void setArticleContext(const AIAssistant::ArticleContext &context);

private slots:
  void sendMessage();
  void slotResponseReady(const QString &text);
  void slotRequestFailed(const QString &error);
  void requestSummary();
  void requestRecommendations();
  void slotAnchorClicked(const QUrl &url);

private:
  void appendMessage(const QString &role, const QString &text);
  void loadHistory();
  void loadRecommendations();

  AIAssistant *assistant_;
  AIAssistant::ArticleContext context_;

  QListWidget *conversationList_;
  QTextBrowser *chatView_;
  QTextEdit *inputEdit_;
  QPushButton *sendButton_;
  QPushButton *summaryButton_;
  QPushButton *recommendButton_;
};

#endif // AIDIALOG_H

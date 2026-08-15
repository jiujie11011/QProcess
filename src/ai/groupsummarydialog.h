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
#ifndef GROUPSUMMARYDIALOG_H
#define GROUPSUMMARYDIALOG_H

#include <QDialog>
#include <QList>

#include "aiassistant.h"

class QComboBox;
class QDateTimeEdit;
class QTextBrowser;
class QTextEdit;
class QPushButton;

/*! AI summary of all articles of a feed group within a time range.
 *
 * Collects the article titles/links of the given feeds published/received
 * in the selected period, lets the user edit the prompt, and delivers the
 * result through AIAssistant (OpenAI-compatible API).
 */
class GroupSummaryDialog : public QDialog
{
  Q_OBJECT
public:
  explicit GroupSummaryDialog(QWidget *parent, AIAssistant *assistant,
                              const QList<int> &feedIds, const QString &groupName);

private slots:
  void slotGenerate();
  void slotCopyResult();
  void slotTimeRangeChanged(int index);
  void slotResponseReady(const QString &text);
  void slotRequestFailed(const QString &error);

private:
  struct ArticleInfo {
    int id;
    QString title;
    QString link;
    QString published;
  };

  QList<ArticleInfo> loadArticles(const QString &from, const QString &to) const;
  QString buildArticleList(const QList<ArticleInfo> &articles) const;
  QString defaultPromptTemplate() const;
  QString resolveFrom() const;
  QString resolveTo() const;

  AIAssistant *assistant_;
  QList<int> feedIds_;
  QString groupName_;

  QComboBox *timeRangeCombo_;
  QDateTimeEdit *fromEdit_;
  QDateTimeEdit *toEdit_;
  QTextEdit *promptEdit_;
  QComboBox *lengthCombo_;
  QTextBrowser *resultView_;
  QPushButton *generateButton_;
  QPushButton *copyButton_;
};

#endif // GROUPSUMMARYDIALOG_H

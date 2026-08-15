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
#ifndef RECOMMENDATIONDIALOG_H
#define RECOMMENDATIONDIALOG_H

#include <QDialog>
#include <QList>

#include "aiassistant.h"

class QComboBox;
class QSpinBox;
class QTextBrowser;
class QPushButton;

/*! AI content recommendation dialog.
 *
 * Collects the article titles/links that the user recently read or starred,
 * and asks the AI which new topics / articles to follow. This is a "stub"
 * implementation: the recommendation logic is delegated to the configured
 * OpenAI-compatible endpoint.
 */
class RecommendationDialog : public QDialog
{
  Q_OBJECT
public:
  explicit RecommendationDialog(QWidget *parent, AIAssistant *assistant,
                                const QList<int> &feedIds);

private slots:
  void slotGenerate();
  void slotCopyResult();
  void slotResponseReady(const QString &text);
  void slotRequestFailed(const QString &error);

private:
  struct ArticleInfo {
    int id;
    QString title;
    QString link;
    QString published;
  };

  QList<ArticleInfo> loadRecentArticles(int limit) const;
  QString buildArticleList(const QList<ArticleInfo> &articles) const;
  QString buildPrompt(const QList<ArticleInfo> &articles) const;

  AIAssistant *assistant_;
  QList<int> feedIds_;

  QComboBox *sourceCombo_;
  QSpinBox *limitSpin_;
  QTextBrowser *resultView_;
  QPushButton *generateButton_;
  QPushButton *copyButton_;
};

#endif // RECOMMENDATIONDIALOG_H

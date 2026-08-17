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
#ifndef AUTOTAGDIALOG_H
#define AUTOTAGDIALOG_H

#include <QDialog>
#include <QList>

#include "aiassistant.h"

class QLabel;
class QSpinBox;
class QTextBrowser;
class QPushButton;

/*! Auto classification / labeling dialog.
 *
 * Collects the titles of the most recent articles of the selected feeds,
 * asks the AI to suggest a set of category labels, and lets the user write
 * the chosen labels back into the news rows.
 */
class AutoTagDialog : public QDialog
{
  Q_OBJECT
public:
  explicit AutoTagDialog(QWidget *parent, AIAssistant *assistant,
                         const QList<int> &feedIds,
                         const QString &groupName);

private slots:
  void slotGenerate();
  void slotApply();
  void slotCopyResult();
  void slotResponseReady(const QString &text);
  void slotRequestFailed(const QString &error);

private:
  QString buildPrompt() const;

  AIAssistant *assistant_;
  QList<int> feedIds_;
  QString groupName_;

  QSpinBox *limitSpin_;
  QTextBrowser *resultView_;
  QPushButton *generateButton_;
  QPushButton *applyButton_;
  QPushButton *copyButton_;
};

#endif // AUTOTAGDIALOG_H

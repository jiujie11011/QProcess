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
#ifndef BLOCKEDWORDSDIALOG_H
#define BLOCKEDWORDSDIALOG_H

#include "dialog.h"

class QListWidget;
class QLineEdit;

/** @brief Manager for the global "blocked words" list.
 *
 * Articles whose title or content contains any configured word are hidden
 * from the news list (non-destructive: removing the word brings them back).
 * The list is stored in the blockedWords table (DB v24) and the WHERE clause
 * is injected by NewsModel::setFilter(), so every news list is affected.
 *---------------------------------------------------------------------------*/
class BlockedWordsDialog : public Dialog
{
  Q_OBJECT
public:
  explicit BlockedWordsDialog(QWidget *parent);

private slots:
  void addWord();
  void removeWord();
  void clearWords();

private:
  void loadWords();
  void saveWords();

  QListWidget *wordsList_;
  QLineEdit *wordEdit_;
};

#endif // BLOCKEDWORDSDIALOG_H

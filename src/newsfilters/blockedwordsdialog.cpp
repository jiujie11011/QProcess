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
#include "blockedwordsdialog.h"

#include "mainapplication.h"
#include "database.h"
#include "newstabwidget.h"

BlockedWordsDialog::BlockedWordsDialog(QWidget *parent)
  : Dialog(parent, Qt::WindowMinMaxButtonsHint)
{
  setWindowTitle(tr("Blocked Words"));
  setMinimumWidth(440);
  setMinimumHeight(380);

  QLabel *tipLabel = new QLabel(
        tr("Articles whose title or content contains any of these words are "
           "hidden from the news list. Removing a word brings the articles "
           "back - nothing is deleted."), this);
  tipLabel->setWordWrap(true);

  wordsList_ = new QListWidget(this);
  wordsList_->setSelectionMode(QAbstractItemView::SingleSelection);

  wordEdit_ = new QLineEdit(this);
  wordEdit_->setPlaceholderText(tr("New word..."));
  QPushButton *addButton = new QPushButton(tr("Add"), this);
  connect(addButton, SIGNAL(clicked()), this, SLOT(addWord()));
  connect(wordEdit_, SIGNAL(returnPressed()), this, SLOT(addWord()));

  QHBoxLayout *addLayout = new QHBoxLayout();
  addLayout->setMargin(0);
  addLayout->addWidget(wordEdit_, 1);
  addLayout->addWidget(addButton);

  QPushButton *removeButton = new QPushButton(tr("Remove selected"), this);
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeWord()));
  QPushButton *clearButton = new QPushButton(tr("Clear all"), this);
  connect(clearButton, SIGNAL(clicked()), this, SLOT(clearWords()));

  QHBoxLayout *editLayout = new QHBoxLayout();
  editLayout->setMargin(0);
  editLayout->addWidget(removeButton);
  editLayout->addWidget(clearButton);
  editLayout->addStretch();

  pageLayout->addWidget(tipLabel);
  pageLayout->addWidget(wordsList_, 1);
  pageLayout->addLayout(addLayout);
  pageLayout->addLayout(editLayout);

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

  loadWords();
  wordEdit_->setFocus();
}

void BlockedWordsDialog::loadWords()
{
  wordsList_->clear();
  const QStringList words = Database::blockedWords();
  foreach (const QString &word, words) {
    QListWidgetItem *item = new QListWidgetItem(word, wordsList_);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
  }
}

void BlockedWordsDialog::addWord()
{
  const QString word = wordEdit_->text().trimmed();
  if (word.isEmpty())
    return;
  // Avoid duplicates (case-insensitive compare of the displayed words).
  for (int i = 0; i < wordsList_->count(); ++i) {
    if (wordsList_->item(i)->text().compare(word, Qt::CaseInsensitive) == 0) {
      wordsList_->setCurrentRow(i);
      return;
    }
  }
  QListWidgetItem *item = new QListWidgetItem(word, wordsList_);
  item->setFlags(item->flags() & ~Qt::ItemIsEditable);
  wordsList_->setCurrentItem(item);
  wordEdit_->clear();
  wordEdit_->setFocus();
}

void BlockedWordsDialog::removeWord()
{
  QListWidgetItem *item = wordsList_->currentItem();
  if (!item)
    return;
  const int row = wordsList_->row(item);
  delete wordsList_->takeItem(row);
  if (row < wordsList_->count())
    wordsList_->setCurrentRow(row);
  else if (wordsList_->count() > 0)
    wordsList_->setCurrentRow(wordsList_->count() - 1);
}

void BlockedWordsDialog::clearWords()
{
  wordsList_->clear();
}

void BlockedWordsDialog::saveWords()
{
  QSqlDatabase db = Database::connection();
  db.transaction();

  QSqlQuery q(db);
  q.exec("DELETE FROM blockedWords");
  for (int i = 0; i < wordsList_->count(); ++i) {
    const QString word = wordsList_->item(i)->text().trimmed();
    if (word.isEmpty())
      continue;
    q.prepare("INSERT INTO blockedWords(word) VALUES(?)");
    q.addBindValue(word);
    q.exec();
  }

  db.commit();

  // Invalidate the model-level cache; the next setFilter() rebuilds the
  // WHERE clause with the new list.
  Database::reloadBlockedWords();

  // Refresh the visible news list so hiding/unhiding takes effect now.
  MainWindow *mainWindow = mainApp->mainWindow();
  if (mainWindow)
    mainWindow->slotUpdateNews(NewsTabWidget::RefreshAll);
}

void BlockedWordsDialog::accept()
{
  saveWords();
  Dialog::accept();
}

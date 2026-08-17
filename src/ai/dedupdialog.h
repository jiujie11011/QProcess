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
#ifndef DEDUPDIALOG_H
#define DEDUPDIALOG_H

#include <QDialog>
#include <QList>

class QLabel;
class QTableWidget;
class QPushButton;

/*! Duplicate article detector.
 *
 * Local, AI-free: scans the selected feeds for articles with the same
 * normalized link or the same normalized title and presents the duplicates
 * in a table so the user can mark them as read or delete them.
 */
class DedupDialog : public QDialog
{
  Q_OBJECT
public:
  explicit DedupDialog(QWidget *parent, const QList<int> &feedIds,
                       const QString &groupName);

private slots:
  void slotScan();
  void slotMarkSelectedRead();
  void slotDeleteSelected();
  void slotClose();

private:
  struct ArticleInfo {
    int id;
    int feedId;
    QString title;
    QString link;
    QString published;
    QString feedName;
  };

  QString normalize(const QString &text) const;
  QList<ArticleInfo> loadArticles() const;
  void fillTable(const QList<ArticleInfo> &articles);

  QList<int> feedIds_;
  QString groupName_;

  QLabel *infoLabel_;
  QTableWidget *table_;
  QPushButton *scanButton_;
  QPushButton *markReadButton_;
  QPushButton *deleteButton_;
  QPushButton *closeButton_;
};

#endif // DEDUPDIALOG_H

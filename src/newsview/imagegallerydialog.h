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
#ifndef IMAGEGALLERYDIALOG_H
#define IMAGEGALLERYDIALOG_H

#include <QDialog>

class QListWidget;
class QNetworkAccessManager;
class QLabel;

/*! Image gallery for the current article.
 *
 * Extracts <img> URLs from the article HTML, shows them in an icon-grid
 * list and opens a full-screen viewer on double click.
 */
class ImageGalleryDialog : public QDialog
{
  Q_OBJECT
public:
  /*! Extract image URLs from \a contentHtml and show the gallery. */
  explicit ImageGalleryDialog(QWidget *parent, const QString &contentHtml);

private slots:
  void slotItemDoubleClicked();

private:
  void slotImageLoadedWithData(int index, const QByteArray &data);

  QListWidget *grid_;
  QNetworkAccessManager *network_;
  QList<QString> urls_;
  QLabel *viewer_;
};

#endif // IMAGEGALLERYDIALOG_H

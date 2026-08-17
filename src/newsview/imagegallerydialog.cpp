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
#include "imagegallerydialog.h"

#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QDialog>
#include <QScreen>
#include <QApplication>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <QMessageBox>

#include "qzregexp.h"

// ----------------------------------------------------------------------------
ImageGalleryDialog::ImageGalleryDialog(QWidget *parent, const QString &contentHtml)
  : QDialog(parent)
{
  setWindowTitle(tr("Image Gallery"));
  setMinimumSize(640, 480);

  network_ = new QNetworkAccessManager(this);

  grid_ = new QListWidget();
  grid_->setViewMode(QListView::IconMode);
  grid_->setIconSize(QSize(160, 160));
  grid_->setResizeMode(QListView::Adjust);
  grid_->setMovement(QListView::Static);
  grid_->setUniformItemSizes(true);
  grid_->setSpacing(6);
  connect(grid_, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
          this, SLOT(slotItemDoubleClicked()));

  // Extract image URLs from the article HTML.
  QzRegExp reg("<img[^>]+src=['\"]?([^'\"\\s>]+)['\"]?[^>]*>", Qt::CaseInsensitive);
  int pos = 0;
  while ((pos = reg.indexIn(contentHtml, pos)) != -1) {
    QString src = reg.cap(1).trimmed();
    if (!src.isEmpty()) {
      src = src.replace("&amp;", "&");
      if (!urls_.contains(src))
        urls_.append(src);
    }
    pos += reg.matchedLength();
  }

  QPushButton *closeButton = new QPushButton(tr("Close"));
  connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));

  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(closeButton);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(grid_, 1);
  mainLayout->addLayout(buttonLayout);

  if (urls_.isEmpty()) {
    QMessageBox::information(this, tr("Image Gallery"),
                             tr("No images found in this article."));
    return;
  }

  for (int i = 0; i < urls_.size(); ++i) {
    QListWidgetItem *item = new QListWidgetItem(QIcon(), tr("Loading..."));
    item->setSizeHint(QSize(170, 170));
    item->setData(Qt::UserRole, i);
    grid_->addItem(item);
  }

  // Kick off async loads.
  for (int i = 0; i < urls_.size(); ++i) {
    QNetworkRequest req(QUrl(urls_.at(i)));
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "Mozilla/5.0 (QuietRSS)");
    QNetworkReply *reply = network_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, i]() {
      QByteArray data = reply->readAll();
      reply->deleteLater();
      slotImageLoadedWithData(i, data);
    });
  }
}

// ----------------------------------------------------------------------------
void ImageGalleryDialog::slotImageLoadedWithData(int index, const QByteArray &data)
{
  QPixmap pm;
  if (!pm.loadFromData(data)) {
    QListWidgetItem *item = grid_->item(index);
    if (item) item->setText(tr("(error)"));
    return;
  }
  QListWidgetItem *item = grid_->item(index);
  if (item) {
    item->setIcon(QIcon(pm.scaled(QSize(160, 160), Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation)));
    item->setText(QString());
  }
}

// ----------------------------------------------------------------------------
void ImageGalleryDialog::slotItemDoubleClicked()
{
  QListWidgetItem *item = grid_->currentItem();
  if (!item) return;
  int index = item->data(Qt::UserRole).toInt();
  if (index < 0 || index >= urls_.size()) return;

  viewer_ = new QLabel();
  viewer_->setWindowFlags(Qt::Window);
  viewer_->setAlignment(Qt::AlignCenter);
  viewer_->setStyleSheet("background: black;");
  viewer_->setText(tr("Loading..."));

  QNetworkReply *reply = network_->get(QNetworkRequest(QUrl(urls_.at(index))));
  connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
    QByteArray data = reply->readAll();
    reply->deleteLater();
    QPixmap pm;
    if (pm.loadFromData(data)) {
      QScreen *screen = QApplication::primaryScreen();
      QRect geo = screen ? screen->geometry() : QRect(0, 0, 800, 600);
      viewer_->setPixmap(pm.scaled(geo.size(), Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation));
      viewer_->show();
    } else {
      viewer_->setText(tr("Failed to load image"));
      viewer_->show();
    }
  });
}

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
#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <QObject>
#include <QHash>
#include <QUrl>
#include <QStringList>
#include <QtSql>

class QNetworkAccessManager;
class QNetworkReply;

/** @brief Offline image cache for articles.
 *
 * When an article is starred, its <img> URLs are downloaded asynchronously
 * (one at a time) into <dataDir>/images/<newsId>/ and the article HTML is
 * rewritten so every <img src> points at the local file. The rewritten HTML
 * is stored in news_ex (name='cachedContent') and can be rendered fully
 * offline; failed downloads keep their original URL as fallback.
 */
class ImageCacheManager : public QObject
{
  Q_OBJECT
public:
  explicit ImageCacheManager(QObject *parent = 0);

  /** Parse \a html for <img src>, download them into the news cache folder
   *  and persist the rewritten HTML when all downloads finished. */
  void cacheNewsImages(int newsId, int feedId,
                       const QString &html, const QUrl &baseUrl);

  /** Cached rewritten HTML for \a newsId, or empty if not cached. */
  static QString cachedContent(int newsId, const QSqlDatabase &db);

  /** Remove the image cache folder of one article. */
  static void removeNewsImages(int newsId);

  /** Remove cache folders whose news id no longer exists in the database. */
  static void cleanupOrphans(const QSet<int> &validNewsIds);

  static QString imagesRootDir();
  static QString newsImagesDir(int newsId);

signals:
  void contentReady(int newsId, const QString &cachedHtml);

private slots:
  void slotImageDownloaded(QNetworkReply *reply);

private:
  struct CacheJob {
    int newsId;
    int feedId;
    QString html;
    QList<QPair<QString, QString> > pendingUrls; // (original src, absolute URL)
    QStringList failedUrls; // original src of downloads that failed
  };

  bool extractImageUrls(const QString &html, const QUrl &baseUrl,
                        CacheJob *job);
  void startNextJob();
  void rewriteAndStore(int newsId, int feedId, const QString &html);

  QNetworkAccessManager *nam_;
  QList<CacheJob> queue_;
  QNetworkReply *activeReply_;
  QString activeSrc_; // original src text of the in-flight download
};

#endif // IMAGECACHE_H

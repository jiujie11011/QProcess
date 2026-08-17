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
#include "imagecache.h"

#include "mainapplication.h"
#include "networkmanager.h"
#include "settings.h"
#include "common.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <qzregexp.h>

namespace {
const char *kCachedContentName = "cachedContent";
}

ImageCacheManager::ImageCacheManager(QObject *parent)
  : QObject(parent)
  , nam_(0)
  , activeReply_(0)
{
}

QString ImageCacheManager::imagesRootDir()
{
  return mainApp->dataDir() + "/images";
}

QString ImageCacheManager::newsImagesDir(int newsId)
{
  return QString("%1/%2").arg(imagesRootDir()).arg(newsId);
}

/** @brief Queue a news article for image caching (deduplicated)
 *---------------------------------------------------------------------------*/
void ImageCacheManager::cacheNewsImages(int newsId, int feedId,
                                        const QString &html, const QUrl &baseUrl)
{
  if (newsId <= 0 || html.isEmpty())
    return;

  // Skip if the same news is already queued.
  for (int i = 0; i < queue_.size(); ++i) {
    if (queue_.at(i).newsId == newsId)
      return;
  }

  CacheJob job;
  job.newsId = newsId;
  job.feedId = feedId;
  job.html = html;

  if (!extractImageUrls(html, baseUrl, &job)) {
    // Nothing downloadable: still persist the raw HTML so offline mode
    // shows the article even when the DB has been cleaned.
    rewriteAndStore(newsId, feedId, html);
    return;
  }

  queue_.append(job);
  if (!activeReply_)
    startNextJob();
}

/** @brief Extract <img src="..."> entries from the article HTML
 *
 * Returns false when there is nothing to download. \a job->pendingUrls is
 * filled with (original src text, absolute URL) pairs. The original src text
 * is kept so the HTML rewrite later can replace the exact substring even when
 * the markup uses a relative path.
 *---------------------------------------------------------------------------*/
bool ImageCacheManager::extractImageUrls(const QString &html, const QUrl &baseUrl,
                                         CacheJob *job)
{
  QzRegExp reg("<img[^>]+src\\s*=\\s*[\"']([^\"']+)[\"'][^>]*>",
               Qt::CaseInsensitive);

  QStringList seen;
  int offset = 0;
  while ((offset = reg.indexIn(html, offset)) != -1) {
    const QString src = reg.cap(1).trimmed();
    offset += reg.matchedLength();

    if (src.isEmpty() || seen.contains(src))
      continue;
    seen.append(src);

    const QUrl resolved = baseUrl.isValid()
        ? baseUrl.resolved(QUrl(src)) : QUrl(src);
    if (!resolved.isValid() ||
        (resolved.scheme() != "http" && resolved.scheme() != "https"))
      continue;

    job->pendingUrls.append(qMakePair(src, resolved.toString()));
  }

  return !job->pendingUrls.isEmpty();
}

/** @brief Start downloading the next queued job
 *---------------------------------------------------------------------------*/
void ImageCacheManager::startNextJob()
{
  if (queue_.isEmpty() || activeReply_)
    return;

  CacheJob &job = queue_.first();
  const QPair<QString, QString> entry = job.pendingUrls.takeFirst();
  const QString src = entry.first;
  const QUrl url(entry.second);

  if (!nam_)
    nam_ = mainApp->networkManager();

  QNetworkRequest request(url);
  activeSrc_ = src;
  activeReply_ = nam_->get(request);
  connect(activeReply_, SIGNAL(finished()), this, SLOT(slotImageDownloaded()));
}

/** @brief One image download finished
 *---------------------------------------------------------------------------*/
void ImageCacheManager::slotImageDownloaded()
{
  QNetworkReply *reply = activeReply_;
  activeReply_ = 0;
  if (!reply)
    return;

  const QString src = activeSrc_;
  activeSrc_.clear();
  const QNetworkReply::NetworkError err = reply->error();
  const QByteArray data = reply->readAll();
  reply->deleteLater();

  CacheJob job = queue_.takeFirst();
  if ((err == QNetworkReply::NoError) && !data.isEmpty()) {
    const QUrl url = reply->request().url();
    QString suffix = QFileInfo(url.path()).suffix().toLower();
    if (suffix.isEmpty())
      suffix = QStringLiteral("img");

    QDir dir(newsImagesDir(job.newsId));
    if (!dir.exists())
      dir.mkpath(".");

    const QString filePath = QString("%1/img_%2.%3").
        arg(newsImagesDir(job.newsId)).
        arg(job.pendingUrls.count() + job.failedUrls.count() + 1).
        arg(suffix);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
      file.write(data);
      file.close();
      job.html.replace(src, QUrl::fromLocalFile(filePath).toString());
    }
  } else {
    job.failedUrls.append(src);
  }

  if (!job.pendingUrls.isEmpty()) {
    queue_.prepend(job);
    startNextJob();
  } else {
    rewriteAndStore(job.newsId, job.feedId, job.html);
  }
}

/** @brief Persist the rewritten HTML to news_ex and notify the tab
 *---------------------------------------------------------------------------*/
void ImageCacheManager::rewriteAndStore(int newsId, int feedId,
                                        const QString &html)
{
  if (newsId <= 0 || html.isEmpty())
    return;

  QSqlDatabase db = QSqlDatabase::database();
  QSqlQuery q(db);
  q.prepare("UPDATE news_ex SET value=:value, feedId=:feedId "
            "WHERE newsId=:newsId AND name=:name");
  q.bindValue(":value", html);
  q.bindValue(":feedId", feedId);
  q.bindValue(":newsId", newsId);
  q.bindValue(":name", QString::fromLatin1(kCachedContentName));
  q.exec();
  if (q.numRowsAffected() == 0) {
    QSqlQuery qi(db);
    qi.prepare("INSERT INTO news_ex(feedId, newsId, name, value) "
               "VALUES(:feedId, :newsId, :name, :value)");
    qi.bindValue(":feedId", feedId);
    qi.bindValue(":newsId", newsId);
    qi.bindValue(":name", QString::fromLatin1(kCachedContentName));
    qi.bindValue(":value", html);
    qi.exec();
  }

  emit contentReady(newsId, html);
}

/** @brief Read the cached rewritten HTML of one article
 *---------------------------------------------------------------------------*/
QString ImageCacheManager::cachedContent(int newsId, const QSqlDatabase &db)
{
  if (newsId <= 0)
    return QString();

  QSqlQuery q(db);
  q.prepare("SELECT value FROM news_ex "
            "WHERE newsId=? AND name=?");
  q.addBindValue(newsId);
  q.addBindValue(QString::fromLatin1(kCachedContentName));
  q.exec();
  if (q.next())
    return q.value(0).toString();
  return QString();
}

/** @brief Remove the image cache folder of one article
 *---------------------------------------------------------------------------*/
void ImageCacheManager::removeNewsImages(int newsId)
{
  if (newsId <= 0)
    return;
  Common::removePath(newsImagesDir(newsId));
}

/** @brief Delete cache folders whose article no longer exists
 *---------------------------------------------------------------------------*/
void ImageCacheManager::cleanupOrphans(const QSet<int> &validNewsIds)
{
  QDir root(imagesRootDir());
  if (!root.exists())
    return;

  foreach (const QFileInfo &info,
           root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    bool ok = false;
    const int newsId = info.fileName().toInt(&ok);
    if (!ok)
      continue;
    if (!validNewsIds.contains(newsId))
      Common::removePath(info.absoluteFilePath());
  }
}

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
#include "webpluginfactory.h"

#include "clicktoflash.h"
#include "mainapplication.h"
#include "adblockmanager.h"

#include <QWebEngineUrlRequestInfo>
#include <QStringList>

namespace {
// Hotlink-protection referer whitelist, borrowed from RSSNext/Folo
// (packages/internal/utils/src/img-proxy.ts). These image hosts require a
// specific third-party referer to serve the picture; the generic "use the
// image host itself" fallback below would still be blocked.
struct RefererRule
{
  QString hostSuffix;
  QByteArray referer;
};
const QList<RefererRule> kImageRefererRules = {
  // *.sinaimg.cn -> weibo.com
  {QStringLiteral("sinaimg.cn"), QByteArrayLiteral("https://weibo.com/")},
  // i.pximg.net -> pixiv.net
  {QStringLiteral("pximg.net"), QByteArrayLiteral("https://www.pixiv.net/")},
  // cdnfile.sspai.com -> sspai.com
  {QStringLiteral("sspai.com"), QByteArrayLiteral("https://sspai.com/")},
  // *.cdninstagram.com -> instagram.com
  {QStringLiteral("cdninstagram.com"), QByteArrayLiteral("https://www.instagram.com/")},
  // sp1.piokok.com -> piokok.com
  {QStringLiteral("piokok.com"), QByteArrayLiteral("https://www.piokok.com/")},
  // *.xhscdn.com -> xiaohongshu.com
  {QStringLiteral("xhscdn.com"), QByteArrayLiteral("https://www.xiaohongshu.com/")},
};

QByteArray refererForHost(const QString &hostLower)
{
  for (const RefererRule &rule : kImageRefererRules) {
    if (hostLower == rule.hostSuffix ||
        hostLower.endsWith(QLatin1Char('.') + rule.hostSuffix))
      return rule.referer;
  }
  return QByteArray();
}
} // namespace

WebPluginFactory::WebPluginFactory(QObject *parent)
  : QWebEngineUrlRequestInterceptor(parent)
{
}

void WebPluginFactory::interceptRequest(QWebEngineUrlRequestInfo &info)
{
  QUrl url = info.requestUrl();

  if (url.isEmpty())
    return;

  AdBlockManager* manager = AdBlockManager::instance();
  if (manager->isEnabled()) {
    QNetworkRequest request(url);
    if (manager->isBlocked(request)) {
      info.block(true);
      return;
    }
  }

  QString path = url.path().toLower();
  if (path.endsWith(".swf")) {
    if (!mainApp->c2fIsEnabled()) {
      info.block(true);
      return;
    }

    QStringList whitelist = mainApp->c2fGetWhitelist();
    QString host = url.host();
    if (whitelist.contains(host) ||
        whitelist.contains("www." + host) ||
        whitelist.contains(QString(host).remove(QLatin1String("www.")))) {
      return;
    }

    if (ClickToFlash::isAlreadyAccepted(url)) {
      return;
    }

    info.block(true);
  }

  // Smart Referer: RSS readers commonly trip hotlink protection because the
  // news page is rendered from a different origin. Same-domain resources keep
  // the original Referer; third-party resources (images/media on a CDN) use
  // their own host as Referer. Decisions are cached per image host.
  if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeImage ||
      info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMedia) {
    QString imageHost = url.host().toLower();
    if (!imageHost.isEmpty()) {
      QString refHost = info.firstPartyUrl().host().toLower();
      if (refHost.isEmpty() ||
          (imageHost != refHost &&
           !imageHost.endsWith("." + refHost) &&
           !refHost.endsWith("." + imageHost))) {
        // Prefer a hotlink-protection referer for known image CDNs, then fall
        // back to the cached "use the image host itself" strategy.
        QByteArray referer = refererForHost(imageHost);
        if (referer.isEmpty()) {
          QHash<QString, QByteArray>::const_iterator it = refererCache_.constFind(imageHost);
          if (it != refererCache_.constEnd()) {
            referer = it.value();
          } else {
            referer = url.scheme().toUtf8() + "://" + imageHost.toUtf8() + "/";
            if (refererCache_.size() > 512)
              refererCache_.clear();
            refererCache_.insert(imageHost, referer);
          }
        }
        info.setHttpHeader("Referer", referer);
      }
    }
  }
}
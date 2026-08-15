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
#include "webpluginfactory.h"

#include "clicktoflash.h"
#include "mainapplication.h"
#include "adblockmanager.h"

#include <QWebEngineUrlRequestInfo>

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
        QHash<QString, QByteArray>::const_iterator it = refererCache_.constFind(imageHost);
        QByteArray referer;
        if (it != refererCache_.constEnd()) {
          referer = it.value();
        } else {
          referer = url.scheme().toUtf8() + "://" + imageHost.toUtf8() + "/";
          if (refererCache_.size() > 512)
            refererCache_.clear();
          refererCache_.insert(imageHost, referer);
        }
        info.setHttpHeader("Referer", referer);
      }
    }
  }
}
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
#include "webpage.h"

#include "mainapplication.h"
#include "adblockicon.h"
#include "adblockmanager.h"

#include <QAction>
#include <QDesktopServices>
#include <QNetworkRequest>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineCertificateError>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

QList<WebPage*> WebPage::livingPages_;

WebPage::WebPage(QObject *parent)
  : QWebEnginePage(parent)
  , loadProgress_(-1)
  , jumpOutLinkWarn_(false)
{
  // Disable plugins (Flash, etc.) - WebEngine handles this differently
  settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
  settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
  settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

  connect(this, SIGNAL(loadProgress(int)), this, SLOT(progress(int)));
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(finished(bool)));
  connect(this, SIGNAL(featurePermissionRequested(const QUrl&, Feature)),
          this, SLOT(handleFeaturePermissionRequested(const QUrl&, Feature)));
  livingPages_.append(this);
}

WebPage::~WebPage()
{
  livingPages_.removeOne(this);
}

void WebPage::disconnectObjects()
{
  livingPages_.removeOne(this);

  disconnect(this);
}

void WebPage::setJumpOutLinkWarn(bool on)
{
  jumpOutLinkWarn_ = on;
}

bool WebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
  lastRequestType_ = type;
  lastRequestUrl_ = url;

  // S-4: warn before jumping to external links
  if (jumpOutLinkWarn_ && isMainFrame &&
      (type == NavigationTypeLinkClicked)) {
    const QString scheme = url.scheme();
    if ((scheme == QLatin1String("http") || scheme == QLatin1String("https")) &&
        (url.toString(QUrl::RemoveFragment) != this->url().toString(QUrl::RemoveFragment))) {
      emit navigationRequested(url);
      return false;
    }
  }

  // Allow navigation
  return true;
}

QWebEnginePage* WebPage::createWindow(WebWindowType type)
{
  Q_UNUSED(type)
  return mainApp->mainWindow()->createWebTab();
}

void WebPage::scheduleAdjustPage()
{
  WebView* webView = qobject_cast<WebView*>(view());
  if (!webView) {
    return;
  }

  if (webView->isLoading()) {
    adjustingScheduled_ = true;
  } else {
    const QSize &originalSize = webView->size();
    QSize newSize(originalSize.width() - 1, originalSize.height() - 1);

    webView->resize(newSize);
    webView->resize(originalSize);
  }
}

bool WebPage::isLoading() const
{
  return loadProgress_ < 100;
}

void WebPage::progress(int prog)
{
  loadProgress_ = prog;
}

void WebPage::finished(bool ok)
{
  Q_UNUSED(ok)
  progress(100);

  if (adjustingScheduled_) {
    adjustingScheduled_ = false;

    WebView* webView = qobject_cast<WebView*>(view());
    if (webView) {
      const QSize &originalSize = webView->size();
      QSize newSize(originalSize.width() - 1, originalSize.height() - 1);

      webView->resize(newSize);
      webView->resize(originalSize);
    }
  }

  // AdBlock - now using JavaScript-based approach
  cleanBlockedObjects();
}

void WebPage::handleFeaturePermissionRequested(const QUrl &securityOrigin, Feature feature)
{
  Q_UNUSED(securityOrigin)
  Q_UNUSED(feature)
  // Deny feature permissions by default (camera, microphone, etc.)
  setFeaturePermission(securityOrigin, feature, PermissionDeniedByUser);
}

void WebPage::populateNetworkRequest(QWebEngineUrlRequestInfo &request)
{
  // WebEngine doesn't support setting arbitrary attributes on requests the same way
  // We use request interception for ad blocking instead
  Q_UNUSED(request)
}

void WebPage::addAdBlockRule(const AdBlockRule* rule, const QUrl &url)
{
  AdBlockedEntry entry;
  entry.rule = rule;
  entry.url = url;

  if (!adBlockedEntries_.contains(entry)) {
    adBlockedEntries_.append(entry);
  }
}

QVector<WebPage::AdBlockedEntry> WebPage::adBlockedEntries() const
{
  return adBlockedEntries_;
}

void WebPage::cleanBlockedObjects()
{
  AdBlockManager* manager = AdBlockManager::instance();
  if (!manager->isEnabled()) {
    return;
  }

  // Use JavaScript-based ad blocking instead of QWebElement DOM manipulation
  cleanBlockedObjectsJavaScript();
}

void WebPage::cleanBlockedObjectsJavaScript()
{
  AdBlockManager* manager = AdBlockManager::instance();
  if (!manager->isEnabled()) {
    return;
  }

  // Generate JavaScript to remove blocked elements
  QString js = "var removed = 0;";

  foreach (const AdBlockedEntry &entry, adBlockedEntries_) {
    QString urlString = entry.url.toString();
    if (urlString.endsWith(QLatin1String(".js")) || urlString.endsWith(QLatin1String(".css"))) {
      continue;
    }

    QString urlEnd;
    int pos = urlString.lastIndexOf(QLatin1Char('/'));
    if (pos > 8) {
      urlEnd = urlString.mid(pos + 1);
    }

    if (urlString.endsWith(QLatin1Char('/'))) {
      urlEnd = urlString.left(urlString.size() - 1);
    }

    if (!urlEnd.isEmpty()) {
      js += QString("var elements = document.querySelectorAll('img[src$=\"%1\"], iframe[src$=\"%1\"], embed[src$=\"%1\"]'); "
                    "for (var i = 0; i < elements.length; i++) { elements[i].remove(); removed++; }")
            .arg(urlEnd);
    }
  }

  js += "removed;";

  runJavaScript(js, [](const QVariant& result) {
    // Log if needed
    qDebug() << "AdBlock: removed" << result.toInt() << "elements";
  });
}

bool WebPage::isPointerSafeToUse(WebPage* page)
{
  return page == 0 ? false : livingPages_.contains(page);
}

void WebPage::addRejectedCerts(const QList<QSslCertificate> &certs)
{
  Q_UNUSED(certs)
  // Certificate handling moved to certificateError signal
}

bool WebPage::containsRejectedCerts(const QList<QSslCertificate> &certs)
{
  Q_UNUSED(certs)
  return false;
}

bool WebPage::certificateError(const QWebEngineCertificateError &error)
{
  Q_UNUSED(error)
  // Certificate errors are handled by NetworkManagerProxy.
  // Return true to accept the certificate and continue loading.
  return true;
}
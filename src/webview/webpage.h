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
#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebEnginePage>
#include <QSslCertificate>

class AdBlockRule;

class WebPage : public QWebEnginePage
{
  Q_OBJECT
public:
  struct AdBlockedEntry {
    const AdBlockRule* rule;
    QUrl url;

    bool operator==(const AdBlockedEntry &other) const {
      return (this->rule == other.rule && this->url == other.url);
    }
  };

  explicit WebPage(QObject *parent);
  ~WebPage();

  void disconnectObjects();

  // Navigation handling - overridden from QWebEnginePage
  bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame);
  void populateNetworkRequest(QWebEngineUrlRequestInfo &request);

  void scheduleAdjustPage();
  bool isLoading() const;

  static bool isPointerSafeToUse(WebPage* page);
  void addAdBlockRule(const AdBlockRule* rule, const QUrl &url);
  QVector<AdBlockedEntry> adBlockedEntries() const;

  void addRejectedCerts(const QList<QSslCertificate> &certs);
  bool containsRejectedCerts(const QList<QSslCertificate> &certs);

protected:
  // Override to handle custom window creation
  QWebEnginePage* createWindow(WebWindowType type) override;

private slots:
  void progress(int prog);
  void finished(bool ok);
  void handleFeaturePermissionRequested(const QUrl &securityOrigin, Feature feature);

private:
  WebPage::NavigationType lastRequestType_;
  QUrl lastRequestUrl_;

  bool adjustingScheduled_;
  static QList<WebPage*> livingPages_;
  QVector<AdBlockedEntry> adBlockedEntries_;

  int loadProgress_;

  // AdBlock related
  void cleanBlockedObjects();
  void cleanBlockedObjectsJavaScript();

  // WebEngine specific: certificate error handling
  void certificateError(const QWebEngineCertificateError &error);
};

#endif // WEBPAGE_H
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
#include "mainapplication.h"

#include "common.h"
#include "cookiejar.h"
#include "database.h"
#include "globals.h"
#include "networkmanager.h"
#include "adblockmanager.h"
#include "settings.h"
#include "imagecache.h"
#include "splashscreen.h"
#include "updatefeeds.h"
#include "VersionNo.h"
#include "webpluginfactory.h"

#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineSettings>
#include <QFile>
#include <QMessageBox>
#include <QCheckBox>
#include <QRegularExpression>
#include <QSslSocket>
#include <QSettings>
#include <QPalette>
#include <QColor>

MainApplication::MainApplication(int &argc, char **argv)
  : QtSingleApplication(argc, argv)
  , isPortableAppsCom_(false)
  , isClosing_(false)
  , dbFileExists_(false)
  , translator_(0)
  , qt_translator_(0)
  , mainWindow_(0)
  , networkManager_(0)
  , cookieJar_(0)
  , diskCache_(0)
  , downloadManager_(0)
  , imageCache_(0)
  , analytics_(0)
{
  setApplicationName("Quill");
  setOrganizationName("Quill");
  setApplicationVersion(STRPRODUCTVER);
  globals.init();

  QString message = arguments().value(1);
  if (isRunning()) {
    if (argc == 1) {
      sendMessage("--show");
    } else {
      for (int i = 2; i < argc; ++i)
        message += '\n' + arguments().value(i);
      sendMessage(message);
    }
    isClosing_ = true;
    return;
  } else {
    if (message.contains("--exit", Qt::CaseInsensitive)) {
      isClosing_ = true;
      return;
    }
  }

  setWindowIcon(QIcon(":/images/quill128"));
  setQuitOnLastWindowClosed(false);

  createSettings();

  qWarning() << "Run application!";

  setStyleApplication();
  setTranslateApplication();
  showSplashScreen();

  createGoogleAnalytics();

  connectDatabase();
  setProgressSplashScreen(30);
  qWarning() << "Run application 2";
  mainWindow_ = new MainWindow();
  qWarning() << "Run application 3";
  setProgressSplashScreen(60);

  loadSettings();
  qWarning() << "Run application 4";
  updateFeeds_ = new UpdateFeeds(mainWindow_);
  setProgressSplashScreen(90);
  qWarning() << "Run application 5";
  mainWindow_->restoreFeedsOnStartUp();
  updateFeeds_->startSpeedDetection();
  setProgressSplashScreen(100);
  qWarning() << "Run application 6";
  if (!mainWindow_->startingTray_ || !mainWindow_->showTrayIcon_) {
    mainWindow_->show();
  }
  mainWindow_->isMinimizeToTray_ = false;

  closeSplashScreen();

  if (mainWindow_->showTrayIcon_) {
    QTimer::singleShot(0, mainWindow_->traySystem, SLOT(show()));
  }

  // Warn the user up front if the OpenSSL runtime is missing. Without it
  // every https:// feed fails with "TLS initialization failed" (which used
  // to be silently swallowed in the feed-update thread), so the user would
  // only see feeds "not refreshing" with no explanation.
  if (!QSslSocket::supportsSsl()) {
    QTimer::singleShot(500, this, SLOT(slotCheckSslRuntime()));
  }

  if (updateFeedsStartUp_) {
    QTimer::singleShot(0, mainWindow_, SLOT(slotGetAllFeeds()));
  }

  receiveMessage(message);
  connect(this, SIGNAL(messageReceived(QString)), SLOT(receiveMessage(QString)));
}

MainApplication::~MainApplication()
{

}

MainApplication *MainApplication::getInstance()
{
  return static_cast<MainApplication*>(QCoreApplication::instance());
}

void MainApplication::receiveMessage(const QString &message)
{
  if (!message.isEmpty()) {
    qWarning() << QString("Received message: %1").arg(message);

    QStringList params = message.split('\n');
    foreach (QString param, params) {
      if (param == "--show") {
        if (isClosing_)
          return;
        mainWindow_->showWindows();
      }
      if (param == "--exit") mainWindow_->quitApp();
      if (param.contains("feed:", Qt::CaseInsensitive)) {
        // Browsers/extensions pass feed URLs in several forms:
        //   feed:https://example.com/feed.xml
        //   feed://https://example.com/feed.xml
        //   feed://example.com/feed.xml
        //   feed:example.com/feed.xml
        // Strip the "feed:" scheme and any leading slashes, then restore the
        // real http(s) scheme if it was carried along.
        QRegularExpression re("feed:/*(.*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(param);
        QString url = m.hasMatch() ? m.captured(1) : param;
        if (!url.startsWith("http://", Qt::CaseInsensitive) &&
            !url.startsWith("https://", Qt::CaseInsensitive))
          url.prepend("http://");
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(url);
        mainWindow_->addFeed();
      }
    }
  }
}

void MainApplication::createSettings()
{
  Settings settings;
  settings.beginGroup("Settings");
  storeDBMemory_ = settings.value("storeDBMemory", true).toBool();
  isSaveDataLastFeed_ = settings.value("createLastFeed", false).toBool();
  styleApplication_ = settings.value("styleApplication", "systemStyle_").toString();
  showSplashScreen_ = settings.value("showSplashScreen", true).toBool();
  updateFeedsStartUp_ = settings.value("autoUpdatefeedsStartUp", false).toBool();

  QString strLang;
  QString strLocalLang = QLocale::system().name();
  bool findLang = false;
  QDir langDir(resourcesDir() + "/lang");
  foreach (QString file, langDir.entryList(QStringList("*.qm"), QDir::Files)) {
    strLang = file.section('.', 0, 0).section('_', 1);
    if (strLocalLang == strLang) {
      strLang = strLocalLang;
      findLang = true;
      break;
    }
  }
  if (!findLang) {
    strLocalLang = strLocalLang.left(2);
    foreach (QString file, langDir.entryList(QStringList("*.qm"), QDir::Files)) {
      strLang = file.section('.', 0, 0).section('_', 1);
      if (strLocalLang.contains(strLang, Qt::CaseInsensitive)) {
        strLang = strLocalLang;
        findLang = true;
        break;
      }
    }
  }
  if (!findLang) strLang = "en";
  // A previously stored language may point to a .qm file that no longer
  // exists (e.g. it was saved while the lang/ dir was missing), which would
  // permanently override the locale auto-detection above. Validate the
  // stored value and fall back to the detected locale if it is unusable.
  QString storedLang = settings.value("langFileName", QString()).toString();
  if (storedLang.isEmpty() ||
      !QFile::exists(resourcesDir() + "/lang/quill_" + storedLang + ".qm")) {
    storedLang = strLang;
  }
  langFileName_ = storedLang;

  settings.endGroup();

  proxyLoadSettings();
}

void MainApplication::createGoogleAnalytics()
{
  Settings settings;
  bool statisticsEnabled = settings.value("Settings/statisticsEnabled2", true).toBool();
  if (statisticsEnabled) {
    QString clientID;
    if (!settings.contains("GAnalytics-cid")) {
      settings.setValue("GAnalytics-cid", QUuid::createUuid().toString());
    }
    clientID = settings.value("GAnalytics-cid").toString();
    analytics_ = new GAnalytics(this, TRACKING_ID, clientID);
    analytics_->generateUserAgentEtc();
    analytics_->startSession();
  }
}

void MainApplication::connectDatabase()
{
  QString fileName(dbFileName() % ".bak");
  if (QFile(fileName).exists()) {
    QString sourceFileName = QFile::symLinkTarget(dbFileName());
    if (sourceFileName.isEmpty()) {
      sourceFileName = dbFileName();
    }
    if (QFile::remove(sourceFileName)) {
      if (!QFile::rename(fileName, sourceFileName))
        qCritical() << "Failed to rename new database file!";
    } else {
      qCritical() << "Failed to delete old database file!";
    }
  }

#if defined(HAVE_QT5) && defined(HAVE_X11)
  fileName = "~/.local/share/data/Quill/Quill/feeds.db";
  if (!QFile(dbFileName()).exists() && QFile(fileName).exists()) {
    QFile::copy(fileName, dbFileName());
  }
#endif

  if (QFile(dbFileName()).exists()) {
    dbFileExists_ = true;
  }

  Database::initialization();
}

void MainApplication::loadSettings()
{
  c2fLoadSettings();
  reloadUserStyleBrowser();

  // 注册 URL 请求拦截器（AdBlock / ClickToFlash SWF 拦截），只注册一次，profile 持有所有权
  static bool interceptorRegistered = false;
  if (!interceptorRegistered) {
    interceptorRegistered = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6 renamed setRequestInterceptor -> setUrlRequestInterceptor
    QWebEngineProfile::defaultProfile()->setUrlRequestInterceptor(new WebPluginFactory());
#else
    QWebEngineProfile::defaultProfile()->setRequestInterceptor(new WebPluginFactory());
#endif
  }
}

/** @brief Warn the user if the OpenSSL runtime is unavailable
 *
 * Qt 5 links OpenSSL dynamically. On Windows, if libssl-1_1-x64.dll /
 * libcrypto-1_1-x64.dll are absent next to the executable, QSslSocket cannot
 * initialize TLS and every https:// feed fails with "TLS initialization
 * failed". The feed-update thread used to swallow this, so surface it here.
 *---------------------------------------------------------------------------*/
void MainApplication::slotCheckSslRuntime()
{
  if (QSslSocket::supportsSsl())
    return;

  // Let the user dismiss this permanently; it only fires once per startup,
  // but it would otherwise reappear on every launch until fixed.
  Settings settings;
  settings.beginGroup("SSL-Configuration");
  if (settings.value("SkipMissingRuntimeWarning", false).toBool()) {
    settings.endGroup();
    return;
  }
  settings.endGroup();

  QString details;
#if defined(Q_OS_WIN)
  details = tr("Quill needs the OpenSSL 1.1.x runtime libraries "
               "(libssl-1_1-x64.dll and libcrypto-1_1-x64.dll) next to "
               "Quill.exe to refresh HTTPS feeds.\n\n"
               "Without them, every HTTPS subscription fails to update "
               "(\"TLS initialization failed\"). Please reinstall Quill "
               "or copy these two DLLs into the application folder, then "
               "restart.");
#else
  details = tr("Quill cannot find an OpenSSL 1.1.x runtime, so HTTPS "
               "feeds will fail to refresh. Please install the OpenSSL 1.1 "
               "libraries for your system and restart Quill.");
#endif

  qWarning() << "OpenSSL runtime not found (QSslSocket::supportsSsl() == false);"
             << "HTTPS feeds will fail to refresh. Build/OpenSSL info:"
             << QSslSocket::sslLibraryBuildVersionString()
             << " / " << QSslSocket::sslLibraryVersionString();

  QWidget *parent = mainWindow_ ? static_cast<QWidget*>(mainWindow_) : 0;
  QMessageBox box(QMessageBox::Warning, tr("HTTPS feeds unavailable"),
                  details, QMessageBox::Ok, parent);
  QCheckBox *dontShow = new QCheckBox(tr("Do not show this warning again"));
  box.setCheckBox(dontShow);
  box.exec();

  if (dontShow->isChecked()) {
    settings.beginGroup("SSL-Configuration");
    settings.setValue("SkipMissingRuntimeWarning", true);
    settings.endGroup();
  }
}

void MainApplication::quitApplication()
{
  qWarning() << "quitApplication 1";
  delete mainWindow_;
  qWarning() << "quitApplication 2";
  delete networkManager_;
  delete cookieJar_;
  delete closingWidget_;

  if (analytics_) {
    analytics_->endSession();
    analytics_->waitForIdle();
    delete analytics_;
  }

  qWarning() << "Quit application";

  quit();
}

void MainApplication::showClosingWidget()
{
  closingWidget_ = new QWidget(0, Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
  closingWidget_->setFocusPolicy(Qt::NoFocus);
  QVBoxLayout *layout = new QVBoxLayout(closingWidget_);
  layout->addWidget(new QLabel(tr("Saving data...")));
  closingWidget_->resize(150, 20);
  closingWidget_->show();
  const QRect desktop = Common::desktopAvailableGeometry();
  closingWidget_->move(desktop.width() - closingWidget_->frameSize().width(),
               desktop.height() - closingWidget_->frameSize().height());
  closingWidget_->setFixedSize(closingWidget_->size());
  qApp->processEvents();
}

void MainApplication::commitData(QSessionManager &manager)
{
  manager.release();
  mainWindow_->quitApp();
}

bool MainApplication::isPortable() const
{
  return globals.isPortable_;
}

bool MainApplication::isPortableAppsCom() const
{
  return isPortableAppsCom_;
}

void MainApplication::setClosing()
{
  isClosing_ = true;
}

bool MainApplication::isClosing() const
{
  return isClosing_;
}

bool MainApplication::isNoDebugOutput() const
{
  return globals.noDebugOutput_;
}

QString MainApplication::resourcesDir() const
{
  return globals.resourcesDir_;
}

QString MainApplication::dataDir() const
{
  return globals.dataDir_;
}

QString MainApplication::absolutePath(const QString &path) const
{
  QString absolutePath = path;
  if (isPortable()) {
    if (!QDir::isAbsolutePath(path)) {
      absolutePath = dataDir() % "/" % path;
    }
  }
  return absolutePath;
}

QString MainApplication::dbFileName() const
{
  return dataDir() % "/feeds.db";
}

bool MainApplication::isSaveDataLastFeed() const
{
  return isSaveDataLastFeed_;
}

bool MainApplication::storeDBMemory() const
{
  return storeDBMemory_;
}

bool MainApplication::systemDarkMode()
{
#ifdef Q_OS_WIN
  // Windows stores the personal "app mode" theme setting in the registry.
  QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\"
                "CurrentVersion\\Themes\\Personalize",
                QSettings::NativeFormat);
  const QVariant value = reg.value("AppsUseLightTheme");
  if (value.isValid())
    return !value.toBool();
#endif
  // Fallback heuristic for other platforms: a dark system palette has a
  // dark Window colour.
  return qApp->palette().color(QPalette::Window).lightness() < 128;
}

void MainApplication::setStyleApplication()
{
  QString fileName(resourcesDir());
  // "systemStyle_" follows the OS dark/light mode; "lightStyle_" and
  // "darkStyle_" are explicit overrides. Legacy names are normalized.
  const bool dark = (styleApplication_ == "darkStyle_" ||
                     styleApplication_ == "dark" ||
                     (styleApplication_ == "systemStyle_" && systemDarkMode()));
  fileName.append(dark ? "/style/codex_dark.qss"
                       : "/style/codex_light.qss");
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly)) {
    file.setFileName(":/style/systemStyle");
    file.open(QFile::ReadOnly);
  }
  // Apply accent color placeholders so the startup stylesheet is fully valid
  // (same logic as MainWindow::setStyleApp).
  Settings settings;
  const QString accentStr = settings.value("accentColor", "").toString();
  QColor accent(accentStr);
  if (!accent.isValid())
    accent = QColor(dark ? "#3B82F6" : "#0F62FE");
  QColor accentHover = dark ? accent.lighter(130) : accent.lighter(115);
  QColor accentSoft, accentSoftActive;
  if (dark) {
    accentSoft        = QColor::fromRgbF(accent.redF()*0.27, accent.greenF()*0.27, accent.blueF()*0.27);
    accentSoftActive  = QColor::fromRgbF(accent.redF()*0.40, accent.greenF()*0.40, accent.blueF()*0.40);
  } else {
    accentSoft        = accent.lighter(150);
    accentSoftActive  = accent.lighter(130);
  }
  QString qss = QString::fromUtf8(file.readAll());
  qss.replace("%ACCENT%", accent.name());
  qss.replace("%ACCENT_HOVER%", accentHover.name());
  qss.replace("%ACCENT_SOFT%", accentSoft.name());
  qss.replace("%ACCENT_SOFT_ACTIVE%", accentSoftActive.name());
  setStyleSheet(qss);
  file.close();

  setStyle(new QProxyStyle);
}

void MainApplication::setTranslateApplication()
{
  if (!translator_)
    translator_ = new QTranslator(this);
  removeTranslator(translator_);
  translator_->load(resourcesDir() + QString("/lang/quill_%1").arg(langFileName_));
  installTranslator(translator_);

  if (!qt_translator_)
    qt_translator_ = new QTranslator(this);
  removeTranslator(qt_translator_);
#ifdef HAVE_X11
  qt_translator_->load(QLibraryInfo::location (QLibraryInfo::TranslationsPath) + "/qtbase_" + langFileName_);
#else
  qt_translator_->load(resourcesDir() + "/lang/qtbase_" + langFileName_);
#endif
  installTranslator(qt_translator_);
}

void MainApplication::showSplashScreen()
{
  Settings settings;
  int versionDB = settings.value("VersionDB", "1").toInt();
  if ((versionDB != Database::version()) && QFile::exists(settings.fileName()))
    showSplashScreen_ = true;

  if (showSplashScreen_) {
    splashScreen_ = new SplashScreen(QPixmap(":/images/images/splashScreen.png"));
    splashScreen_->show();
    processEvents();
    if ((versionDB != Database::version()) && QFile::exists(settings.fileName())) {
      splashScreen_->showMessage(QString("Converting database to version %1...").arg(Database::version()),
                                Qt::AlignRight | Qt::AlignTop, Qt::darkGray);
    }
  }
}

void MainApplication::closeSplashScreen()
{
  if (showSplashScreen_) {
    splashScreen_->finish(mainWindow_);
    splashScreen_->deleteLater();
  }
}

void MainApplication::setProgressSplashScreen(int value)
{
  if (showSplashScreen_)
    splashScreen_->setProgress(value);
}

MainWindow *MainApplication::mainWindow()
{
  return mainWindow_;
}

NetworkManager *MainApplication::networkManager()
{
  if (!networkManager_) {
    networkManager_ = new NetworkManager(false, this);
    setDiskCache();
  }
  return networkManager_;
}

CookieJar *MainApplication::cookieJar()
{
  if (!cookieJar_) {
    cookieJar_ = new CookieJar(this);
  }
  return cookieJar_;
}

void MainApplication::setDiskCache()
{
  Settings settings;
  settings.beginGroup("Settings");

  bool useDiskCache = settings.value("useDiskCache", true).toBool();
  if (useDiskCache) {
    if (!diskCache_) {
      diskCache_ = new QNetworkDiskCache(this);
    }

    QString diskCacheDirPath = settings.value("dirDiskCache", cacheDefaultDir()).toString();
    if (diskCacheDirPath.isEmpty()) diskCacheDirPath = cacheDefaultDir();
    diskCacheDirPath = absolutePath(diskCacheDirPath);

    bool cleanDiskCache = settings.value("cleanDiskCache", true).toBool();
    if (cleanDiskCache) {
      Common::removePath(diskCacheDirPath);
      settings.setValue("cleanDiskCache", false);
    }

    diskCache_->setCacheDirectory(diskCacheDirPath);
    int maxDiskCache = settings.value("maxDiskCache", 50).toInt();
    diskCache_->setMaximumCacheSize(maxDiskCache*1024*1024);

    networkManager()->setCache(diskCache_);
  } else {
    if (diskCache_) {
      diskCache_->setMaximumCacheSize(0);
      diskCache_->clear();
    }
  }

  settings.endGroup();
}

QString MainApplication::cacheDefaultDir() const
{
  return globals.cacheDir_;
}

QString MainApplication::soundNotifyDefaultFile() const
{
  return globals.soundNotifyDir_ % "/notification.wav";
}

QString MainApplication::styleSheetNewsDefaultFile() const
{
  if (isPortable()) {
    return "style/news.css";
  } else {
    return resourcesDir() % "/style/news.css";
  }
}

QString MainApplication::styleSheetWebDarkFile() const
{
  if (isPortable()) {
    return "style/web_dark.css";
  } else {
    return resourcesDir() % "/style/web_dark.css";
  }
}

UpdateFeeds *MainApplication::updateFeeds()
{
  return updateFeeds_;
}

void MainApplication::runUserFilter(int feedId, int filterId)
{
  emit signalRunUserFilter(feedId, filterId);
}

void MainApplication::sqlQueryExec(const QString &query)
{
  emit signalSqlQueryExec(query);
}

/** @brief Click to Flash
 *---------------------------------------------------------------------------*/
void MainApplication::c2fLoadSettings()
{
  Settings settings;
  settings.beginGroup("ClickToFlash");
  c2fWhitelist_ = settings.value("whitelist", QStringList()).toStringList();
  c2fEnabled_ = settings.value("enabled", true).toBool();
#if QT_VERSION >= 0x050900
  c2fEnabled_ = false;
#endif
  settings.endGroup();
}

void MainApplication::c2fSaveSettings()
{
  Settings settings;
  settings.beginGroup("ClickToFlash");
  settings.setValue("whitelist", c2fWhitelist_);
  settings.setValue("enabled", c2fEnabled_);
  settings.endGroup();
}

bool MainApplication::c2fIsEnabled() const
{
  return c2fEnabled_;
}

void MainApplication::c2fSetEnabled(bool enabled)
{
  c2fEnabled_ = enabled;
}

QStringList MainApplication::c2fGetWhitelist()
{
  return c2fWhitelist_;
}

void MainApplication::c2fSetWhitelist(QStringList whitelist)
{
  c2fWhitelist_ = whitelist;
}

void MainApplication::c2fAddWhitelist(const QString &site)
{
  c2fWhitelist_.append(site);
}

DownloadManager *MainApplication::downloadManager()
{
  if (!downloadManager_) {
    downloadManager_ = new DownloadManager();
  }
  return downloadManager_;
}

ImageCacheManager *MainApplication::imageCache()
{
  if (!imageCache_) {
    imageCache_ = new ImageCacheManager(this);
  }
  return imageCache_;
}

void MainApplication::reloadUserStyleBrowser()
{
  Settings settings;
  settings.beginGroup("Settings");
  QString userStyleBrowser = settings.value("userStyleBrowser", QString()).toString();
  QUrl styleUrl = userStyleSheet(userStyleBrowser);
  settings.endGroup();

  // Qt 5.15 has no QWebEngineSettings::setUserStyleSheetUrl (Qt 6 API);
  // inject the stylesheet via a document-level script instead.
  QWebEngineScript styleScript;
  styleScript.setName(QStringLiteral("_qtrssUserStyle"));
  styleScript.setInjectionPoint(QWebEngineScript::DocumentReady);
  styleScript.setRunsOnSubFrames(true);
  styleScript.setWorldId(QWebEngineScript::ApplicationWorld);
  styleScript.setSourceCode(QStringLiteral(
      "(function(){var s=document.createElement('link');"
      "s.rel='stylesheet';s.type='text/css';s.href='%1';"
      "(document.head||document.documentElement).appendChild(s);})();")
      .arg(styleUrl.toString()));
  QWebEngineProfile::defaultProfile()->scripts()->insert(styleScript);
}

/** @brief Set user style sheet for browser
 * @param filePath Filepath of user style
 * @return URL-link to user style
 *---------------------------------------------------------------------------*/
QUrl MainApplication::userStyleSheet(const QString &filePath) const
{
  QString userStyle;

#ifndef HAVE_X11
  // Don't grey out selection on losing focus (to prevent graying out found text)
  QString highlightColor;
  QString highlightedTextColor;
#ifdef Q_OS_MAC
  highlightColor = QLatin1String("#b6d6fc");
  highlightedTextColor = QLatin1String("#000");
#else
  QPalette pal = style()->standardPalette();
  highlightColor = pal.color(QPalette::Highlight).name();
  highlightedTextColor = pal.color(QPalette::HighlightedText).name();
#endif
  userStyle += QString("::selection {background: %1; color: %2;} ").arg(highlightColor, highlightedTextColor);
#endif

  userStyle += AdBlockManager::instance()->elementHidingRules();

  QFile file(filePath);
  if (!filePath.isEmpty() && file.open(QFile::ReadOnly)) {
    QString fileData = QString::fromUtf8(file.readAll());
    fileData.remove(QLatin1Char('\n'));
    userStyle.append(fileData);
    file.close();
  }

  const QString &encodedStyle = userStyle.toLatin1().toBase64();
  const QString &dataString = QString("data:text/css;charset=utf-8;base64,%1").arg(encodedStyle);

  return QUrl(dataString);
}

void MainApplication::proxyLoadSettings()
{
  Settings settings;
  settings.beginGroup("networkProxy");
  networkProxy_.setType(static_cast<QNetworkProxy::ProxyType>(
                          settings.value("type", QNetworkProxy::DefaultProxy).toInt()));
  networkProxy_.setHostName(settings.value("hostName", "").toString());
  networkProxy_.setPort(    settings.value("port",     "").toUInt());
  networkProxy_.setUser(    settings.value("user",     "").toString());
  networkProxy_.setPassword(settings.value("password", "").toString());
  settings.endGroup();

  setProxy();
}

void MainApplication::proxySaveSettings(const QNetworkProxy &proxy)
{
  networkProxy_ = proxy;

  Settings settings;
  settings.beginGroup("networkProxy");
  settings.setValue("type",     networkProxy_.type());
  settings.setValue("hostName", networkProxy_.hostName());
  settings.setValue("port",     networkProxy_.port());
  settings.setValue("user",     networkProxy_.user());
  settings.setValue("password", networkProxy_.password());
  settings.endGroup();

  setProxy();
}

void MainApplication::setProxy()
{

  if (QNetworkProxy::DefaultProxy == networkProxy_.type())
    QNetworkProxyFactory::setUseSystemConfiguration(true);
  else
    QNetworkProxy::setApplicationProxy(networkProxy_);
}
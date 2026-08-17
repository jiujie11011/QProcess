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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// NOTE: use qmake-injected QT5/QT6 macros here, NOT QT_VERSION_CHECK: this
// #if sits before any Qt include, and qt6compat.h (force-included first) no
// longer pulls in <QtGlobal> for translation units whose INCPATH lacks Qt dirs.
#if defined(HAVE_QT5) && !defined(QT6)
#include <QtWidgets>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#elif defined(QT6)
#include <QtWidgets>
#include <QMediaPlayer>
#else
#include <QtGui>
#ifdef HAVE_PHONON
#include <phonon/audiooutput.h>
#include <phonon/mediaobject.h>
#endif
#endif
#include <QtSql>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#if defined(QT6)
#include <QtMultimedia/QSoundEffect>
#else
#include <QSound>
#endif

#include "categoriestreewidget.h"
#include "feedsmodel.h"
#include "feedsview.h"
#include "findfeed.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "newstabwidget.h"
#include "newsview.h"

class IntelligentRefreshCalculator;
#include "notificationswidget.h"
#include "tabbar.h"
#include "optionsdialog.h"
#include "updateappdialog.h"
#include "webview.h"
#include "parseobject.h"
#include "toolbutton.h"
#include "progressservice.h"
#include "statistics.h"
#include "aiassistant.h"
#include "translationservice.h"

#define TAB_WIDGET_PERMANENT 0

#define NEW_TAB_FOREGROUND 1
#define NEW_TAB_BACKGROUND 2

#define MAX_TAB_WIDTH 150

enum FeedReedType {
  FeedReadSwitchingFeed,
  FeedReadClosingTab,
  FeedReadPlaceToTray,
  FeedReadTypeCount,
  FeedReadSwitchingTab
};

class AdBlockIcon;

// ---- Codex UI 新控件类前向声明（仅指针，不 #include 实现头）----
class NavRail;
class SplitterHandle;
class NewsCardDelegate;
class RightPanel;
class PlayerBar;
class CommandPalette;
class ThemeManager;
class NavigationContext;
class SvgIconEngine;
class ReaderToolbar;

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  void loadSettings();
  void saveSettings();

  /** True when focus mode (reader only) is active. */
  bool isFocusMode() const { return focusMode_; }

  bool showSplashScreen_;
  bool showTrayIcon_;
  bool startingTray_;
  bool isMinimizeToTray_;
  QSystemTrayIcon *traySystem;

  QString getIdFeedsString(int idFolder, int idException = -1);
  void recountCategoryCounts();

  void setToolBarStyle(const QString &styleStr);
  void setToolBarIconSize(QToolBar *toolbar, const QString &iconSizeStr);

  static QStringList nameLabels() {
    QStringList nameLabels;
    nameLabels << "Important" << "Work" << "Personal"
               << "To Do" << "Later" << "Amusingly";
    return nameLabels;
  }
  static QStringList trNameLabels() {
    QStringList trNameLabels;
    trNameLabels << tr("Important") << tr("Work") << tr("Personal")
                 << tr("To Do") << tr("Later") << tr("Amusingly");
    return trNameLabels;
  }

  QSqlDatabase db_;
  FeedsModel *feedsModel_;
  FeedsProxyModel *feedsProxyModel_;
  FeedsView *feedsView_;
  CategoriesTreeWidget *categoriesTree_;
  QStackedWidget *stackedWidget_;

  bool showFeedsTabPermanent_;

  NewsTabWidget *currentNewsTab;
  ProgressService *progressService_;
  StatisticsService *statisticsService_;
  AIAssistant *aiAssistant_;
  TranslationService *translationService_;
  IntelligentRefreshCalculator *intelligentRefresh_;

  QAction *newsToolbarToggle_;
  QAction *browserToolbarToggle_;
  QAction *categoriesPanelToggle_;
  QAction *statusBarToggle_;
  QAction *newsFilter_;
  QAction *openDescriptionNewsAct_;
  QAction *openInBrowserAct_;
  QAction *openInExternalBrowserAct_;
  QAction *openNewsNewTabAct_;
  QAction *openNewsBackgroundTabAct_;
  QAction *markNewsRead_;
  QAction *markAllNewsRead_;
  QAction *markStarAct_;
  QAction *updateFeedAct_;
  QAction *deleteNewsAct_;
  QAction *deleteAllNewsAct_;
  QAction *newsKeyUpAct_;
  QAction *newsKeyDownAct_;
  QAction *newsKeyPageUpAct_;
  QAction *newsKeyPageDownAct_;
  QAction *prevUnreadNewsAct_;
  QAction *nextUnreadNewsAct_;
  QAction *autoLoadImagesToggle_;
  QAction *printAct_;
  QAction *printPreviewAct_;
  QAction *savePageAsAct_;
  QAction *savePageAsDescriptAct_;
  QAction *restoreNewsAct_;
  QAction *restoreLastNewsAct_;
  QAction *newsLabelAction_;
  QAction *newsLabelMenuAction_;
  QAction *showLabelsMenuAct_;
  QAction *findTextAct_;
  QAction *openHomeFeedAct_;
  QAction *shareMenuAct_;
  QAction *emailShareAct_;
  QAction *evernoteShareAct_;
  QAction *facebookShareAct_;
  QAction *livejournalShareAct_;
  QAction *pocketShareAct_;
  QAction *twitterShareAct_;
  QAction *vkShareAct_;
  QAction *linkedinShareAct_;
  QAction *bloggerShareAct_;
  QAction *printfriendlyShareAct_;
  QAction *instapaperShareAct_;
  QAction *redditShareAct_;
  QAction *hackerNewsShareAct_;
  QAction *telegramShareAct_;
  QAction *viberShareAct_;
  QAction *copyLinkAct_;
  QAction *pageUpWebViewAct_;
  QAction *pageDownWebViewAct_;
  QAction *nextFolderAct_;
  QAction *prevFolderAct_;
  QAction *expandFolderAct_;
  QAction *closeTabAct_;
  QAction *closeOtherTabsAct_;
  QAction *closeAllTabsAct_;
  QAction *settingPageLabelsAct_;
  QAction *backWebPageAct_;
  QAction *forwardWebPageAct_;
  QAction *reloadWebPageAct_;

  QActionGroup *newsFilterGroup_;
  QActionGroup *newsLabelGroup_;
  QActionGroup *shareGroup_;

  QMenu *newsLabelMenu_;
  QMenu *shareMenu_;
  QMenu *newsSortByMenu_;

  QString newsListFontFamily_;
  int newsListFontSize_;
  QString newsTitleFontFamily_;
  int newsTitleFontSize_;
  QString newsTextFontFamily_;
  int newsTextFontSize_;
  QString notificationFontFamily_;
  int notificationFontSize_;
  QString newsListTextColor_;
  QString newsListBackgroundColor_;
  QString newNewsTextColor_;
  QString unreadNewsTextColor_;
  QString focusedNewsTextColor_;
  QString focusedNewsBGColor_;
  QString linkColor_;
  QString titleColor_;
  QString dateColor_;
  QString authorColor_;
  QString newsTextColor_;
  QString newsTitleBackgroundColor_;
  QString newsBackgroundColor_;
  QString alternatingRowColors_;
  QString notifierTextColor_;
  QString notifierBackgroundColor_;
  QString formatDate_;
  QString formatTime_;

  bool markNewsReadOn_;
  bool markCurNewsRead_;
  int  markNewsReadTime_;
  bool markPrevNewsRead_;
  bool markReadSwitchingFeed_;
  bool markReadClosingTab_;
  bool markReadMinimize_;
  bool hoverMarkRead_;
  int  hoverMarkDelay_;
  bool scrollMarkRead_;
  bool viewportMarkRead_;
  bool markExcludeOnlyStarred_;
  QStringList excludedGroups_;
  QStringList excludedFeeds_;
  bool cleanupEnabled_;
  QString cleanupStrategy_;
  int  cleanupDays_;
  int  cleanupKeepCount_;
  int  cleanupRecycleDays_;
  bool autoCleanUpEnable_;
  int  autoCleanUpIntervalDays_;
  bool aiEnabled_;
  QString aiModel_;
  QString aiApiKey_;
  QString aiBaseUrl_;
  int  aiSummaryLength_;
  int  aiHistoryRetention_;
  bool aiOfflineCache_;
  bool aiAutoTranslate_;
  QString aiTranslateLang_;
  QString aiTranslationEngine_;
  QString aiDeeplKey_;
  QString aiBaiduAppId_;
  QString aiBaiduKey_;
  bool aiAutoSummary_;
  bool aiAutoRecommend_;
  int  aiMaxTokens_;
  bool showDescriptionNews_;
  bool alternatingRowColorsNews_;
  bool simplifiedDateTime_;
  bool notDeleteStarred_;
  bool notDeleteLabeled_;
  bool markIdenticalNewsRead_;
  bool avoidOldNews_;
  QDate avoidedOldNewsDate_;

  bool autoLoadImages_;
  bool openLinkInBackground_;
  bool isOpeningLink_;  //!< Flag - link is being opened
  bool openLinkInBackgroundEmbedded_;

  int externalBrowserOn_;
  QString externalBrowser_;
  bool javaScriptEnable_;
  bool pluginsEnable_;
  int maxPagesInCache_;
  QString downloadLocation_;
  bool askDownloadLocation_;
  int defaultZoomPages_;

  int newsLayout_;
  int browserPosition_;

  QString newsFilterStr;

  int oldState;

  bool hideFeedsOpenTab_;
  bool showToggleFeedsTree_;
  bool defaultIconFeeds_;

  int openNewsTab_;

  int screenNotify_;
  int positionNotify_;
  int transparencyNotify_;
  int countShowNewsNotify_;
  int widthTitleNewsNotify_;
  int timeShowNewsNotify_;
  bool showTitlesFeedsNotify_;
  bool showIconFeedNotify_;
  bool showButtonMarkAllNotify_;
  bool showButtonMarkReadNotify_;
  bool showButtonExBrowserNotify_;
  bool showButtonDeleteNotify_;
  bool closeNotify_;
  QList<int> idFeedsNotifyList_;

  AdBlockIcon *adBlockIcon() { return adblockIcon_; }

  void webViewFullScreen(bool on);

  // Podcast: play an audio/video enclosure in the global player bar.
  void playPodcast(const QUrl &url, const QString &title);

  // AI tools: resolve the currently selected folder/feed into a list of
  // concrete feed ids plus a display name. Returns false if nothing usable.
  bool getSelectedFeedIds(QList<int> *feedIds, QString *name);

  // S-1: dim read news in the list
  bool dimRead_;
  // S-2: group news by date in the list
  bool groupByDate_;
  // S-3: show only unread news
  bool unreadOnly_;
  // S-4: warn when jumping to external link
  bool jumpOutLinkWarn_;
  // S-5: accent color for theme
  QString accentColor_;
  // S-6: reader font size / line height
  int readerFontSize_;
  int readerLineHeight_;
  // S-7: custom date format
  QString customDateFormat_;
  // S-8: highlight code in news
  bool highlightCode_;
  // S-9: wide mode
  bool wideMode_;
  // S-10: reduce motion
  bool reduceMotion_;
  // Focus mode: hide the feed tree and the news list, reader only.
  bool focusMode_;

  /** Info needed to restore a closed tab (undo close). */
  struct ClosedTabInfo {
    int type;
    int feedId;
    int feedParId;
    QString title;
  };
  QList<ClosedTabInfo> closedTabs_;

public slots:
  // Entry points invoked from NewsTabWidget via new-style connects; they must
  // stay public so their addresses can be taken from other classes.
  void findText();
  void setTextTitle(const QString &text, NewsTabWidget *widget);

  void restoreFeedsOnStartUp();
  void addFeed();
  void addFolder();
  void deleteItemFeedsTree();
  void slotImportFeeds();
  void slotExportFeeds();
  void slotImportFeedsJson();
  void slotExportFeedsJson();
  void slotFeedsManagement();
  void slotCleanupDatabase();
  void slotFeedsChanged();
  void slotCheckStatus(QList<int> feedIds);
  void slotUndoLastMark();
  void slotShowStatistics();
  void slotShowAIDialog();
  void slotShowGroupSummary();
  void slotShowUnreadSummary();
  void slotShowDailyBriefing();
  void slotShowRecommendations();
  void slotShowDedup();
  void slotShowAutoTag();
  void slotShowKeywordTrends();
  void slotSaveProgress();
  void slotCheckDiskSpace();
  void slotCheckRssHubInstances();
  void slotFeedClicked(QModelIndex index);
  void slotFeedSelected(QModelIndex index, bool createTab = false);
  void setFeedsFilter(bool clicked = true);
  void slotGetFeed();
  void slotGetAllFeeds();
  void slotStopUpdate();
  void showProgressBar(int addToMaximum);
  void slotSetValue(int value);
  void showMessageStatusBar(QString message, int timeout = 0);
  void slotCountsStatusBar(int unreadCount, int allCount);
  void slotTaskStats(int queued, int running, int done, int failed);
  void slotCurrentFeed(int feedId, QString feedUrl);
  void slotPlaySound(const QString &path);
  void slotPlayerTogglePlay();
  void slotPlayerClose();
  void slotPlayerPositionChanged(qint64 position);
  void slotPlayerDurationChanged(qint64 duration);
  void slotPlayerSliderMoved(int position);
  void slotPlayerStateChanged();
  void slotAddColorList(int id, const QString &color);
  void showOptionDlg(int index = -1);
  void slotPlaceToTray();
  void slotActivationTray(QSystemTrayIcon::ActivationReason reason);
  void slotTrayOpenNotifyTimer();
  void showWindows(bool trayClick = false);
  void quitApp();
  void myEmptyWorkingSet();
  void slotUpdateFeed(int feedId, bool changed, int newCount, bool finish);
  void slotFeedCountsUpdate(FeedCountStruct counts);
  void slotUpdateNews(int refresh);
  void slotUpdateStatus(int feedId, bool changed = true);
  void setNewsFilter(QAction*, bool clicked = true);
  void slotCloseTab(int index);
  QWebEnginePage *createWebTab(QUrl url = QUrl());
  void feedsModelReload(bool checkFilter = false);
  void setStatusFeed(int feedId, QString status);
  void slotFeedUrlChanged(int feedId, const QString &newUrl);
  void slotPrint(QWebEnginePage *page = 0);
  void slotPrintPreview(QWebEnginePage* page = 0);
  // Qt5: QWebEnginePage::print(QPagedPaintDevice*); Qt6: printToPdf() ->
  // QPdfDocument rasterization. Both end up painting onto the QPrinter.
  void printWebPage(QWebEnginePage *page, QPrinter *printer);

signals:
  void signalQuitApp();
  void signalPlaceToTray();
  void signalGetFeedTimer(int feedId);
  void signalGetAllFeedsTimer();
  void signalGetFeed(int feedId, QString feedUrl, QDateTime date, int auth);
  void signalGetFeedsFolder(QString query);
  void signalGetAllFeeds();
  void signalStopUpdate();
  void signalImportFeeds(QByteArray xmlData);
  void signalRequestUrl(int feedId, QString urlString,
                        QDateTime date, QString userInfo, QString proxyUrl,
                        bool highPriority = false);
  void faviconRequestUrl(QString urlString, QString feedUrl);
  void signalIconFeedReady(QString feedUrl, QByteArray faviconData);
  void signalSetCurrentTab(int index, bool updateTab = false);
  void signalShowNotification(bool bShowRecentNews=false);
  void signalRefreshInfoTray();
  void signalNextUpdate(bool finish);
  void signalRecountCategoryCounts();
  void signalRecountFeedCounts(int feedId, bool update = true);
  void signalSetFeedRead(int readType, int feedId, int idException, QList<int> idNewsList);
  void signalPlaySoundNewNews();
  void signalUpdateStatus(int feedId, bool changed);
  void signalMarkAllFeedsRead();
  void signalMarkFeedRead(int id, bool isFolder, bool openFeed);
  void signalRefreshNewsView(int nextUnread);
  void signalSetFeedsFilter(bool clicked = false);
  void signalMarkAllFeedsOld();
  void signalMarkReadCategory(int type, int idLabel);

private slots:
  void showMainMenu();
  void slotTimerLinkOpening();
  void slotUnreadOnlyToggled(bool);
  void slotVisibledFeedsWidget();
  void updateIconToolBarNull(bool feedsWidgetVisible);
  void setFeedRead(int type, int feedId, FeedReedType feedReadType,
                   NewsTabWidget *widgetTab = 0, int idException = -1);
  void markFeedRead();
  void slotRecountCategoryCounts(QList<int> deletedList, QList<int> starredList,
                                 QList<int> readList, QStringList labelList);
  void slotFeedsViewportUpdate();
  void slotPlaySoundNewNews();

#ifdef HAVE_QT5
  void mediaStatusChanged(QMediaPlayer::MediaStatus status);
  void mediaError(QMediaPlayer::Error error);
#endif
 #ifdef HAVE_PHONON
  void mediaStateChanged(Phonon::State newstate, Phonon::State oldstate);
#endif

  void slotShowAboutDlg();

  void showContextMenuFeed(const QPoint & pos);
  void slotFeedsFilter();
  void slotNewsFilter();
  void slotGetFeedsTimer();
  void slotTogglePauseUpdates();
  void updateIntelligentRefreshInterval(int feedId);
  void slotShowUpdateAppDlg();
  void showContextMenuToolBar(const QPoint &pos);
  void showFeedPropertiesDlg();
  void slotFeedMenuShow();
  void slotRefreshNewsView(int nextUnread = -1);
  void slotIconFeedPreparing(QString feedUrl, QByteArray byteArray, QString format);
  void slotIconFeedUpdate(int feedId, QByteArray faviconData);
  void showNewsFiltersDlg(bool newFilter = false);
  void showFilterRulesDlg();
  void slotUpdateAppCheck();
  void slotNewVersion(const QString &newVersion);
  void slotFeedUpPressed();
  void slotFeedDownPressed();
  void slotFeedHomePressed();
  void slotFeedEndPressed();
  void slotFeedPageUpPressed();
  void slotFeedPageDownPressed();
  void slotFeedPrevious();
  void slotFeedNext();
  void setStyleApp(QAction*);
  void slotCheckSystemTheme();
  void applyDimReadSettings();
  void applyReduceMotionSettings();
  void slotSwitchFocus();
  void slotSwitchPrevFocus();
  void slotOpenFeedNewTab();
  void slotOpenCategoryNewTab();
  void slotTabCurrentChanged(int index);
  void slotTabMoved(int fromIndex, int toIndex);
  void feedsColumnVisible(QAction *action);
  void setNewsLayout(QAction *action);
  void setNewsLayout();
  void setBrowserPosition(QAction *action);
  void slotOpenNewsWebView();

  void slotNewsUpPressed();
  void slotNewsDownPressed();
  void slotNewsPageUpPressed();
  void slotNewsPageDownPressed();
  void markNewsRead();
  void markAllNewsRead();
  void markNewsStar();
  void deleteNews();
  void deleteAllNewsList();
  void restoreNews();
  void openInBrowserNews();
  void openInExternalBrowserNews();
  void slotOpenNewsNewTab();
  void slotOpenNewsBackgroundTab();
  void slotCopyLinkNews();
  void slotShowLabelsMenu();
  void slotPageUpWebView();
  void slotPageDownWebView();
  void setCurrentTab(int index, bool updateCurrentTab = false);

  void showNotification(bool bShowRecentNews=false);
  void deleteNotification();
  void clearNotification(bool bClearRecentNews=false);
  void slotOpenNew(int feedId, int newsId);
  void slotOpenNewBrowser(const QUrl &url);
  void slotMarkReadNewsInNotification(int feedId, int newsId, int read);
  void slotDeleteNewsInNotification(int feedId, int newsId);
  void slotMarkAllReadNewsInNotification();

  void slotFindFeeds(QString);
  void slotSelectFind();
  void findFeedVisible(bool visible);

  void browserZoom(QAction*);
  void slotReportProblem();

  void slotSavePageAs();
  void slotSavePageAsDescript();

  void setFullScreen();
  void setStayOnTop();
  void showMenuBar();

  void slotMoveIndex(const QModelIndex &indexWhere, int how);

  void slotRefreshInfoTray(int newCount, int unreadCount);

  void slotCategoriesClicked(QTreeWidgetItem *item, int, bool createTab = false);
  void clearDeleted();
  void slotMarkReadCategory(QTreeWidgetItem *item);
  void showNewsCategoriesTree();
  void feedsSplitterMoved(int pos, int);

  void setLabelNews(QAction *action);
  void setDefaultLabelNews();
  void getLabelNews();

  void lockMainToolbar(bool lock);
  void hideMainToolbar();

  void reduceNewsList();
  void increaseNewsList();

  void restoreLastNews();

  void nextUnreadNews();
  void prevUnreadNews();

  void slotToggleFocusMode();
  void slotUndoCloseTab();

  void slotIndentationFeedsTree();

  void customizeMainToolbar();
  void showCustomizeToolbarDlg(QAction *action);

  void slotShareNews(QAction *action);
  void showMenuShareNews();

  void slotOpenHomeFeed();
  void sortedByTitleFeedsTree();
  void sortedByUnreadFeedsTree();
  void sortedByUpdatedFeedsTree();
  void sortFeedsTree(const QString &orderBy);

  void showNewsMenu();
  void showNewsSortByMenu();
  void setNewsSortByColumn();

  void slotNextFolder();
  void slotPrevFolder();
  void slotExpandFolder();

  void showDownloadManager(bool activate = true);
  void updateInfoDownloads(const QString &text);

  void cleanUp();

  void showSettingPageLabels();

  void createBackup();
  void backupData();
  void restoreData();
  void showBlockedWordsDlg();

private:
  void closeEvent(QCloseEvent *event);
  bool eventFilter(QObject *obj, QEvent *event);
  void changeEvent(QEvent *event);

  void createFeedsWidget();
  void createNewsTab(int index);
  void createToolBarNull();
  void createActions();
  void createShortcut();
  void loadActionShortcuts();
  void saveActionShortcuts();
  void createMenu();
  void createToolBar();
  void createTrayMenu();
  void createStatusBar();
  void createTray();
  void createTabBarWidget();
  void createCentralWidget();
  void createPlayerBar();
  void loadSettingsFeeds();
  void retranslateStrings();
  void recountFeedCategories(const QList<int> &categoriesList);
  void creatFeedTab(int feedId, int feedParId);
  void initUpdateFeeds();
  void addOurFeed();

  int addTab(NewsTabWidget *widget);

  NewsModel *newsModel_;
#ifndef Q_OS_MAC
  QMenu *mainMenu_;
#endif
  ToolButton *mainMenuButton_;
  TabBar *tabBar_;
  QWidget *tabBarWidget_;
  QSplitter *mainSplitter_;
  QWidget *feedsPanel_;
  QFrame *feedsWidget_;
  QWidget *centralWidget_;

  QList<QAction *> listActions_;
  QStringList listDefaultShortcut_;

  QAction *addAct_;
  QAction *addFeedAct_;
  QAction *addFeedTrayAct_;
  QAction *addFolderAct_;
  QAction *deleteFeedAct_;
  QAction *importFeedsAct_;
  QAction *exportFeedsAct_;
  QAction *importFeedsJsonAct_;
  QAction *exportFeedsJsonAct_;
  QAction *feedsManagementAct_;
  QAction *undoLastMarkAct_;
  QAction *statisticsAct_;
  QAction *aiAssistantAct_;
  QAction *saveProgressAct_;
  QAction *mainToolbarToggle_;
  QAction *feedsToolbarToggle_;
  QAction *toolBarLockAct_;
  QAction *toolBarHideAct_;
  QAction *customizeMainToolbarAct_;
  QAction *customizeMainToolbarAct2_;
  QAction *customizeFeedsToolbarAct_;
  QAction *customizeNewsToolbarAct_;
  QAction *classicLayoutAct_;
  QAction *newspaperLayoutAct_;
  QAction *layoutToggle_;
  QAction *systemStyle_;
  QAction *lightStyle_;
  QAction *darkStyle_;
  QAction *topBrowserPositionAct_;
  QAction *bottomBrowserPositionAct_;
  QAction *rightBrowserPositionAct_;
  QAction *leftBrowserPositionAct_;
  QAction *showCleanUpWizardAct_;
  QAction *showDownloadManagerAct_;
  QAction *setNewsFiltersAct_;
  QAction *setFilterNewsAct_;
  QAction *optionsAct_;
  QAction *updateAllFeedsAct_;
  QAction *stopUpdateAct_;
  QAction *markAllFeedsRead_;
  QAction *indentationFeedsTreeAct_;
  QAction *sortedByTitleFeedsTreeAct_;
  QAction *sortFeedsByUnreadAct_;
  QAction *sortFeedsByUpdatedAct_;
  QAction *collapseAllFoldersAct_;
  QAction *expandAllFoldersAct_;
  QAction *exitAct_;
  QAction *feedsFilter_;
  QAction *filterFeedsAll_;
  QAction *filterFeedsNew_;
  QAction *filterFeedsUnread_;
  QAction *filterFeedsStarred_;
  QAction *filterFeedsError_;

  QAction *filterNewsAll_;
  QAction *filterNewsNew_;
  QAction *filterNewsUnread_;
  QAction *filterNewsStar_;
  QAction *filterNewsNotStarred_;
  QAction *filterNewsUnreadStar_;
  QAction *filterNewsLastDay_;
  QAction *filterNewsLastWeek_;
  QAction *aboutAct_;
  QAction *updateAppAct_;
  QAction *reportProblemAct_;

  QAction *markFeedRead_;
  QAction *feedProperties_;
  QAction *showWindowAct_;
  QAction *showRecentNotifyAct_;
  QAction *feedKeyUpAct_;
  QAction *feedKeyDownAct_;
  QAction *switchFocusAct_;
  QAction *switchFocusPrevAct_;
  QAction *feedsWidgetVisibleAct_;
  QAction *openFeedNewTabAct_;
  QAction *placeToTrayAct_;

  QAction *zoomInAct_;
  QAction *zoomOutAct_;
  QAction *zoomTo100Act_;

  QActionGroup *customizeToolbarGroup_;
  QActionGroup *layoutGroup_;
  QActionGroup *styleGroup_;
  QActionGroup *browserPositionGroup_;
  QActionGroup *feedsFilterGroup_;
  QActionGroup *browserZoomGroup_;
  QActionGroup *newsSortByColumnGroup_;
  QActionGroup *newsSortOrderGroup_;

  QAction *feedsFilterAction_; //!< Filter name storage, needed to enable last used filter
  QAction *newsFilterAction_;  //!< Filter name storege, needed to enable last used filter
  QString mainNewsFilter_;

  QAction *showUnreadCount_;
  QAction *showUndeleteCount_;
  QAction *showLastUpdated_;
  QActionGroup *feedsColumnsGroup_;

  QAction *findFeedAct_;
  QAction *fullScreenAct_;
  QAction *stayOnTopAct_;

  QAction *nextTabAct_;
  QAction *prevTabAct_;

  QAction *reduceNewsListAct_;
  QAction *increaseNewsListAct_;
  QAction *focusModeAct_;
  QAction *undoCloseTabAct_;

  QAction *createBackupAct_;
  QAction *backupDataAct_;
  QAction *restoreDataAct_;
  QAction *blockedWordsAct_;
  QAction *pauseUpdatesAct_;
  QAction *showMenuBarAct_;

  QMenu *fileMenu_;
  QMenu *newMenu_;
  QMenu *viewMenu_;
  QMenu *toolbarsMenu_;
  QMenu *customizeToolbarMenu_;
  QMenu *layoutMenu_;
  QMenu *styleMenu_;
  QMenu *browserPositionMenu_;
  QMenu *feedMenu_;
  QMenu *newsMenu_;
  QMenu *browserMenu_;
  QMenu *toolsMenu_;
  QMenu *helpMenu_;
  QMenu *trayMenu_;
  QMenu *newsContextMenu_;
  QMenu *feedsFilterMenu_;
  QMenu *newsFilterMenu_;
  QMenu *feedsColumnsMenu_;
  QMenu *browserZoomMenu_;

  QToolBar *mainToolbar_;
  QToolBar *feedsToolBar_;

  NewsView *newsView_;

  QProgressBar *progressBar_;
  QLabel *statusUnread_;
  QLabel *statusAll_;
  QLabel *statusUpdating_;
  int currentUpdatingFeedId_;
  QString currentUpdatingFeedName_;
  int updateQueueCount_;
  int updateRunningCount_;
  int updateDoneCount_;
  int updateFailedCount_;

  QPushButton *pushButtonNull_;

  QTimer *updateFeedsTimer_;
  // Runtime OS dark/light mode polling (only active in "System" style).
  QTimer *systemThemeTimer_;
  bool lastSystemDark_;
  int updateIntervalSec_;
  int updateTimeCount_;
  bool updateFeedsEnable_;
  bool updatePaused_;
  int  updateFeedsInterval_;
  int  updateFeedsIntervalType_;
  QList<int> feedIdList_;
  QMap<int,int> updateFeedsIntervalSec_;
  QMap<int,int> updateFeedsTimeCount_;

  bool minimizingTray_;
  bool closingTray_;
  bool singleClickTray_;
  bool clearStatusNew_;
  bool emptyWorking_;
  int behaviorIconTray_;

  int openingFeedAction_;
  bool openNewsWebViewOn_;

#if defined(HAVE_QT5) && !defined(QT6)
  QMediaPlayer *mediaPlayer_;
  QMediaPlaylist *playlist_;   // Qt5 only: QMediaPlaylist was removed in Qt6
  // Podcast: global player instance living at MainWindow scope so playback
  // survives tab switches / closings.
  QMediaPlayer *podcastPlayer_;
#elif defined(QT6)
  QMediaPlayer *mediaPlayer_;
  QMediaPlayer *podcastPlayer_;
#else
#ifdef HAVE_PHONON
  Phonon::MediaObject *mediaPlayer_;
  Phonon::AudioOutput *audioOutput_;
#endif
#endif

  // Podcast: global player bar (bottom of the central widget).
  QWidget *playerBarWidget_;
  QToolButton *playerPlayButton_;
  QLabel *playerTitleLabel_;
  QSlider *playerProgressSlider_;
  QLabel *playerTimeLabel_;
  QToolButton *playerCloseButton_;

  // Codex UI New Layout (Phase 2) - skeleton declarations only
  // These will be connected after compilation is fixed
  QFrame *navRailFrame_;
  QWidget *sidebarShell_;
  QWidget *rightPanel_;
  QSplitter *mainSplitterCodex_;
  QSplitter *sidebarSplitter_;
  QSplitter *contentSplitter_;
  bool codexLayoutEnabled_;
  int sidebarWidth_;
  int rightPanelWidth_;

  // ---- New control class pointers (declared, NOT yet instantiated/wired) ----
  // 等编译全绿后，在 createCodexLayout() 中 new 并接线
  NavRail                *navRail_                 = nullptr;  // 左侧导航栏
  SplitterHandle         *sidebarSplitterHandle_   = nullptr;  // 侧栏拖拽手柄
  SplitterHandle         *contentSplitterHandle_   = nullptr;  // 内容区拖拽手柄
  NewsCardDelegate       *newsCardDelegate_        = nullptr;  // 文章列表卡片代理
  RightPanel             *rightPanelWidget_        = nullptr;  // 右侧可折叠面板
  PlayerBar              *playerBarWidget_         = nullptr;  // 底部播放条
  CommandPalette         *commandPalette_          = nullptr;  // 命令面板
  ThemeManager           *themeManager_            = nullptr;  // 主题管理单例（可选持有指针）
  NavigationContext      *navigationContext_       = nullptr;  // 导航上下文
  SvgIconEngine          *faviconEngine_          = nullptr;  // 图标渲染引擎（可选持有）
  ReaderToolbar          *readerToolbar_           = nullptr;  // 阅读区工具栏

  bool soundNewNews_;
  QString soundNotifyPath_;
  bool playSoundNewNews_;
  bool showNotifyOn_;
  bool fullscreenModeNotify_;
  bool showNotifyInactiveApp_;
  bool onlySelectedFeeds_;
  bool quietHoursOn_;
  QTime quietHoursStart_;
  QTime quietHoursEnd_;

  UpdateAppDialog *updateAppDialog_;

  bool cleanupOnShutdown_;
  int maxDayCleanUp_;
  int maxNewsCleanUp_;
  bool dayCleanUpOn_;
  bool newsCleanUpOn_;
  bool readCleanUp_;
  bool neverUnreadCleanUp_;
  bool neverStarCleanUp_;
  bool neverLabelCleanUp_;
  bool cleanUpDeleted_;
  bool optimizeDB_;

  struct NewNewsData
  {
    QList<int> idFeedList_;
    QList<int> cntNewsList_;
  };

  NewNewsData newNews;
  NewNewsData recentNews;

  QList<int> idColorList_;
  QStringList colorList_;

  QTimer timerTrayOpenNotify;

  bool reopenFeedStartup_;
  bool openNewTabNextToActive_;

  bool updateCurrentTab_;

  NotificationWidget *notificationWidget;

  FindFeed *findFeeds_;
  QWidget *findFeedsWidget_;

  int feedIdOld_;

  int  openingLinkTimeout_;  //!< During this time we'll trying swithing back to apllication
  QTimer timerLinkOpening_;

  QWidget *categoriesPanel_;
  QLabel *categoriesLabel_;
  ToolButton *showCategoriesButton_;
  QWidget *categoriesWidget_;
  QSplitter *feedsSplitter_;
  QByteArray feedsWidgetSplitterState_;

  qint64 activationStateChangedTime_;

  bool isStartImportFeed_;

  bool changeBehaviorActionNUN_;

  bool recountCategoryCountsOn_;

  QAction *unreadOnlyAct_;

  OptionsDialog *optionsDialog_;
  QString optionsDialogLanguage_;

  AdBlockIcon* adblockIcon_;

};

#endif // MAINWINDOW_H
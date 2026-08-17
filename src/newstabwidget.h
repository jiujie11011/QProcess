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
#ifndef NEWSTABWIDGET_H
#define NEWSTABWIDGET_H

#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>
#include <QtWebEngineWidgets>
#include <QtWebChannel>

#include "feedsproxymodel.h"
#include "feedsmodel.h"
#include "feedsview.h"
#include "findtext.h"
#include "lineedit.h"
#include "locationbar.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "newsview.h"
#include "webview.h"
#include "interactivemark.h"
#include "groupbydateproxymodel.h"

class MainWindow;

#define TOP_POSITION    0
#define BOTTOM_POSITION 1
#define RIGHT_POSITION  2
#define LEFT_POSITION   3

#define RESIZESTEP 25   // News list/browser size step

class NewsTabWidget : public QWidget
{
  Q_OBJECT
public:
  enum TabType {
    TabTypeFeed,
    TabTypeUnread,
    TabTypeStar,
    TabTypeDel,
    TabTypeLabel,
    TabTypeWeb,
    TabTypeDownloads
  };

  enum RefreshNewspaper {
    RefreshAll,
    RefreshInsert,
    RefreshWithPos
  };

  explicit NewsTabWidget(QWidget *parent, TabType type, int feedId = -1, int feedParId = -1);
  ~NewsTabWidget();

  void disconnectObjects();

  void retranslateStrings();
  void setSettings(bool init = true, bool newTab = true);
  void setNewsLayout();
  void setBrowserPosition();
  void markNewsRead();
  void markAllNewsRead();
  void markNewsStar();
  void setLabelNews(int labelId);
  void deleteNews();
  void deleteAllNewsList();
  void restoreNews();
  void slotCopyLinkNews();
  void showLabelsMenu();
  void savePageAsDescript();

  bool openUrl(const QUrl &url);
  void openInBrowserNews();
  void openInExternalBrowserNews();
  void openNewsNewTab();

  void updateWebView(QModelIndex index);
  void loadNewspaper(int refresh = RefreshAll);
  void hideWebContent();
  // UI: attach native lazy loading to content <img> tags (Chromium >= 77).
  void enableImageLazyLoading(QString &html);
  QString getLinkNews(int row);

  // S-2: group news by date (Today / Yesterday / Earlier)
  void setGroupByDate(bool on);
  QModelIndex newsIndexToSource(const QModelIndex &index) const;
  QModelIndex newsIndexFromSource(const QModelIndex &index) const;

  void reduceNewsList();
  void increaseNewsList();

  int findUnreadNews(bool next);

  void setTextTab(const QString &text);

  void slotShareNews(QAction *action);

  /*! \brief Convert \a countString to unreadCount depending on \a type_
   * \param countString from categories tree
   * \return unreadCount for displaying in status
   */
  int getUnreadCount(QString countString);

  TabType type_;
  int feedId_;
  int feedParId_;
  int currentNewsIdOld;
  bool autoLoadImages_;
  int labelId_;
  QString categoryFilterStr_;

  FindTextContent *findText_;

  NewsModel *newsModel_;
  GroupByDateProxyModel *newsProxyModel_;
  NewsView *newsView_;
  NewsHeader *newsHeader_;
  QToolBar *newsToolBar_;
  QSplitter *newsTabWidgetSplitter_;
  InteractiveMarkController *interactiveMarkController_;

  QWidget *newsWidget_;
  WebView *webView_;
  QToolBar *webToolBar_;
  LocationBar *locationBar_;
  QWidget *webControlPanel_;

  QLabel *newsIconTitle_;
  QMovie *newsIconMovie_;
  QLabel *newsTextTitle_;
  QWidget *newsTitleLabel_;
  QToolButton *closeButton_;

  QAction *separatorRAct_;

public slots:
  void setAutoLoadImages(bool apply = true);
  void slotNewsViewClicked(QModelIndex index);
  void slotNewsViewSelected(QModelIndex index, bool clicked=false);
  void slotNewsViewDoubleClicked(QModelIndex index);
  void slotNewsMiddleClicked(QModelIndex index);
  void slotNewsUpPressed(QModelIndex index=QModelIndex());
  void slotNewsDownPressed(QModelIndex index=QModelIndex());
  void slotNewsHomePressed(QModelIndex index=QModelIndex());
  void slotNewsEndPressed(QModelIndex index=QModelIndex());
  void slotNewsPageUpPressed(QModelIndex index=QModelIndex());
  void slotNewsPageDownPressed(QModelIndex index=QModelIndex());
  void slotNewsNextUnreadPressed(QModelIndex index=QModelIndex());
  void slotNewsPrevUnreadPressed(QModelIndex index=QModelIndex());
  void slotSort(int column, int order);

  /** Shows/hides the news list (used by focus mode). */
  void setNewsListVisible(bool visible);

signals:
  void signalSetHtmlWebView(const QString &html = "", const QUrl &baseUrl = QUrl());
  void signalSetTextTab(const QString &text, NewsTabWidget *widget);
  void loadProgress(int);

private slots:
  void showContextMenuNews(const QPoint &pos);
  void slotSetItemRead(QModelIndex index, int read);
  void slotSetItemStar(QModelIndex index, int starred);
  void slotMarkReadTimeout();
  void slotNavigationRequested(const QUrl &url);
  void slotNewsListScrolled();
  void slotInteractiveMarkRead(int count);
  void slotAutoSummaryReady(int newsId, const QString &text);
  void slotAutoTranslationReady(int newsId, const QString &text, const QString &targetLang);
  void slotAutoRecommendationsReady(int newsId, const QString &content);
  void slotShowImageGallery();
  void slotImageCacheReady(int newsId, const QString &cachedHtml);
  void slotFetchFullText();
  void slotFullTextPageLoaded(bool ok);

  void slotSetHtmlWebView(const QString &html);
  void slotSaveArticleScroll();
  void webHomePage();
  void openPageInExternalBrowser();
  void slotLinkClicked(QUrl url);
  // QWebEnginePage::linkHovered carries 1 arg on Qt5 and 3 on Qt6; the slot
  // only uses the URL, so keep a single argument for cross-version connects.
  void slotLinkHovered(const QString &link);
  void slotSetValue(int value);
  void slotLoadStarted();
  void slotLoadFinished(bool);
  void slotUrlEnter();
  void slotUrlChanged(const QUrl &url);
  void showContextWebPage(const QPoint &p);
  void openUrlInExternalBrowser();

  void slotTabClose();
  void webTitleChanged(QString title);
  void openLink();
  void openLinkInNewTab();

  void slotFindText(const QString& text);
  void slotSelectFind();

  void setWebToolbarVisible(bool show = true, bool checked = true);

  void slotNewslLabelClicked(QModelIndex index);
  void slotRetryCurrentFeed();

private:
  void createNewsList();
  void createWebWidget();
  QModelIndex neighborNewsIndex(bool next, const QModelIndex &from = QModelIndex());
  QString getHtmlLabels(int row);
  void actionNewspaper(QUrl url);
  void maybeAutoSummarize(const QModelIndex &index, int newsId);
  void maybeAutoTranslate(const QModelIndex &index, int newsId);
  void maybeAutoRecommend(const QModelIndex &index, int newsId);

  // UI-3: article reading progress (WebView scroll position per news item)
  void saveArticleScrollAsync(int newsId);
  void storeArticleScroll(int newsId, int pos);
  int articleScrollFor(int newsId);
  void restoreArticleScroll(int newsId);

  // UI-5: inline error banner for failed feed updates
  void updateErrorBanner();

  MainWindow *mainWindow_;
  QSqlDatabase db_;

  FeedsModel *feedsModel_;
  FeedsProxyModel *feedsProxyModel_;
  FeedsView *feedsView_;

  QFrame *lineWebWidget;
  QWidget *webWidget_;
  QProgressBar *webViewProgress_;
  QLabel *webViewProgressLabel_;

  QAction *webHomePageAct_;
  QAction *webExternalBrowserAct_;
  QAction *urlExternalBrowserAct_;

  QTimer *markNewsReadTimer_;

  int webDefaultFontSize_;
  int webDefaultFixedFontSize_;

  /*! Id of the news currently rendered in webView_, used to route async
   *  AI translation/summary results back to the correct article. */
  int currentShownNewsId_;

  /*! News id waiting for scroll restoration after the next page load. */
  int pendingRestoreNewsId_;
  /*! In-memory cache of saved WebView scroll positions (newsId -> y). */
  QHash<int,int> articleScrollCache_;
  /*! Periodic timer that captures the current article scroll position. */
  QTimer *articleScrollTimer_;
  /*! False between loadStarted() and loadFinished(); the periodic scroll
   *  capture is suppressed while a page is still loading, otherwise the old
   *  page's offset would be recorded against the new article id. */
  bool articlePageLoaded_;

  QUrl linkUrl_;
  QString linkNewsString_;

  QWidget *newsPanelWidget_;
  bool webToolbarShow_;

  // UI-5: inline error banner for failed feed updates
  QWidget *errorBanner_;
  QLabel *errorBannerIcon_;
  QLabel *errorBannerLabel_;
  QToolButton *errorBannerRetryButton_;

  QString newspaperHeadHtml_;
  QString newspaperHtml_;
  QString newspaperHtmlRtl_;
  QString htmlString_;
  QString htmlRtlString_;
  QString cssString_;
  QString audioPlayerHtml_;
  QString videoPlayerHtml_;

  // Full-text fetch: background page used to run the readability extractor.
  QWebEnginePage *fullTextPage_;
  int fetchFullTextNewsId_;
  int fetchFullTextFeedId_;

};

#endif // NEWSTABWIDGET_H

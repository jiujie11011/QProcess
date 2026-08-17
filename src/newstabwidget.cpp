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
#include "newstabwidget.h"

#include "mainapplication.h"
#include "ftssearch.h"
#include "htmlsanitizer.h"
#include "adblockicon.h"
#include "settings.h"
#include "webpage.h"
#include "localsummary.h"
#include "imagegallerydialog.h"
#include "imagecache.h"
#include "newsview/newstitledelegate.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QPointer>
#include <QRegularExpression>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif
#include <qzregexp.h>

NewsTabWidget::NewsTabWidget(QWidget *parent, TabType type, int feedId, int feedParId)
  : QWidget(parent)
  , type_(type)
  , feedId_(feedId)
  , feedParId_(feedParId)
  , currentNewsIdOld(-1)
  , autoLoadImages_(true)
  , currentShownNewsId_(-1)
  , pendingRestoreNewsId_(-1)
  , articlePageLoaded_(false)
  , fullTextPage_(NULL)
  , fetchFullTextNewsId_(-1)
  , fetchFullTextFeedId_(-1)
{
  mainWindow_ = mainApp->mainWindow();
  db_ = QSqlDatabase::database();
  feedsView_ = mainWindow_->feedsView_;
  feedsModel_ = mainWindow_->feedsModel_;
  feedsProxyModel_ = mainWindow_->feedsProxyModel_;

  // UI-5: keep the inline error banner in sync with feed status changes
  connect(feedsModel_, &QAbstractItemModel::dataChanged, this,
          [this](const QModelIndex &, const QModelIndex &,
                 const QVector<int> &) {
            updateErrorBanner();
          });

  // Offline image cache: re-render the current article when the images for
  // the article being shown have finished downloading.
  connect(mainApp->imageCache(), &ImageCacheManager::contentReady,
          this, &NewsTabWidget::slotImageCacheReady);

  newsIconTitle_ = new QLabel();
  newsIconMovie_ = new QMovie(":/images/loading");
  newsIconTitle_->setMovie(newsIconMovie_);
  newsTextTitle_ = new QLabel();
  newsTextTitle_->setObjectName("newsTextTitle_");

  closeButton_ = new QToolButton();
  closeButton_->setFixedSize(15, 15);
  closeButton_->setCursor(Qt::ArrowCursor);
  closeButton_->setStyleSheet(
        "QToolButton { background-color: transparent;"
        "border: none; padding: 0px;"
        "image: url(:/images/close); }"
        "QToolButton:hover {"
        "image: url(:/images/closeHover); }"
        );
  connect(closeButton_, &QAbstractButton::clicked,
          this, &NewsTabWidget::slotTabClose);

  QHBoxLayout *newsTitleLayout = new QHBoxLayout();
  newsTitleLayout->setContentsMargins(0, 0, 0, 0);
  newsTitleLayout->setSpacing(0);
  newsTitleLayout->addWidget(newsIconTitle_);
  newsTitleLayout->addSpacing(3);
  newsTitleLayout->addWidget(newsTextTitle_, 1);
  newsTitleLayout->addWidget(closeButton_);

  newsTitleLabel_ = new QWidget();
  newsTitleLabel_->setObjectName("newsTitleLabel_");
  newsTitleLabel_->setMinimumHeight(16);
  newsTitleLabel_->setLayout(newsTitleLayout);
  newsTitleLabel_->setVisible(false);

  Settings settings;
  bool showCloseButtonTab = settings.value("Settings/showCloseButtonTab", true).toBool();
  if (!showCloseButtonTab) {
    closeButton_->hide();
    newsTitleLabel_->setFixedWidth(MAX_TAB_WIDTH-15);
  } else {
    newsTitleLabel_->setFixedWidth(MAX_TAB_WIDTH);
  }

  if (type_ != TabTypeDownloads) {
    if (type_ != TabTypeWeb) {
      createNewsList();
    } else {
      autoLoadImages_ = mainWindow_->autoLoadImages_;
    }
    createWebWidget();

    if (type_ != TabTypeWeb) {
      newsTabWidgetSplitter_ = new QSplitter(this);
      newsTabWidgetSplitter_->setObjectName("newsTabWidgetSplitter");

      if ((mainWindow_->browserPosition_ == TOP_POSITION) ||
          (mainWindow_->browserPosition_ == LEFT_POSITION)) {
        newsTabWidgetSplitter_->addWidget(webWidget_);
        newsTabWidgetSplitter_->addWidget(newsWidget_);
      } else {
        newsTabWidgetSplitter_->addWidget(newsWidget_);
        newsTabWidgetSplitter_->addWidget(webWidget_);
      }
    }
  }

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  if (type_ == TabTypeDownloads)
    layout->addWidget(mainApp->downloadManager());
  else if (type_ != TabTypeWeb)
    layout->addWidget(newsTabWidgetSplitter_);
  else
    layout->addWidget(webWidget_);
  setLayout(layout);

  if (type_ < TabTypeWeb) {
    newsTabWidgetSplitter_->setHandleWidth(1);

    if ((mainWindow_->browserPosition_ == RIGHT_POSITION) ||
        (mainWindow_->browserPosition_ == LEFT_POSITION)) {
      newsTabWidgetSplitter_->setOrientation(Qt::Horizontal);
      newsTabWidgetSplitter_->setStyleSheet(
            QString("QSplitter::handle {background: qlineargradient("
                    "x1: 0, y1: 0, x2: 0, y2: 1,"
                    "stop: 0 %1, stop: 0.07 %2);}").
            arg(newsPanelWidget_->palette().window().color().name()).
            arg(qApp->palette().color(QPalette::Dark).name()));
    } else {
      newsTabWidgetSplitter_->setOrientation(Qt::Vertical);
      newsTabWidgetSplitter_->setStyleSheet(
            QString("QSplitter::handle {background: %1; margin-top: 1px; margin-bottom: 1px;}").
            arg(qApp->palette().color(QPalette::Dark).name()));
    }
  }

  connect(this, &NewsTabWidget::signalSetTextTab,
          mainWindow_, &MainWindow::setTextTitle);

  if (mainWindow_->aiAssistant_) {
    connect(mainWindow_->aiAssistant_, &AIAssistant::summaryReady,
            this, &NewsTabWidget::slotAutoSummaryReady);
    connect(mainWindow_->aiAssistant_, &AIAssistant::translationReady,
            this, &NewsTabWidget::slotAutoTranslationReady);
    connect(mainWindow_->aiAssistant_, &AIAssistant::recommendationsReady,
            this, &NewsTabWidget::slotAutoRecommendationsReady);
  }

  if (mainWindow_->translationService_) {
    connect(mainWindow_->translationService_,
            &TranslationService::translationReady,
            this, &NewsTabWidget::slotAutoTranslationReady);
  }
}

NewsTabWidget::~NewsTabWidget()
{
  if (type_ == TabTypeDownloads) {
    mainApp->downloadManager()->hide();
    mainApp->downloadManager()->setParent(mainWindow_);
  }
}

void NewsTabWidget::disconnectObjects()
{
  // UI-3: persist the final scroll position before the tab is torn down —
  // the 3-second poll may never fire again.
  if ((type_ < TabTypeWeb) && articlePageLoaded_ &&
      (currentShownNewsId_ > 0)) {
    saveArticleScrollAsync(currentShownNewsId_);
  }
  disconnect(this);
  if (type_ != TabTypeDownloads) {
    webView_->disconnect(this);
    webView_->disconnectObjects();
    qobject_cast<WebPage*>(webView_->page())->disconnectObjects();
  }
  // Full-text fetch page is only used transiently; release it with the tab.
  if (fullTextPage_) {
    fullTextPage_->deleteLater();
    fullTextPage_ = 0;
  }
}

/** @brief Create news list with all related panels
 *----------------------------------------------------------------------------*/
void NewsTabWidget::createNewsList()
{
  newsView_ = new NewsView(this);
  newsView_->setFrameStyle(QFrame::NoFrame);
  newsModel_ = new NewsModel(this, newsView_);
  newsModel_->setTable("news");
  newsModel_->setFilter("feedId=-1");
  newsHeader_ = new NewsHeader(newsModel_, newsView_);

  // S-2: date grouping proxy (enabled on demand)
  newsProxyModel_ = new GroupByDateProxyModel(this);
  newsProxyModel_->setDateColumn(newsModel_->fieldIndex("published"));
  newsProxyModel_->setSourceModel(newsModel_);

  newsView_->setModel(newsModel_);
  newsView_->setHeader(newsHeader_);

  // Render AI summaries as a second line under the title.
  newsView_->setItemDelegateForColumn(
        newsModel_->fieldIndex("title"),
        new NewsTitleDelegate(newsView_));

  connect(newsView_->verticalScrollBar(), &QAbstractSlider::valueChanged,
          this, &NewsTabWidget::slotNewsListScrolled);

  newsHeader_->init();

  newsToolBar_ = new QToolBar(this);
  newsToolBar_->setObjectName("newsToolBar");
  newsToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");

  Settings settings;
  QString actionListStr = "markNewsRead,markAllNewsRead,Separator,markStarAct,"
                          "newsLabelAction,shareMenuAct,openInExternalBrowserAct,Separator,"
                          "nextUnreadNewsAct,prevUnreadNewsAct,Separator,"
                          "newsFilter,Separator,deleteNewsAct";
  QString str = settings.value("Settings/newsToolBar", actionListStr).toString();

  foreach (QString actionStr, str.split(",", Qt::SkipEmptyParts)) {
    if (actionStr == "Separator") {
      newsToolBar_->addSeparator();
    } else {
      QListIterator<QAction *> iter(mainWindow_->actions());
      while (iter.hasNext()) {
        QAction *pAction = iter.next();
        if (!pAction->icon().isNull()) {
          if (pAction->objectName() == actionStr) {
            newsToolBar_->addAction(pAction);
            break;
          }
        }
      }
    }
  }
  separatorRAct_ = newsToolBar_->addSeparator();
  separatorRAct_->setObjectName("separatorRAct");
  newsToolBar_->addAction(mainWindow_->restoreNewsAct_);

  findText_ = new FindTextContent(this);
  findText_->setFixedWidth(200);

  QHBoxLayout *newsPanelLayout = new QHBoxLayout();
  newsPanelLayout->setContentsMargins(2, 2, 2, 2);
  newsPanelLayout->setSpacing(2);
  newsPanelLayout->addWidget(newsToolBar_);
  newsPanelLayout->addStretch(1);
  newsPanelLayout->addWidget(findText_);

  newsPanelWidget_ = new QWidget(this);
  newsPanelWidget_->setObjectName("newsPanelWidget_");
  newsPanelWidget_->setStyleSheet(
        QString("#newsPanelWidget_ {border-bottom: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));

  newsPanelWidget_->setLayout(newsPanelLayout);
  if (!mainWindow_->newsToolbarToggle_->isChecked())
    newsPanelWidget_->hide();

  // UI-5: inline error banner shown when the current feed failed to update
  errorBanner_ = new QWidget(this);
  errorBanner_->setObjectName("errorBanner_");
  errorBanner_->setStyleSheet(
        QString("#errorBanner_ {background: %1; color: %2; border-bottom: 1px solid %3;}")
        .arg(qApp->palette().color(QPalette::ToolTipBase).name())
        .arg(qApp->palette().color(QPalette::ToolTipText).name())
        .arg(qApp->palette().color(QPalette::Dark).name()));
  QHBoxLayout *bannerLayout = new QHBoxLayout();
  bannerLayout->setContentsMargins(4, 4, 4, 4);
  bannerLayout->setSpacing(6);
  errorBannerIcon_ = new QLabel();
  errorBannerIcon_->setPixmap(QPixmap(":/images/bulletError"));
  errorBannerLabel_ = new QLabel();
  errorBannerLabel_->setWordWrap(true);
  errorBannerRetryButton_ = new QToolButton();
  errorBannerRetryButton_->setText(tr("Retry"));
  errorBannerRetryButton_->setAutoRaise(true);
  connect(errorBannerRetryButton_, &QAbstractButton::clicked,
          this, &NewsTabWidget::slotRetryCurrentFeed);
  bannerLayout->addWidget(errorBannerIcon_);
  bannerLayout->addWidget(errorBannerLabel_, 1);
  bannerLayout->addWidget(errorBannerRetryButton_);
  errorBanner_->setLayout(bannerLayout);
  errorBanner_->hide();

  QVBoxLayout *newsLayout = new QVBoxLayout();
  newsLayout->setContentsMargins(0, 0, 0, 0);
  newsLayout->setSpacing(0);
  newsLayout->addWidget(newsPanelWidget_);
  newsLayout->addWidget(errorBanner_);
  newsLayout->addWidget(newsView_);

  newsWidget_ = new QWidget(this);
  newsWidget_->setLayout(newsLayout);

  markNewsReadTimer_ = new QTimer(this);

  // UI-3: periodically capture the article scroll position
  articleScrollTimer_ = new QTimer(this);
  articleScrollTimer_->setInterval(3000);
  connect(articleScrollTimer_, &QTimer::timeout,
          this, &NewsTabWidget::slotSaveArticleScroll);
  articleScrollTimer_->start();

  QFile htmlFile;
  htmlFile.setFileName(":/html/newspaper_head");
  htmlFile.open(QFile::ReadOnly);
  newspaperHeadHtml_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();
  htmlFile.setFileName(":/html/newspaper_description");
  htmlFile.open(QFile::ReadOnly);
  newspaperHtml_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();
  htmlFile.setFileName(":/html/newspaper_description_rtl");
  htmlFile.open(QFile::ReadOnly);
  newspaperHtmlRtl_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();
  htmlFile.setFileName(":/html/description");
  htmlFile.open(QFile::ReadOnly);
  htmlString_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();
  htmlFile.setFileName(":/html/description_rtl");
  htmlFile.open(QFile::ReadOnly);
  htmlRtlString_ = QString::fromUtf8(htmlFile.readAll());
  htmlFile.close();

  connect(newsView_, &QAbstractItemView::pressed,
          this, &NewsTabWidget::slotNewsViewClicked);
  connect(newsView_, &NewsView::pressKeyUp,
          this, &NewsTabWidget::slotNewsUpPressed);
  connect(newsView_, &NewsView::pressKeyDown,
          this, &NewsTabWidget::slotNewsDownPressed);
  connect(newsView_, &NewsView::pressKeyHome,
          this, &NewsTabWidget::slotNewsHomePressed);
  connect(newsView_, &NewsView::pressKeyEnd,
          this, &NewsTabWidget::slotNewsEndPressed);
  connect(newsView_, &NewsView::pressKeyPageUp,
          this, &NewsTabWidget::slotNewsPageUpPressed);
  connect(newsView_, &NewsView::pressKeyPageDown,
          this, &NewsTabWidget::slotNewsPageDownPressed);
  connect(newsView_, &NewsView::pressKeyNextUnread,
          this, &NewsTabWidget::slotNewsNextUnreadPressed);
  connect(newsView_, &NewsView::pressKeyPrevUnread,
          this, &NewsTabWidget::slotNewsPrevUnreadPressed);
  connect(newsView_, &NewsView::signalSetItemRead,
          this, &NewsTabWidget::slotSetItemRead);
  connect(newsView_, &NewsView::signalSetItemStar,
          this, &NewsTabWidget::slotSetItemStar);
  connect(newsView_, &NewsView::signalDoubleClicked,
          this, &NewsTabWidget::slotNewsViewDoubleClicked);
  connect(newsView_, &NewsView::signalMiddleClicked,
          this, &NewsTabWidget::slotNewsMiddleClicked);
  connect(newsView_, &NewsView::signaNewslLabelClicked,
          this, &NewsTabWidget::slotNewslLabelClicked);
  connect(markNewsReadTimer_, &QTimer::timeout,
          this, &NewsTabWidget::slotMarkReadTimeout);
  connect(newsView_, &QWidget::customContextMenuRequested,
          this, &NewsTabWidget::showContextMenuNews);

  interactiveMarkController_ = new InteractiveMarkController(newsView_, newsModel_, db_, this);
  connect(newsView_, &NewsView::signalHoverRowChanged,
          interactiveMarkController_, &InteractiveMarkController::slotHoverRowChanged);
  connect(newsView_, &NewsView::signalRowsScrolledOut,
          interactiveMarkController_, &InteractiveMarkController::slotRowsScrolledOut);
  connect(newsModel_, &QAbstractItemModel::rowsInserted,
          interactiveMarkController_, &InteractiveMarkController::slotRowsInserted);
  connect(interactiveMarkController_, &InteractiveMarkController::rowsMarkedRead,
          this, &NewsTabWidget::slotInteractiveMarkRead);

  connect(newsModel_, &NewsModel::signalSort,
          this, &NewsTabWidget::slotSort);

  connect(findText_, &QLineEdit::textChanged,
          this, &NewsTabWidget::slotFindText);
  connect(findText_, &FindTextContent::signalSelectFind,
          this, &NewsTabWidget::slotSelectFind);
  connect(findText_, &QLineEdit::returnPressed,
          this, &NewsTabWidget::slotSelectFind);
  connect(findText_, &FindTextContent::signalVisible,
          mainWindow_, &MainWindow::findText);

  connect(mainWindow_->newsToolbarToggle_, &QAction::toggled,
          newsPanelWidget_, &QWidget::setVisible);
}

/** @brief Call context menu of selected news in news list
 *----------------------------------------------------------------------------*/
void NewsTabWidget::showContextMenuNews(const QPoint &pos)
{
  if (!newsView_->currentIndex().isValid()) return;

  QMenu menu;
  menu.addAction(mainWindow_->restoreNewsAct_);
  menu.addSeparator();
  menu.addAction(mainWindow_->openInBrowserAct_);
  menu.addAction(mainWindow_->openInExternalBrowserAct_);
  menu.addAction(mainWindow_->openNewsNewTabAct_);
  menu.addSeparator();
  QAction *fetchFullTextAct = menu.addAction(tr("Fetch Full Text"));
  connect(fetchFullTextAct, &QAction::triggered,
          this, &NewsTabWidget::slotFetchFullText);
  menu.addSeparator();
  menu.addAction(mainWindow_->markNewsRead_);
  {
    // Mark above/below as read, relative to the row under the cursor
    QModelIndex index = newsView_->indexAt(pos);
    if (!index.isValid())
      index = newsView_->currentIndex();
    int sourceRow = -1;
    if (newsView_->model() == newsProxyModel_) {
      QModelIndex src = newsProxyModel_->mapToSource(index);
      if (src.isValid())
        sourceRow = src.row();
    } else {
      sourceRow = index.row();
    }
    if (sourceRow >= 0) {
      QAction *markAboveAct = menu.addAction(tr("Mark News Above as Read"));
      QAction *markBelowAct = menu.addAction(tr("Mark News Below as Read"));
      connect(markAboveAct, &QAction::triggered, this,
              [this, sourceRow]() { interactiveMarkController_->markAboveRead(sourceRow); });
      connect(markBelowAct, &QAction::triggered, this,
              [this, sourceRow]() { interactiveMarkController_->markBelowRead(sourceRow); });
    }
  }
  menu.addAction(mainWindow_->markAllNewsRead_);
  menu.addSeparator();
  menu.addAction(mainWindow_->markStarAct_);
  menu.addAction(mainWindow_->newsLabelMenuAction_);
  menu.addAction(mainWindow_->shareMenuAct_);
  menu.addAction(mainWindow_->copyLinkAct_);
  menu.addSeparator();
  menu.addAction(mainWindow_->updateFeedAct_);
  menu.addSeparator();
  menu.addAction(mainWindow_->deleteNewsAct_);
  menu.addAction(mainWindow_->deleteAllNewsAct_);

  menu.exec(newsView_->viewport()->mapToGlobal(pos));
}

/** @brief Create web-widget and control panel
 *----------------------------------------------------------------------------*/
void NewsTabWidget::createWebWidget()
{
  webView_ = new WebView(this);

  webViewProgress_ = new QProgressBar(this);
  webViewProgress_->setObjectName("webViewProgress_");
  webViewProgress_->setFixedHeight(15);
  webViewProgress_->setMinimum(0);
  webViewProgress_->setMaximum(100);
  webViewProgress_->setVisible(true);
  connect(this, &NewsTabWidget::loadProgress,
          webViewProgress_, &QProgressBar::setValue, Qt::QueuedConnection);

  webViewProgressLabel_ = new QLabel(this);
  webViewProgressLabel_->setObjectName("webViewProgressLabel_");
  webViewProgressLabel_->setStyleSheet("background: none;");
  QHBoxLayout *progressLayout = new QHBoxLayout();
  progressLayout->setContentsMargins(0, 0, 0, 0);
  progressLayout->addWidget(webViewProgressLabel_, 0, Qt::AlignLeft|Qt::AlignVCenter);
  webViewProgress_->setLayout(progressLayout);

  //! Create web control panel
  webToolBar_ = new QToolBar(this);
  webToolBar_->setStyleSheet("QToolBar { border: none; padding: 0px; }");
  webToolBar_->setIconSize(QSize(18, 18));

  webHomePageAct_ = new QAction(this);
  webHomePageAct_->setIcon(QIcon(":/images/homePage"));

  webToolBar_->addAction(webHomePageAct_);
  QAction *webAction = webView_->pageAction(QWebEnginePage::Back);
  webToolBar_->addAction(webAction);
  webAction = webView_->pageAction(QWebEnginePage::Forward);
  webToolBar_->addAction(webAction);
  webAction = webView_->pageAction(QWebEnginePage::Reload);
  webToolBar_->addAction(webAction);
  webAction = webView_->pageAction(QWebEnginePage::Stop);
  webToolBar_->addAction(webAction);
  webToolBar_->addSeparator();

  webToolBar_->addAction(mainApp->mainWindow()->shareMenuAct_);

  webExternalBrowserAct_ = new QAction(this);
  webExternalBrowserAct_->setIcon(QIcon(":/images/openBrowser"));
  webToolBar_->addAction(webExternalBrowserAct_);

  QAction *galleryAct_ = new QAction(this);
  galleryAct_->setIcon(QIcon(":/images/imagesOn"));
  galleryAct_->setToolTip(tr("Image gallery"));
  webToolBar_->addAction(galleryAct_);
  connect(galleryAct_, &QAction::triggered,
          this, &NewsTabWidget::slotShowImageGallery);

  locationBar_ = new LocationBar(webView_, this);

  QHBoxLayout *webControlPanelLayout = new QHBoxLayout();
  webControlPanelLayout->setContentsMargins(2, 2, 2, 2);
  webControlPanelLayout->setSpacing(2);
  webControlPanelLayout->addWidget(webToolBar_);
  webControlPanelLayout->addWidget(locationBar_, 1);

  webControlPanel_ = new QWidget(this);
  webControlPanel_->setObjectName("webControlPanel_");
  webControlPanel_->setStyleSheet(
        QString("#webControlPanel_ {border-bottom: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));
  webControlPanel_->setLayout(webControlPanelLayout);

  if (type_ != TabTypeWeb)
    setWebToolbarVisible(false, false);
  else
    setWebToolbarVisible(true, false);

  //! Create web layout
  QVBoxLayout *webLayout = new QVBoxLayout();
  webLayout->setContentsMargins(0, 0, 0, 0);
  webLayout->setSpacing(0);
  webLayout->addWidget(webControlPanel_);
  webLayout->addWidget(webView_, 1);
  webLayout->addWidget(webViewProgress_);

  webWidget_ = new QWidget(this);
  webWidget_->setObjectName("webWidget_");
  webWidget_->setLayout(webLayout);
  webWidget_->setMinimumWidth(400);
  webWidget_->setMinimumHeight(100);

  urlExternalBrowserAct_ = new QAction(this);
  urlExternalBrowserAct_->setIcon(QIcon(":/images/openBrowser"));

  connect(webHomePageAct_, &QAction::triggered,
          this, &NewsTabWidget::webHomePage);
  connect(webExternalBrowserAct_, &QAction::triggered,
          this, &NewsTabWidget::openPageInExternalBrowser);
  connect(urlExternalBrowserAct_, &QAction::triggered,
          this, &NewsTabWidget::openUrlInExternalBrowser);
  connect(this, &NewsTabWidget::signalSetHtmlWebView,
          this, &NewsTabWidget::slotSetHtmlWebView, Qt::QueuedConnection);
  connect(webView_, &QWebEngineView::loadStarted,
          this, &NewsTabWidget::slotLoadStarted);
  connect(webView_, &QWebEngineView::loadFinished,
          this, &NewsTabWidget::slotLoadFinished);
  // Note: link clicks are handled through WebPage::navigationRequested
  // (see below); QWebEngineView/QWebEnginePage have no linkClicked signal.
  connect(webView_->page(), &QWebEnginePage::linkHovered,
          this, &NewsTabWidget::slotLinkHovered);
  // S-4: warn before jumping to external links
  connect((WebPage*)webView_->page(), &WebPage::navigationRequested,
          this, &NewsTabWidget::slotNavigationRequested);
  connect(webView_, &QWebEngineView::loadProgress,
          this, &NewsTabWidget::slotSetValue, Qt::QueuedConnection);

  connect(webView_, &QWebEngineView::titleChanged,
          this, &NewsTabWidget::webTitleChanged);

  connect(webView_, &WebView::showContextMenu,
          this, &NewsTabWidget::showContextWebPage, Qt::QueuedConnection);
  connect(webView_, &WebView::signalGoHome,
          this, &NewsTabWidget::webHomePage);

  connect(mainWindow_->autoLoadImagesToggle_, &QAction::triggered,
          this, &NewsTabWidget::setAutoLoadImages);
  connect(mainWindow_->browserToolbarToggle_, &QAction::triggered,
          this, [this]() { setWebToolbarVisible(); });

  connect(locationBar_, &QLineEdit::returnPressed,
          this, &NewsTabWidget::slotUrlEnter);
  connect(webView_, &WebView::rssChanged,
          locationBar_, &LocationBar::showRssIcon);
  connect(webView_, &QWebEngineView::urlChanged,
          this, &NewsTabWidget::slotUrlChanged, Qt::QueuedConnection);
}

/** @brief Enable/disable date grouping in the news list (S-2)
 *---------------------------------------------------------------------------*/
void NewsTabWidget::setGroupByDate(bool on)
{
  if (type_ >= TabTypeWeb)
    return;
  if (on) {
    if (newsView_->model() != newsProxyModel_) {
      newsProxyModel_->setSourceModel(newsModel_);
      newsView_->setModel(newsProxyModel_);
      newsView_->expandAll();
    }
  } else {
    if (newsView_->model() != newsModel_)
      newsView_->setModel(newsModel_);
  }
}

/** @brief Convert a view index (possibly from the grouping proxy) to the
 *         underlying news model index (S-2)
 *---------------------------------------------------------------------------*/
QModelIndex NewsTabWidget::newsIndexToSource(const QModelIndex &index) const
{
  if (index.isValid() && newsView_->model() == newsProxyModel_) {
    QModelIndex src = newsProxyModel_->mapToSource(index);
    if (src.isValid())
      return src;
  }
  return index;
}

/** @brief Convert a source news model index back to the view index,
 *         mapping through the grouping proxy when enabled (S-2)
 *---------------------------------------------------------------------------*/
QModelIndex NewsTabWidget::newsIndexFromSource(const QModelIndex &index) const
{
  if (index.isValid() && newsView_->model() == newsProxyModel_) {
    QModelIndex proxyIdx = newsProxyModel_->mapFromSource(index);
    if (proxyIdx.isValid())
      return proxyIdx;
  }
  return index;
}

/** @brief Return the previous/next news index in the active view model,
 *         skipping group header rows when grouping is enabled (S-2)
 *---------------------------------------------------------------------------*/
QModelIndex NewsTabWidget::neighborNewsIndex(bool next, const QModelIndex &from)
{
  QAbstractItemModel *model = newsView_->model();
  if (!model)
    return QModelIndex();

  if (model == newsModel_) {
    int row = from.isValid() ? from.row() : newsView_->currentIndex().row();
    if (row < 0)
      row = 0;
    row += next ? 1 : -1;
    if (row < 0 || row >= newsModel_->rowCount())
      return QModelIndex();
    return newsModel_->index(row, newsModel_->fieldIndex("title"));
  }

  if (model == newsProxyModel_) {
    QModelIndex cur = from.isValid() ? from : newsView_->currentIndex();
    if (!cur.isValid()) {
      QModelIndex firstGroup = newsProxyModel_->index(0, 0);
      if (!firstGroup.isValid())
        return QModelIndex();
      return newsProxyModel_->index(0, newsModel_->fieldIndex("title"), firstGroup);
    }
    if (!cur.parent().isValid()) {
      // group header row: jump into first/last leaf of the group
      int cnt = newsProxyModel_->rowCount(cur);
      if (cnt == 0)
        return QModelIndex();
      int leaf = next ? 0 : cnt - 1;
      return newsProxyModel_->index(leaf, newsModel_->fieldIndex("title"), cur);
    }
    QModelIndex parentIdx = cur.parent();
    int row = cur.row() + (next ? 1 : -1);
    if (row >= 0 && row < newsProxyModel_->rowCount(parentIdx))
      return newsProxyModel_->index(row, newsModel_->fieldIndex("title"), parentIdx);
    // move to the adjacent group
    int g = parentIdx.row() + (next ? 1 : -1);
    if (g < 0 || g >= newsProxyModel_->rowCount())
      return QModelIndex();
    QModelIndex ng = newsProxyModel_->index(g, 0);
    int cnt = newsProxyModel_->rowCount(ng);
    if (cnt == 0)
      return QModelIndex();
    int leaf = next ? 0 : cnt - 1;
    return newsProxyModel_->index(leaf, newsModel_->fieldIndex("title"), ng);
  }

  return QModelIndex();
}

/** @brief Read settings from ini-file
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setSettings(bool init, bool newTab)
{
  Settings settings;

  if (type_ == TabTypeDownloads) return;

  QString style = settings.value("Settings/styleApplication", "defaultStyle_").toString();
  if (style == "darkStyle_")
    newsIconMovie_->setFileName(":/images/loading_dark");
  else
    newsIconMovie_->setFileName(":/images/loading");

  if (newTab) {
    if (type_ < TabTypeWeb) {
      newsTabWidgetSplitter_->restoreState(settings.value("NewsTabSplitterState").toByteArray());
      QString iconStr = settings.value("Settings/newsToolBarIconSize", "toolBarIconSmall_").toString();
      mainWindow_->setToolBarIconSize(newsToolBar_, iconStr);

      newsView_->setFont(
            QFont(mainWindow_->newsListFontFamily_, mainWindow_->newsListFontSize_));
      newsModel_->formatDate_ = mainWindow_->formatDate_;
      newsModel_->formatTime_ = mainWindow_->formatTime_;
      newsModel_->simplifiedDateTime_ = mainWindow_->simplifiedDateTime_;

      newsModel_->textColor_ = mainWindow_->newsListTextColor_;
      newsView_->setStyleSheet(QString("#newsView_ {background: %1;}").arg(mainWindow_->newsListBackgroundColor_));
      newsModel_->newNewsTextColor_ = mainWindow_->newNewsTextColor_;
      newsModel_->unreadNewsTextColor_ = mainWindow_->unreadNewsTextColor_;
      newsModel_->focusedNewsTextColor_ = mainWindow_->focusedNewsTextColor_;
      newsModel_->focusedNewsBGColor_ = mainWindow_->focusedNewsBGColor_;
      // S-1: dim read news in the list
      newsModel_->dimRead_ = mainWindow_->dimRead_;
      newsModel_->dimReadColor_ = QColor(mainWindow_->newsListTextColor_).lighter(160).name();

      QString styleSheetNews = settings.value("Settings/styleSheetNews",
                                              mainApp->styleSheetNewsDefaultFile()).toString();
      QFile file(styleSheetNews);
      if (!file.open(QFile::ReadOnly)) {
        file.setFileName(":/style/newsStyle");
        file.open(QFile::ReadOnly);
      }
      cssString_ = QString::fromUtf8(file.readAll()).
          arg(mainWindow_->newsTextFontFamily_).
          arg(mainWindow_->newsTextFontSize_).
          arg(mainWindow_->newsTitleFontFamily_).
          arg(mainWindow_->newsTitleFontSize_).
          arg(0).
          arg(qApp->palette().color(QPalette::Dark).name()). // color separator
          arg(mainWindow_->newsBackgroundColor_). // news background
          arg(mainWindow_->newsTitleBackgroundColor_). // title background
          arg(mainWindow_->linkColor_). // link color
          arg(mainWindow_->titleColor_). // title color
          arg(mainWindow_->dateColor_). // date color
          arg(mainWindow_->authorColor_). // author color
          arg(mainWindow_->newsTextColor_); // text color
      file.close();

      file.setFileName(":/html/audioplayer");
      file.open(QFile::ReadOnly);
      audioPlayerHtml_ = QString::fromUtf8(file.readAll());
      file.close();

      file.setFileName(":/html/videoplayer");
      file.open(QFile::ReadOnly);
      videoPlayerHtml_ = QString::fromUtf8(file.readAll());
      file.close();
    }

    if (mainWindow_->externalBrowserOn_ <= 0) {
      // WebEngine: all links handled via urlChanged signal
    } else {
      // WebEngine: external links opened externally
    }

    webView_->page()->action(QWebEnginePage::Back)->setShortcut(mainWindow_->backWebPageAct_->shortcut());
    webView_->page()->action(QWebEnginePage::Forward)->setShortcut(mainWindow_->forwardWebPageAct_->shortcut());
    webView_->page()->action(QWebEnginePage::Reload)->setShortcut(mainWindow_->reloadWebPageAct_->shortcut());

    QWebEngineProfile::defaultProfile()->setHttpCacheMaximumSize(0);
  }

  if (interactiveMarkController_) {
    bool isReadLaterView = (type_ == TabTypeStar) || (type_ == TabTypeLabel);
    interactiveMarkController_->setConfig(
          mainWindow_->hoverMarkRead_,
          mainWindow_->hoverMarkDelay_,
          mainWindow_->scrollMarkRead_,
          mainWindow_->viewportMarkRead_,
          mainWindow_->markExcludeOnlyStarred_,
          mainWindow_->excludedGroups_,
          mainWindow_->excludedFeeds_,
          isReadLaterView);
  }

  QModelIndex feedIndex = feedsModel_->indexById(feedId_);

  if (init) {
    QWebEngineProfile::defaultProfile()->clearHttpCache();

    if (type_ == TabTypeFeed) {
      int displayEmbeddedImages = feedsModel_->dataField(feedIndex, "displayEmbeddedImages").toInt();
      if (displayEmbeddedImages == 2) {
        autoLoadImages_ = true;
      } else if (displayEmbeddedImages == 1) {
        autoLoadImages_ = mainWindow_->autoLoadImages_;
      } else {
        autoLoadImages_ = false;
      }
    } else {
      autoLoadImages_ = mainWindow_->autoLoadImages_;
    }
    webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, autoLoadImages_);

    // S-4: warn before jumping to external links
    ((WebPage*)webView_->page())->setJumpOutLinkWarn(mainWindow_->jumpOutLinkWarn_);

    webView_->setZoomFactor(qreal(mainWindow_->defaultZoomPages_)/100.0);
  }
  setAutoLoadImages(false);

  if (type_ == TabTypeFeed) {
    int javaScriptEnable = feedsModel_->dataField(feedIndex, "javaScriptEnable").toInt();
    if (javaScriptEnable == 2) {
      webView_->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    } else if (javaScriptEnable == 1) {
      webView_->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, mainWindow_->javaScriptEnable_);
    } else if (javaScriptEnable == 0) {
      webView_->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    }

    int layoutDirection = feedsModel_->dataField(feedIndex, "layoutDirection").toInt();
    if (!layoutDirection) {
      newsView_->setLayoutDirection(Qt::LeftToRight);
    } else {
      newsView_->setLayoutDirection(Qt::RightToLeft);
    }
  } else {
    webView_->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, mainWindow_->javaScriptEnable_);
  }

  if (type_ < TabTypeWeb) {
    newsView_->setAlternatingRowColors(mainWindow_->alternatingRowColorsNews_);

    QPalette palette = newsView_->palette();
    palette.setColor(QPalette::AlternateBase, mainWindow_->alternatingRowColors_);
    newsView_->setPalette(palette);

    // S-2: apply date grouping (setGroupByDate guards against no-op re-switch)
    setGroupByDate(mainWindow_->groupByDate_);

    if (!newTab)
      newsModel_->setFilter("feedId=-1");
    newsHeader_->setColumns(feedIndex);
    mainWindow_->slotUpdateStatus(feedId_, false);
    mainWindow_->newsFilter_->setEnabled(type_ == TabTypeFeed);
    separatorRAct_->setVisible(type_ == TabTypeDel);
    mainWindow_->restoreNewsAct_->setVisible(type_ == TabTypeDel);

    // UI-5: reflect the current feed's update status in the news list
    updateErrorBanner();

    if (mainWindow_->isFocusMode()) {
      newsWidget_->setVisible(false);
    } else {
      switch (mainWindow_->newsLayout_) {
      case 1:
        newsWidget_->setVisible(false);
        break;
      default:
        newsWidget_->setVisible(true);
      }
    }
  }
}

/** @brief Reload translation
 *----------------------------------------------------------------------------*/
void NewsTabWidget::retranslateStrings() {
  if (type_ != TabTypeDownloads) {
    webViewProgress_->setFormat(tr("Loading... (%p%)"));

    webHomePageAct_->setText(tr("Home"));
    webExternalBrowserAct_->setText(tr("Open Page in External Browser"));
    urlExternalBrowserAct_->setText(tr("Open Link in External Browser"));

    if (type_ != TabTypeWeb) {
      findText_->retranslateStrings();
      newsHeader_->retranslateStrings();
    }

    if (mainWindow_->currentNewsTab == this) {
      if (autoLoadImages_) {
        mainWindow_->autoLoadImagesToggle_->setText(tr("Load Images"));
        mainWindow_->autoLoadImagesToggle_->setToolTip(tr("Auto Load Images to News View"));
      } else {
        mainWindow_->autoLoadImagesToggle_->setText(tr("No Load Images"));
        mainWindow_->autoLoadImagesToggle_->setToolTip(tr("No Load Images to News View"));
      }
    }
  }

  closeButton_->setToolTip(tr("Close Tab"));
}

void NewsTabWidget::setAutoLoadImages(bool apply)
{
  if (type_ == NewsTabWidget::TabTypeDownloads) return;
  if (mainWindow_->currentNewsTab != this) return;

  if (apply)
    autoLoadImages_ = !autoLoadImages_;

  if (autoLoadImages_) {
    mainWindow_->autoLoadImagesToggle_->setText(tr("Load Images"));
    mainWindow_->autoLoadImagesToggle_->setToolTip(tr("Auto Load Images in News View"));
    mainWindow_->autoLoadImagesToggle_->setIcon(QIcon(":/images/imagesOn"));
  } else {
    mainWindow_->autoLoadImagesToggle_->setText(tr("Don't Load Images"));
    mainWindow_->autoLoadImagesToggle_->setToolTip(tr("Don't Load Images in News View"));
    mainWindow_->autoLoadImagesToggle_->setIcon(QIcon(":/images/imagesOff"));
  }

  if (apply) {
    webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, autoLoadImages_);
    if (autoLoadImages_) {
      if ((webView_->title() == "news_descriptions") &&
          (type_ == NewsTabWidget::TabTypeFeed)) {
        switch (mainWindow_->newsLayout_) {
        case 1:
          loadNewspaper();
          break;
        default:
          updateWebView(newsView_->currentIndex());
        }
      } else {
        webView_->reload();
      }
    }
  }
}

/** @brief Process mouse click in news list
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsViewClicked(QModelIndex index)
{
  slotNewsViewSelected(index);
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsViewSelected(QModelIndex index, bool clicked)
{
  if (mainWindow_->newsLayout_ == 1) return;

  // S-2: convert grouped proxy index back to the source model
  index = newsIndexToSource(index);

  int newsId = newsModel_->dataField(index.row(), "id").toInt();
  if (mainWindow_->markNewsReadOn_ && mainWindow_->markPrevNewsRead_ &&
      (newsId != currentNewsIdOld)) {
    QModelIndex startIndex = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(startIndex, Qt::EditRole, currentNewsIdOld);
    if (!indexList.isEmpty()) {
      slotSetItemRead(indexList.first(), 1);
    }
  }

  if (!index.isValid()) {
    hideWebContent();
    currentNewsIdOld = newsId;
    return;
  }

  if (!((newsId == currentNewsIdOld) &&
        newsModel_->dataField(index.row(), "read").toInt() >= 1) ||
      clicked) {
    markNewsReadTimer_->stop();
    if (mainWindow_->markNewsReadOn_ && mainWindow_->markCurNewsRead_) {
      if (mainWindow_->markNewsReadTime_ == 0) {
        slotSetItemRead(index, 1);
      } else {
        markNewsReadTimer_->start(mainWindow_->markNewsReadTime_*1000);
      }
    }

    if (type_ == TabTypeFeed) {
      // Write current news to feed
      QString qStr = QString("UPDATE feeds SET currentNews='%1' WHERE id=='%2'").
          arg(newsId).arg(feedId_);
      mainApp->sqlQueryExec(qStr);

      QModelIndex feedIndex = feedsModel_->indexById(feedId_);
      feedsModel_->setData(feedsModel_->indexSibling(feedIndex, "currentNews"), newsId);
    } else if (type_ == TabTypeLabel) {
      QString qStr = QString("UPDATE labels SET currentNews='%1' WHERE id=='%2'").
          arg(newsId).
          arg(mainWindow_->categoriesTree_->currentItem()->text(2).toInt());
      mainApp->sqlQueryExec(qStr);

      mainWindow_->categoriesTree_->currentItem()->setText(3, QString::number(newsId));
    }

    updateWebView(index);
    mainWindow_->statusBar()->showMessage(linkNewsString_, 3000);

    currentShownNewsId_ = newsId;
    if (mainWindow_->aiAssistant_) {
      if (mainWindow_->aiAutoTranslate_)
        maybeAutoTranslate(index, newsId);
      if (mainWindow_->aiAutoSummary_)
        maybeAutoSummarize(index, newsId);
      if (mainWindow_->aiAutoRecommend_)
        maybeAutoRecommend(index, newsId);
    }

    if (mainWindow_->statisticsService_)
      mainWindow_->statisticsService_->addEvent(StatType::NewsView);

    if (mainWindow_->progressService_) {
      mainWindow_->progressService_->updateContext(
            feedId_, newsId, newsView_->verticalScrollBar()->value());
    }
  }
  currentNewsIdOld = newsId;
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsListScrolled()
{
  // Paging: load the next page when the user scrolls near the bottom.
  if (newsModel_ && newsModel_->canFetchMore()) {
    QScrollBar *sb = newsView_->verticalScrollBar();
    if (sb->value() >= sb->maximum() - sb->pageStep() * 2) {
      const int newsId = currentShownNewsId_;
      const int scrollValue = sb->value();
      newsModel_->fetchMore();
      // The fetch resets the model; restore the selection and scroll offset.
      if (newsId > 0) {
        QModelIndex start = newsModel_->index(0, newsModel_->fieldIndex("id"));
        QModelIndexList list = newsModel_->match(start, Qt::EditRole, newsId);
        if (!list.isEmpty()) {
          QModelIndex target = newsIndexFromSource(
                newsModel_->index(list.first().row(),
                                  newsModel_->fieldIndex("title")));
          if (target.isValid())
            newsView_->setCurrentIndex(target);
        }
      }
      sb->setValue(qMin(scrollValue, sb->maximum()));
    }
  }

  if (!mainWindow_->progressService_) return;

  QModelIndex index = newsIndexToSource(newsView_->currentIndex());
  if (!index.isValid()) return;

  int newsId = newsModel_->dataField(index.row(), "id").toInt();
  if (newsId <= 0) return;

  mainWindow_->progressService_->updateScrollPos(
        newsView_->verticalScrollBar()->value());
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsViewDoubleClicked(QModelIndex index)
{
  index = newsIndexToSource(index);
  if (!index.isValid()) return;

  QUrl url = QUrl::fromEncoded(getLinkNews(index.row()).toUtf8());
  if (url.isEmpty() || !url.isValid())
    return;
  slotLinkClicked(url);
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotNewsMiddleClicked(QModelIndex index)
{
  index = newsIndexToSource(index);
  if (!index.isValid()) return;

  if (mainWindow_->markNewsReadOn_ && mainWindow_->markCurNewsRead_)
    slotSetItemRead(index, 1);

  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    webView_->buttonClick_ = MIDDLE_BUTTON;
  } else if (QApplication::keyboardModifiers() == Qt::AltModifier) {
    webView_->buttonClick_ = LEFT_BUTTON_ALT;
  } else {
    webView_->buttonClick_ = MIDDLE_BUTTON_MOD;
  }

  QUrl url = QUrl::fromEncoded(getLinkNews(index.row()).toUtf8());
  if (url.isEmpty() || !url.isValid())
    return;
  slotLinkClicked(url);
}

/** @brief Process pressing UP-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsUpPressed(QModelIndex index)
{
  if (type_ >= TabTypeWeb) return;

  if (!index.isValid())
    index = neighborNewsIndex(false);

  if (!index.isValid())
    return;

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (index.row() < (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(index.row() - pageStep/2);

  newsView_->setCurrentIndex(index);
  slotNewsViewSelected(index);
}

/** @brief Process pressing DOWN-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsDownPressed(QModelIndex index)
{
  if (type_ >= TabTypeWeb) return;

  if (!index.isValid())
    index = neighborNewsIndex(true);

  if (!index.isValid())
    return;

  int value = newsView_->verticalScrollBar()->value();
  int pageStep = newsView_->verticalScrollBar()->pageStep();
  if (index.row() > (value + pageStep/2))
    newsView_->verticalScrollBar()->setValue(index.row() - pageStep/2);
  newsView_->setCurrentIndex(index);
  slotNewsViewSelected(index);
}

/** @brief Process pressing HOME-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsHomePressed(QModelIndex index)
{
  slotNewsViewSelected(index);
}

/** @brief Process pressing END-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsEndPressed(QModelIndex index)
{
  slotNewsViewSelected(index);
}

/** @brief Process pressing PageUp-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsPageUpPressed(QModelIndex index)
{
  if (type_ >= TabTypeWeb) return;

  if (!index.isValid()) {
    int step = newsView_->verticalScrollBar()->pageStep();
    QModelIndex cur = newsView_->currentIndex();
    for (int i = 0; i < step; i++) {
      QModelIndex prev = neighborNewsIndex(false, cur);
      if (!prev.isValid())
        break;
      cur = prev;
    }
    index = cur;
    newsView_->setCurrentIndex(index);
  }

  slotNewsViewSelected(index);
}

/** @brief Process pressing PageDown-key
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotNewsPageDownPressed(QModelIndex index)
{
  if (type_ >= TabTypeWeb) return;

  if (!index.isValid()) {
    int step = newsView_->verticalScrollBar()->pageStep();
    QModelIndex cur = newsView_->currentIndex();
    for (int i = 0; i < step; i++) {
      QModelIndex nxt = neighborNewsIndex(true, cur);
      if (!nxt.isValid())
        break;
      cur = nxt;
    }
    index = cur;
    newsView_->setCurrentIndex(index);
  }

  slotNewsViewSelected(index);
}

void NewsTabWidget::slotNewsNextUnreadPressed(QModelIndex index)
{
  Q_UNUSED(index);
  if (type_ >= TabTypeWeb) return;
  // Forward to the main window, which implements the "next unread" logic
  // (including jumping to the next feed when this list is exhausted).
  QMetaObject::invokeMethod(mainWindow_, "nextUnreadNews", Qt::QueuedConnection);
}

void NewsTabWidget::slotNewsPrevUnreadPressed(QModelIndex index)
{
  Q_UNUSED(index);
  if (type_ >= TabTypeWeb) return;
  QMetaObject::invokeMethod(mainWindow_, "prevUnreadNews", Qt::QueuedConnection);
}

void NewsTabWidget::setNewsListVisible(bool visible)
{
  newsWidget_->setVisible(visible);
  if (visible)
    newsView_->setFocus(Qt::OtherFocusReason);
}

/** @brief Show/hide the inline error banner for the current feed (UI-5)
 *---------------------------------------------------------------------------*/
void NewsTabWidget::updateErrorBanner()
{
  if (!errorBanner_ || !feedsModel_) return;

  bool show = false;
  QString message;

  if (type_ == TabTypeFeed) {
    QModelIndex feedIndex = feedsModel_->indexById(feedId_);
    if (feedIndex.isValid() && !feedsModel_->isFolder(feedIndex)) {
      QString status = feedsModel_->dataField(feedIndex, "status").toString();
      const int code = status.section(" ", 0, 0).toInt();
      if (code < 0) {
        show = true;
        message = status.section(" ", 1).trimmed();
        if (message.isEmpty()) {
          switch (code) {
          case -2: message = tr("Authentication required"); break;
          case -4: message = tr("Too many redirects"); break;
          case -5: message = tr("Subscription not found (404)"); break;
          case -1: message = tr("Network error"); break;
          default: message = tr("Update failed"); break;
          }
        }
      }
    }
  }

  errorBanner_->setVisible(show);
  if (show)
    errorBannerLabel_->setText(tr("Update failed: %1").arg(message));
}

/** @brief Retry updating the current feed from the error banner (UI-5)
 *---------------------------------------------------------------------------*/
void NewsTabWidget::slotRetryCurrentFeed()
{
  if (type_ != TabTypeFeed) return;

  QList<int> feedIds;
  feedIds << feedId_;
  QMetaObject::invokeMethod(mainWindow_, "slotCheckStatus",
                            Qt::QueuedConnection,
                            Q_ARG(QList<int>, feedIds));
}

/** @brief Mark news Read
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSetItemRead(QModelIndex index, int read)
{
  markNewsReadTimer_->stop();
  index = newsIndexToSource(index);
  if (!index.isValid() || (newsModel_->rowCount() == 0)) return;

  bool changed = false;
  int newsId = newsModel_->dataField(index.row(), "id").toInt();

  if (read == 1) {
    if (newsModel_->dataField(index.row(), "new").toInt() == 1) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("new")),
            0);
      mainApp->sqlQueryExec(QString("UPDATE news SET new=0 WHERE id=='%1'").arg(newsId));
    }
    if (newsModel_->dataField(index.row(), "read").toInt() == 0) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
            1);
      mainApp->sqlQueryExec(QString("UPDATE news SET read=1 WHERE id=='%1'").arg(newsId));
      if (mainWindow_->statisticsService_)
        mainWindow_->statisticsService_->addEvent(StatType::NewsRead);
      changed = true;
    }
  } else {
    if (newsModel_->dataField(index.row(), "read").toInt() != 0) {
      newsModel_->setData(
            newsModel_->index(index.row(), newsModel_->fieldIndex("read")),
            0);
      mainApp->sqlQueryExec(QString("UPDATE news SET read=0 WHERE id=='%1'").arg(newsId));
      changed = true;
    }
  }

  if (changed) {
    newsView_->viewport()->update();
    int feedId = newsModel_->dataField(index.row(), "feedId").toInt();
    mainWindow_->slotUpdateStatus(feedId);
    mainWindow_->recountCategoryCounts();
  }
}

/** @brief Mark news Star
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSetItemStar(QModelIndex index, int starred)
{
  index = newsIndexToSource(index);
  if (!index.isValid()) return;

  newsModel_->setData(index, starred);

  int newsId = newsModel_->dataField(index.row(), "id").toInt();
  mainApp->sqlQueryExec(QString("UPDATE news SET starred='%1' WHERE id=='%2'").
                        arg(starred).arg(newsId));
  if ((starred == 1) && mainWindow_->statisticsService_)
    mainWindow_->statisticsService_->addEvent(StatType::NewsStar);

  // Offline image cache: starring an article is a strong "keep" intent, so
  // download its images and persist a locally rewritten copy.
  if (starred == 1) {
    const QString content =
        newsModel_->dataField(index.row(), "content").toString();
    if (!content.isEmpty()) {
      QString link = newsModel_->dataField(index.row(), "link_href").toString();
      if (link.isEmpty())
        link = newsModel_->dataField(index.row(), "link_alternate").toString();
      mainApp->imageCache()->cacheNewsImages(newsId, feedId_,
                                             content, QUrl(link));
    }
  }

  mainWindow_->recountCategoryCounts();
}

void NewsTabWidget::slotInteractiveMarkRead(int count)
{
  Q_UNUSED(count);
  if (mainWindow_->statisticsService_)
    mainWindow_->statisticsService_->addEvent(StatType::NewsRead);
}

void NewsTabWidget::slotMarkReadTimeout()
{
  slotSetItemRead(newsView_->currentIndex(), 1);
}

/** @brief Mark selected news Read
 *----------------------------------------------------------------------------*/
void NewsTabWidget::markNewsRead()
{
  if (type_ >= TabTypeWeb) return;
  markNewsReadTimer_->stop();

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  if (cnt == 1) {
    curIndex = newsIndexToSource(indexes.at(0));
    if (newsModel_->dataField(curIndex.row(), "read").toInt() == 0) {
      slotSetItemRead(curIndex, 1);
    } else {
      slotSetItemRead(curIndex, 0);
    }
  } else {
    QStringList feedIdList;

    bool markRead = false;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = newsIndexToSource(indexes.at(i));
      if (newsModel_->dataField(curIndex.row(), "read").toInt() == 0) {
        markRead = true;
        break;
      }
    }

    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = newsIndexToSource(indexes.at(i));
      newsModel_->setData(
            newsModel_->index(curIndex.row(), newsModel_->fieldIndex("new")),
            0);
      newsModel_->setData(
            newsModel_->index(curIndex.row(), newsModel_->fieldIndex("read")),
            markRead);

      int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
      q.exec(QString("UPDATE news SET new=0, read='%1' WHERE id=='%2'").
             arg(markRead).arg(newsId));
      QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
    }
    db_.commit();

    foreach (QString feedId, feedIdList) {
      mainWindow_->slotUpdateStatus(feedId.toInt());
    }
    mainWindow_->recountCategoryCounts();
    newsView_->viewport()->update();
  }
}

/** @brief Mark all news of the feed Read
 *----------------------------------------------------------------------------*/
void NewsTabWidget::markAllNewsRead()
{
  if (type_ >= TabTypeWeb) return;
  markNewsReadTimer_->stop();

  int cnt = newsModel_->rowCount();
  if (cnt == 0) return;

  QStringList feedIdList;

  db_.transaction();
  QSqlQuery q;
  for (int i = cnt-1; i >= 0; --i) {
    int newsId = newsModel_->dataField(i, "id").toInt();
    q.exec(QString("UPDATE news SET read=1 WHERE id=='%1' AND read=0").arg(newsId));
    q.exec(QString("UPDATE news SET new=0 WHERE id=='%1' AND new=1").arg(newsId));

    QString feedId = newsModel_->dataField(i, "feedId").toString();
    if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
  }
  db_.commit();

  QModelIndex curIndex = newsView_->currentIndex();
  QModelIndex curSrcIndex = newsIndexToSource(curIndex);
  int currentRow = curSrcIndex.isValid() ? curSrcIndex.row() : 0;

  newsModel_->select();

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  loadNewspaper(RefreshWithPos);

  if (newsView_->model() == newsProxyModel_) {
    QModelIndex newSrc = newsModel_->index(currentRow, newsModel_->fieldIndex("title"));
    QModelIndex newProxy = newsProxyModel_->mapFromSource(newSrc);
    if (newProxy.isValid())
      newsView_->setCurrentIndex(newProxy);
  } else {
    newsView_->setCurrentIndex(newsModel_->index(currentRow, newsModel_->fieldIndex("title")));
  }

  foreach (QString feedId, feedIdList) {
    mainWindow_->slotUpdateStatus(feedId.toInt());
  }
  mainWindow_->recountCategoryCounts();
}

/** @brief Mark selected news Starred
 *----------------------------------------------------------------------------*/
void NewsTabWidget::markNewsStar()
{
  if (type_ >= TabTypeWeb) return;

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(
        newsModel_->fieldIndex("starred"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  if (cnt == 1) {
    curIndex = indexes.at(0);
    if (curIndex.data(Qt::EditRole).toInt() == 0) {
      slotSetItemStar(curIndex, 1);
    } else {
      slotSetItemStar(curIndex, 0);
    }
  } else {
    bool markStar = false;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = indexes.at(i);
      if (curIndex.data(Qt::EditRole).toInt() == 0) {
        markStar = true;
        break;
      }
    }

    db_.transaction();
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = newsIndexToSource(indexes.at(i));
      newsModel_->setData(curIndex, markStar);

      int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
      QSqlQuery q;
      q.exec(QString("UPDATE news SET starred='%1' WHERE id=='%2'").
             arg(markStar).arg(newsId));
    }
    db_.commit();

    mainWindow_->recountCategoryCounts();
  }
}

/** @brief Delete selected news
 *----------------------------------------------------------------------------*/
void NewsTabWidget::deleteNews()
{
  if (type_ >= TabTypeWeb) return;

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(newsModel_->fieldIndex("deleted"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  QStringList feedIdList;

  if (type_ != TabTypeDel) {
    if (cnt == 1) {
      curIndex = newsIndexToSource(indexes.at(0));
      if (newsModel_->dataField(curIndex.row(), "starred").toInt() &&
          mainWindow_->notDeleteStarred_)
        return;
      QString labelStr = newsModel_->dataField(curIndex.row(), "label").toString();
      if (!(labelStr.isEmpty() || (labelStr == ",")) && mainWindow_->notDeleteLabeled_)
        return;

      slotSetItemRead(curIndex, 1);

      newsModel_->setData(curIndex, 1);
      newsModel_->setData(newsModel_->index(curIndex.row(), newsModel_->fieldIndex("deleteDate")),
                          QDateTime::currentDateTime().toString(Qt::ISODate));

      QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);

      newsModel_->submitAll();
    } else {
      db_.transaction();
      QSqlQuery q;
      for (int i = cnt-1; i >= 0; --i) {
        curIndex = newsIndexToSource(indexes.at(i));
        if (newsModel_->dataField(curIndex.row(), "starred").toInt() &&
            mainWindow_->notDeleteStarred_)
          continue;
        QString labelStr = newsModel_->dataField(curIndex.row(), "label").toString();
        if (!(labelStr.isEmpty() || (labelStr == ",")) && mainWindow_->notDeleteLabeled_)
          continue;

        int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
        q.exec(QString("UPDATE news SET new=0, read=2, deleted=1, deleteDate='%1' WHERE id=='%2'").
               arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
               arg(newsId));

        QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
        if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
      }
      db_.commit();

      newsModel_->select();
    }
  }
  else {
    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = newsIndexToSource(indexes.at(i));

      int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
      q.exec(QString("UPDATE news SET description='', content='', received='', "
                     "author_name='', author_uri='', author_email='', "
                     "category='', new='', read='', starred='', label='', "
                     "deleteDate='', feedParentId='', deleted=2 WHERE id=='%1'").
             arg(newsId));

      QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
    }
    db_.commit();

    newsModel_->select();
  }

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  if (curIndex.row() == newsModel_->rowCount())
    curIndex = newsModel_->index(curIndex.row()-1, newsModel_->fieldIndex("title"));
  else if (curIndex.row() > newsModel_->rowCount())
    curIndex = newsModel_->index(newsModel_->rowCount()-1, newsModel_->fieldIndex("title"));
  else
    curIndex = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("title"));
  QModelIndex viewIdx = newsIndexFromSource(curIndex);
  newsView_->setCurrentIndex(viewIdx);
  slotNewsViewSelected(viewIdx);

  foreach (QString feedId, feedIdList) {
    mainWindow_->slotUpdateStatus(feedId.toInt());
  }
  mainWindow_->recountCategoryCounts();
}

/** @brief Delete all news of the feed
 *----------------------------------------------------------------------------*/
void NewsTabWidget::deleteAllNewsList()
{
  if (type_ >= TabTypeWeb) return;

  newsModel_->fetchAll();
  int cnt = newsModel_->rowCount();
  if (cnt == 0) return;

  QStringList feedIdList;

  db_.transaction();
  QSqlQuery q;
  for (int i = cnt-1; i >= 0; --i) {
    int newsId = newsModel_->dataField(i, "id").toInt();

    if (type_ != TabTypeDel) {
      if (newsModel_->dataField(i, "starred").toInt() &&
          mainWindow_->notDeleteStarred_)
        continue;
      QString labelStr = newsModel_->dataField(i, "label").toString();
      if (!(labelStr.isEmpty() || (labelStr == ",")) && mainWindow_->notDeleteLabeled_)
        continue;

      q.exec(QString("UPDATE news SET new=0, read=2, deleted=1, deleteDate='%1' WHERE id=='%2'").
             arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
             arg(newsId));
    }
    else {
      q.exec(QString("UPDATE news SET description='', content='', received='', "
                     "author_name='', author_uri='', author_email='', "
                     "category='', new='', read='', starred='', label='', "
                     "deleteDate='', feedParentId='', deleted=2 WHERE id=='%1'").
             arg(newsId));
    }

    QString feedId = newsModel_->dataField(i, "feedId").toString();
    if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
  }
  db_.commit();

  newsModel_->select();

  slotNewsViewSelected(QModelIndex());

  foreach (QString feedId, feedIdList) {
    mainWindow_->slotUpdateStatus(feedId.toInt());
  }
  mainWindow_->recountCategoryCounts();
}

/** @brief Restore deleted news
 *----------------------------------------------------------------------------*/
void NewsTabWidget::restoreNews()
{
  if (type_ >= TabTypeWeb) return;

  QModelIndex curIndex;
  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(newsModel_->fieldIndex("deleted"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  QStringList feedIdList;

  if (cnt == 1) {
    curIndex = newsIndexToSource(indexes.at(0));
    newsModel_->setData(curIndex, 0);
    newsModel_->setData(newsModel_->index(curIndex.row(), newsModel_->fieldIndex("deleteDate")), "");
    newsModel_->submitAll();

    QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
    if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
  } else {
    db_.transaction();
    QSqlQuery q;
    for (int i = cnt-1; i >= 0; --i) {
      curIndex = newsIndexToSource(indexes.at(i));
      int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
      q.exec(QString("UPDATE news SET deleted=0, deleteDate='' WHERE id=='%1'").
             arg(newsId));

      QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
      if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
    }
    db_.commit();

    newsModel_->select();
  }

  while (newsModel_->canFetchMore())
    newsModel_->fetchMore();

  loadNewspaper(RefreshWithPos);

  if (curIndex.row() == newsModel_->rowCount())
    curIndex = newsModel_->index(curIndex.row()-1, newsModel_->fieldIndex("title"));
  else if (curIndex.row() > newsModel_->rowCount())
    curIndex = newsModel_->index(newsModel_->rowCount()-1, newsModel_->fieldIndex("title"));
  else
    curIndex = newsModel_->index(curIndex.row(), newsModel_->fieldIndex("title"));
  QModelIndex viewIdx = newsIndexFromSource(curIndex);
  newsView_->setCurrentIndex(viewIdx);
  slotNewsViewSelected(viewIdx);
  mainWindow_->slotUpdateStatus(feedId_);
  mainWindow_->recountCategoryCounts();

  foreach (QString feedId, feedIdList) {
    mainWindow_->slotUpdateStatus(feedId.toInt());
  }
  mainWindow_->recountCategoryCounts();
}

/** @brief Copy news link
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotCopyLinkNews()
{
  if (type_ >= TabTypeWeb) return;

  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  QString copyStr;
  for (int i = cnt-1; i >= 0; --i) {
    if (!copyStr.isEmpty()) copyStr.append("\n");
    copyStr.append(getLinkNews(newsIndexToSource(indexes.at(i)).row()));
  }

  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(copyStr);
}

/** @brief Sort news by Star or Read column
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSort(int column, int/* order*/)
{
  QString strId;
  if (feedsModel_->isFolder(feedsModel_->indexById(feedId_))) {
    strId = QString("(%1)").arg(mainWindow_->getIdFeedsString(feedId_));
  } else {
    strId = QString("feedId='%1'").arg(feedId_);
  }

  QString qStr;
  if (column == newsModel_->fieldIndex("read")) {
    qStr = QString("UPDATE news SET rights=read WHERE %1").arg(strId);
  }
  else if (column == newsModel_->fieldIndex("starred")) {
    qStr = QString("UPDATE news SET rights=starred WHERE %1").arg(strId);
  }
  else if (column == newsModel_->fieldIndex("rights")) {
    qStr = QString("UPDATE news SET rights = (SELECT text from feeds where id = news.feedId) WHERE %1").arg(strId);
  }

  QSqlQuery q;
  q.exec(qStr);
}

/** @brief Load/Update browser contents
 *----------------------------------------------------------------------------*/
void NewsTabWidget::updateWebView(QModelIndex index)
{
  // S-2: convert grouped proxy index back to the source model
  index = newsIndexToSource(index);

  if (!index.isValid()) {
    hideWebContent();
    return;
  }

  QString newsId = newsModel_->dataField(index.row(), "id").toString();
  const int newsIdInt = newsId.toInt();

  // UI-3: remember the scroll position of the article we are leaving
  if ((type_ < TabTypeWeb) && (currentShownNewsId_ > 0) &&
      (currentShownNewsId_ != newsIdInt)) {
    saveArticleScrollAsync(currentShownNewsId_);
  }
  currentShownNewsId_ = newsIdInt;

  linkNewsString_ = getLinkNews(index.row());
  QString linkString = linkNewsString_;
  QUrl newsUrl = QUrl::fromEncoded(linkString.toUtf8());

  bool showDescriptionNews_ = mainWindow_->showDescriptionNews_;
  QModelIndex currentIndex = feedsProxyModel_->mapToSource(feedsView_->currentIndex());
  QVariant displayNews = feedsModel_->dataField(currentIndex, "displayNews");
  QString feedId = newsModel_->dataField(index.row(), "feedId").toString();
  QModelIndex feedIndex = feedsModel_->indexById(feedId.toInt());

  if (!displayNews.toString().isEmpty())
    showDescriptionNews_ = !displayNews.toInt();

  if (!showDescriptionNews_) {
    if (mainWindow_->externalBrowserOn_ <= 0) {
      locationBar_->setText(newsUrl.toString());
      setWebToolbarVisible(true, false);

      if (newsUrl.isEmpty() || !newsUrl.isValid()) {
        // No link (or malformed link) for this news item: show a blank
        // page instead of handing an empty QUrl to QWebEngineView::load(),
        // which can crash the renderer in Qt 5.15.
        webView_->stop();
        webView_->history()->clear();
        webView_->setHtml(QString());
      } else {
        webView_->stop();
        webView_->history()->clear();
        webView_->load(newsUrl);
      }
    } else {
      openUrl(newsUrl);
    }
  } else {
    setWebToolbarVisible(false, false);

    QString htmlStr;
    QString content = newsModel_->dataField(index.row(), "content").toString();
    QString translatedContent = newsModel_->dataField(
          index.row(), "translatedContent").toString();
    if (!translatedContent.isEmpty()) {
      content = translatedContent;
    } else {
      // Fallback chain: user-fetched full text -> offline image cache ->
      // original feed content (already handled below by 'content').
      QSqlQuery qfc(db_);
      qfc.prepare("SELECT value FROM news_ex "
                  "WHERE newsId=? AND name='fullContent'");
      qfc.addBindValue(newsIdInt);
      qfc.exec();
      if (qfc.next() && !qfc.value(0).toString().isEmpty()) {
        content = qfc.value(0).toString();
      } else {
        // Offline image cache: use the locally rewritten HTML (file://
        // images) when available so the article renders fully offline.
        const QString cached = ImageCacheManager::cachedContent(newsIdInt, db_);
        if (!cached.isEmpty())
          content = cached;
      }
    }

    // Security: strip scripts, event handlers and dangerous URL schemes from
    // the untrusted article HTML before it is rendered by WebEngine. This
    // runs after the fallback chain above so every source (translated
    // content, fetched full text, offline cache, original feed content) is
    // covered, while the lightbox/code-highlighter tags injected later stay
    // untouched.
    content = HtmlSanitizer::sanitize(content);

    if (!content.contains(QzRegExp("<html(.*)</html>", Qt::CaseInsensitive))) {
      QString description = newsModel_->dataField(index.row(), "description").toString();
      if (content.isEmpty() || (description.length() > content.length())) {
        content = description;
      }

      QString titleString = newsModel_->dataField(index.row(), "title").toString();
      if (!linkString.isEmpty()) {
        titleString = QString("<a href='%1' class='unread'>%2</a>").
            arg(linkString, titleString);
      }

      QDateTime dtLocal;
      QString dateString = newsModel_->dataField(index.row(), "published").toString();
      if (!dateString.isNull()) {
        QDateTime dtLocalTime = QDateTime::currentDateTime();
        QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
        int nTimeShift = dtLocalTime.secsTo(dtUTC);

        QDateTime dt = QDateTime::fromString(dateString, Qt::ISODate);
        dtLocal = dt.addSecs(nTimeShift);
      } else {
        dtLocal = QDateTime::fromString(
              newsModel_->dataField(index.row(), "received").toString(),
              Qt::ISODate);
      }
      if (QDateTime::currentDateTime().date() <= dtLocal.date())
        dateString = dtLocal.toString(mainWindow_->formatTime_);
      else
        dateString = dtLocal.toString(mainWindow_->formatDate_ + " " + mainWindow_->formatTime_);

      // Create author panel from news author
      QString authorString;
      QString authorName = newsModel_->dataField(index.row(), "author_name").toString();
      QString authorEmail = newsModel_->dataField(index.row(), "author_email").toString();
      QString authorUri = newsModel_->dataField(index.row(), "author_uri").toString();

      QzRegExp reg("(^\\S+@\\S+\\.\\S+)", Qt::CaseInsensitive);
      int pos = reg.indexIn(authorName);
      if (pos > -1) {
        authorName.replace(reg.cap(1), QString(" <a href='mailto:%1'>%1</a>").arg(reg.cap(1)));
      }

      authorString = authorName;

      if (!authorEmail.isEmpty())
        authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
      if (!authorUri.isEmpty())
        authorString.append(QString(" <a href='%1'>page</a>"). arg(authorUri));

      // If news author is absent, create author panel from feed author
      // @note(arhohryakov:2012.01.03) Author is got from current feed, because
      //   news is belong to it
      if (authorString.isEmpty()) {
        authorName  = feedsModel_->dataField(feedIndex, "author_name").toString();
        authorEmail = feedsModel_->dataField(feedIndex, "author_email").toString();
        authorUri   = feedsModel_->dataField(feedIndex, "author_uri").toString();

        authorString = authorName;

        if (!authorEmail.isEmpty())
          authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
        if (!authorUri.isEmpty())
          authorString.append(QString(" <a href='%1'>page</a>").arg(authorUri));
      }

      QString commentsStr;
      QString commentsUrl = newsModel_->dataField(index.row(), "comments").toString();

      if (!commentsUrl.isEmpty())
      {
        commentsStr = QString("<a href=\"%1\"> %2</a>").arg(commentsUrl, tr("Comments"));
      }

      QString category = newsModel_->dataField(index.row(), "category").toString();

      if (!authorString.isEmpty())
      {
        authorString = QString(tr("Author: %1")).arg(authorString);

        if (!commentsStr.isEmpty())
        {
          authorString.append(QString(" | %1").arg(commentsStr));
        }
        if (!category.isEmpty())
        {
          authorString.append(QString(" | %1").arg(category));
        }
      }
      else
      {
        if (!commentsStr.isEmpty())
        {
          authorString.append(commentsStr);
        }

        if (!category.isEmpty())
        {
          if (!commentsStr.isEmpty())
          {
            authorString.append(QString(" | %1").arg(category));
          }
          else
          {
            authorString.append(category);
          }
        }
      }

      QString labelsString = getHtmlLabels(index.row());

      authorString.append(QString("<table class=\"labels\" id=\"labels%1\"><tr>%2</tr></table>").
                          arg(newsId).arg(labelsString));

      QString enclosureStr;
      QString enclosureUrl = newsModel_->dataField(index.row(), "enclosure_url").toString();

      if (!enclosureUrl.isEmpty())
      {
        QString type = newsModel_->dataField(index.row(), "enclosure_type").toString();

        if (type.contains("image"))
        {
          if (!content.contains(enclosureUrl) && autoLoadImages_)
          {
            enclosureStr = QString("<IMG SRC=\"%1\" class=\"enclosureImg\"><p>").arg(enclosureUrl);
          }
        }
        else
        {
          if (type.contains("audio"))
          {
            type = tr("audio");
            enclosureStr = audioPlayerHtml_.arg(enclosureUrl);
            enclosureStr.append("<p>");
          }
          else if (type.contains("video"))
          {
            type = tr("video");
            enclosureStr = videoPlayerHtml_.arg(enclosureUrl);
            enclosureStr.append("<p>");
          }
          else
          {
            type = tr("media");
          }

          enclosureStr.append(QString("<a href=\"%1\" class=\"enclosure\"> %2 %3 </a><p>").
                              arg(enclosureUrl, tr("Link to"), type));

          // Global player link: keeps playing when switching articles.
          enclosureStr.append(QString("<a href=\"podcast://%1\" class=\"podcastPlay\">%2</a><p>").
                              arg(QString::fromLatin1(QUrl::toPercentEncoding(enclosureUrl))).
                              arg(tr("Play in player")));
        }
      }

      content = enclosureStr + content;

      bool ltr = !feedsModel_->dataField(feedIndex, "layoutDirection").toInt();
      QString cssStr = cssString_.
          arg(ltr ? "left" : "right").  // text-align
          arg(ltr ? "ltr" : "rtl").    // direction
          arg(ltr ? "right" : "left");  // "Date" text-align

      // S-6: reader font size / line height
      if (mainWindow_->readerFontSize_ > 0)
        cssStr.append(QString("\nbody{font-size:%1pt;}").arg(mainWindow_->readerFontSize_));
      if (mainWindow_->readerLineHeight_ > 0)
        cssStr.append(QString("\n.newsTable, .newsTable td{line-height:%1%;}")
                      .arg(mainWindow_->readerLineHeight_));

      // S-9: wide mode - full width vs focused reading column
      if (mainWindow_->wideMode_)
        cssStr.append(QString("\n.newsTable{max-width:none !important;width:100% !important;}"));
      else
        cssStr.append(QString("\n.newsTable{max-width:960px;margin-left:auto;margin-right:auto;}"));

      // S-8: code highlighting styles
      if (mainWindow_->highlightCode_)
        cssStr.append(QString(
            "\npre{background:#f6f8fa;border:1px solid #d0d7de;border-radius:6px;"
            "padding:10px;overflow:auto;font-family:'Consolas','Menlo',monospace;"
            "font-size:13px;line-height:1.5;}"
            "code{font-family:'Consolas','Menlo',monospace;}"
            ".code-hl{display:block;white-space:pre-wrap;word-break:break-word;}"
            ".tok-kw{color:#cf222e;font-weight:600;}"
            ".tok-st{color:#0a3069;}"
            ".tok-nu{color:#953800;}"
            ".tok-cm{color:#6e7781;font-style:italic;}"));

      if (!autoLoadImages_) {
        QzRegExp reg("<img[^>]+>", Qt::CaseInsensitive);
        content = content.remove(reg);
      }

      QUrl url;
      url.setScheme(newsUrl.scheme());
      url.setHost(newsUrl.host());
      if (url.host().indexOf('.') == -1) {
        QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();
        url.setHost(hostUrl.host());
      }

      if (ltr)
        htmlStr = htmlString_.arg(cssStr, titleString, dateString, authorString, content, url.toString());
      else
        htmlStr = htmlRtlString_.arg(cssStr, titleString, dateString, authorString, content, url.toString());
    } else {
      if (!autoLoadImages_) {
        content = content.remove(QzRegExp("<img[^>]+>", Qt::CaseInsensitive));
      }

      htmlStr = content;
    }

    htmlStr = htmlStr.replace("src=\"//", "src=\"http://");

    // S-8: inject lightweight code highlighter (auto language detection)
    if (mainWindow_->highlightCode_ && !htmlStr.isEmpty()) {
      QFile hlFile(":/html/code_highlight");
      if (hlFile.open(QFile::ReadOnly)) {
        QString hlScript = QString::fromUtf8(hlFile.readAll());
        hlFile.close();
        QString hlTag = QString("<script type=\"text/javascript\">%1</script>").arg(hlScript);
        if (htmlStr.contains("</head>"))
          htmlStr = htmlStr.replace("</head>", hlTag + "</head>");
        else if (htmlStr.contains("</body>"))
          htmlStr = htmlStr.replace("</body>", hlTag + "</body>");
        else
          htmlStr.append(hlTag);
      }
    }

    // UI-2: inject image lightbox (click article images to zoom). The script
    // is an IIFE that touches document.body, so it must run AFTER the body has
    // been parsed — inject just before </body>, never inside <head>.
    if (!htmlStr.isEmpty()) {
      QFile lbFile(":/html/lightbox");
      if (lbFile.open(QFile::ReadOnly)) {
        QString lbScript = QString::fromUtf8(lbFile.readAll());
        lbFile.close();
        QString lbTag = QString("<script type=\"text/javascript\">%1</script>").arg(lbScript);
        if (htmlStr.contains("</body>"))
          htmlStr = htmlStr.replace("</body>", lbTag + "</body>");
        else if (htmlStr.contains("</head>"))
          htmlStr = htmlStr.replace("</head>", lbTag + "</head>");
        else
          htmlStr.append(lbTag);
      }
    }

    // UI-3: restore the article scroll position once this page is loaded
    pendingRestoreNewsId_ = newsIdInt;

    emit signalSetHtmlWebView(htmlStr);
  }
}

// ----------------------------------------------------------------------------
void NewsTabWidget::maybeAutoSummarize(const QModelIndex &index, int newsId)
{
  if (newsId <= 0) return;
  if (newsModel_->dataField(index.row(), "aiSummary").toInt() >= 1)
    return;

  AIAssistant *ai = mainWindow_->aiAssistant_;
  if (!ai) return;

  AIAssistant::ArticleContext context;
  context.feedId = newsModel_->dataField(index.row(), "feedId").toInt();
  context.newsId = newsId;
  context.title = newsModel_->dataField(index.row(), "title").toString();
  context.content = newsModel_->dataField(index.row(), "content").toString();
  if (context.content.isEmpty())
    context.content = newsModel_->dataField(index.row(), "description").toString();
  context.category = newsModel_->dataField(index.row(), "category").toString();

  // Offline fallback: if no API key is configured, use the local summarizer.
  if (!ai->isConfigured()) {
    QString summary = LocalSummarizer::summarize(context.content, 3);
    if (!summary.trimmed().isEmpty()) {
      QSqlQuery q(db_);
      q.prepare("UPDATE news SET aiSummary=1 WHERE id=?");
      q.addBindValue(newsId);
      q.exec();
      slotAutoSummaryReady(newsId, summary);
    }
    return;
  }

  ai->requestAutoSummary(context);
}

// ----------------------------------------------------------------------------
void NewsTabWidget::maybeAutoTranslate(const QModelIndex &index, int newsId)
{
  if (newsId <= 0) return;
  QString translated = newsModel_->dataField(index.row(), "translatedContent").toString();
  if (!translated.isEmpty())
    return;

  AIAssistant *ai = mainWindow_->aiAssistant_;
  if (!ai) return;
  if (mainWindow_->aiTranslateLang_.isEmpty())
    return;

  QString content = newsModel_->dataField(index.row(), "content").toString();
  if (content.isEmpty())
    content = newsModel_->dataField(index.row(), "description").toString();
  if (content.isEmpty())
    return;

  AIAssistant::ArticleContext context;
  context.feedId = newsModel_->dataField(index.row(), "feedId").toInt();
  context.newsId = newsId;
  context.title = newsModel_->dataField(index.row(), "title").toString();
  context.category = newsModel_->dataField(index.row(), "category").toString();

  TranslationService *ts = mainWindow_->translationService_;
  if (ts && ts->engine() != "ai") {
    // Non-AI engines route through TranslationService.
    ts->translate(content, mainWindow_->aiTranslateLang_, newsId);
    return;
  }
  ai->translate(content, mainWindow_->aiTranslateLang_, context);
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotAutoSummaryReady(int newsId, const QString &text)
{
  if (newsId <= 0 || text.trimmed().isEmpty()) return;

  QSqlQuery q(db_);
  q.prepare("UPDATE news SET aiSummary=1, summary=? WHERE id=?");
  q.addBindValue(text);
  q.addBindValue(newsId);
  q.exec();

  if (!newsModel_) return;

  // Refresh the in-memory row so the list shows the summary immediately
  // without a full model reload.
  const QModelIndex startIndex =
      newsModel_->index(0, newsModel_->fieldIndex("id"));
  const QModelIndexList indexList = newsModel_->match(
        startIndex, Qt::EditRole, newsId);
  if (indexList.isEmpty())
    return;
  const int row = indexList.first().row();
  newsModel_->setData(
        newsModel_->index(row, newsModel_->fieldIndex("summary")), text);
  newsModel_->setData(
        newsModel_->index(row, newsModel_->fieldIndex("aiSummary")), 1);
  newsModel_->submitAll();
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotAutoTranslationReady(int newsId, const QString &text,
                                             const QString &targetLang)
{
  Q_UNUSED(targetLang)
  if (newsId <= 0 || text.trimmed().isEmpty()) return;

  QSqlQuery q(db_);
  q.prepare("UPDATE news SET translatedContent=? WHERE id=?");
  q.addBindValue(text);
  q.addBindValue(newsId);
  q.exec();

  if (newsId == currentShownNewsId_ && newsModel_) {
    QModelIndex startIndex = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(
          startIndex, Qt::EditRole, newsId);
    if (!indexList.isEmpty())
      updateWebView(indexList.first());
  }
}

// ----------------------------------------------------------------------------
void NewsTabWidget::maybeAutoRecommend(const QModelIndex &index, int newsId)
{
  if (newsId <= 0) return;

  AIAssistant *ai = mainWindow_->aiAssistant_;
  if (!ai) return;

  AIAssistant::ArticleContext context;
  context.feedId = newsModel_->dataField(index.row(), "feedId").toInt();
  context.newsId = newsId;
  context.title = newsModel_->dataField(index.row(), "title").toString();
  context.content = newsModel_->dataField(index.row(), "content").toString();
  if (context.content.isEmpty())
    context.content = newsModel_->dataField(index.row(), "description").toString();
  context.category = newsModel_->dataField(index.row(), "category").toString();
  ai->requestAutoRecommendations(context);
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotAutoRecommendationsReady(int newsId, const QString &content)
{
  Q_UNUSED(newsId)
  Q_UNUSED(content)
}

// ----------------------------------------------------------------------------
void NewsTabWidget::slotShowImageGallery()
{
  QModelIndex index = newsIndexToSource(newsView_->currentIndex());
  if (!index.isValid()) return;

  QString content = newsModel_->dataField(index.row(), "content").toString();
  if (content.isEmpty())
    content = newsModel_->dataField(index.row(), "description").toString();

  ImageGalleryDialog dialog(this, content);
  dialog.exec();
}

/** @brief Re-render the current article when its offline images are ready
 *---------------------------------------------------------------------------*/
void NewsTabWidget::slotImageCacheReady(int newsId, const QString &cachedHtml)
{
  Q_UNUSED(cachedHtml)
  if (type_ >= TabTypeWeb) return;
  if (newsId != currentShownNewsId_) return;
  if (!newsView_->currentIndex().isValid()) return;
  updateWebView(newsView_->currentIndex());
}

/** @brief Manually fetch the full text of the current article (readability)
 *
 * Loads the article URL in a hidden page, extracts the main content with a
 * readability-style scoring script and stores it to news_ex (fullContent).
 * The render fallback chain prefers fullContent over the feed-provided
 * content/description.
 *---------------------------------------------------------------------------*/
void NewsTabWidget::slotFetchFullText()
{
  if (type_ >= TabTypeWeb) return;

  QModelIndex index = newsIndexToSource(newsView_->currentIndex());
  if (!index.isValid()) return;

  QString link = newsModel_->dataField(index.row(), "link_href").toString();
  if (link.isEmpty())
    link = newsModel_->dataField(index.row(), "link_alternate").toString();
  if (link.isEmpty())
    return;

  fetchFullTextNewsId_ = newsModel_->dataField(index.row(), "id").toInt();
  fetchFullTextFeedId_ = feedId_;
  if (fetchFullTextNewsId_ <= 0)
    return;

  if (!fullTextPage_) {
    fullTextPage_ = new QWebEnginePage(this);
    connect(fullTextPage_, &QWebEnginePage::loadFinished,
            this, &NewsTabWidget::slotFullTextPageLoaded);
  }
  fullTextPage_->load(QUrl(link));
}

/** @brief Full-text page finished loading; run the extractor
 *---------------------------------------------------------------------------*/
void NewsTabWidget::slotFullTextPageLoaded(bool ok)
{
  const int newsId = fetchFullTextNewsId_;
  const int feedId = fetchFullTextFeedId_;
  fetchFullTextNewsId_ = -1;
  fetchFullTextFeedId_ = -1;

  if (!ok || newsId <= 0 || !fullTextPage_)
    return;

  // Readability-style extractor: score candidates by text density, then
  // clone the best node and strip interactive/noise elements.
  static const char *kExtractJs =
      "(function() {"
      "  function score(el) {"
      "    var text = el.innerText || '';"
      "    var length = text.length;"
      "    if (length < 80) return 0;"
      "    var commas = (text.match(/,/g) || []).length;"
      "    var s = length + commas * 10;"
      "    var cls = (el.className || '') + ' ' + (el.id || '');"
      "    if (/(nav|menu|comment|footer|sidebar|aside|advert|social|share|promo)/i.test(cls))"
      "      s *= 0.1;"
      "    return s;"
      "  }"
      "  var best = null, bestScore = 0, i;"
      "  var all = document.querySelectorAll('article, section, main, div, td, p');"
      "  for (i = 0; i < all.length; i++) {"
      "    var s = score(all[i]);"
      "    if (s > bestScore) { bestScore = s; best = all[i]; }"
      "  }"
      "  if (!best) best = document.body;"
      "  if (!best) return '';"
      "  var clone = best.cloneNode(true);"
      "  var bad = clone.querySelectorAll('script, style, iframe, form, nav, footer, aside, button, input');"
      "  for (i = 0; i < bad.length; i++)"
      "    if (bad[i].parentNode) bad[i].parentNode.removeChild(bad[i]);"
      "  return clone.innerHTML;"
      "})()";

  QPointer<NewsTabWidget> guard(this);
  fullTextPage_->runJavaScript(QString::fromLatin1(kExtractJs),
      [guard, newsId, feedId](const QVariant &result) {
        if (guard.isNull())
          return;
        QString html = result.toString().trimmed();
        if (html.isEmpty())
          return;
        QSqlQuery q(guard->db_);
        q.prepare("UPDATE news_ex SET value=:value, feedId=:feedId "
                  "WHERE newsId=:newsId AND name='fullContent'");
        q.bindValue(":value", html);
        q.bindValue(":feedId", feedId);
        q.bindValue(":newsId", newsId);
        q.exec();
        if (q.numRowsAffected() == 0) {
          QSqlQuery qi(guard->db_);
          qi.prepare("INSERT INTO news_ex(feedId, newsId, name, value) "
                     "VALUES(:feedId, :newsId, 'fullContent', :value)");
          qi.bindValue(":feedId", feedId);
          qi.bindValue(":newsId", newsId);
          qi.bindValue(":value", html);
          qi.exec();
        }
        if ((newsId == guard->currentShownNewsId_) &&
            guard->newsView_->currentIndex().isValid()) {
          guard->updateWebView(guard->newsView_->currentIndex());
        }
      });
}

void NewsTabWidget::loadNewspaper(int refresh)
{
  if (mainWindow_->newsLayout_ != 1) return;
  // Paging: the newspaper render pass walks every row, so load them all.
  newsModel_->fetchAll();
  setWebToolbarVisible(false, false);
  webView_->setUpdatesEnabled(false);

  int sortOrder = newsHeader_->sortIndicatorOrder();
  int scrollBarValue = 0;
  int height = 0;
  if (refresh != RefreshAll) {
    // WebEngine: scroll position retrieved asynchronously via JS
    // For simplicity, we skip scroll restoration on incremental updates
  }
  webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);

  QString htmlStr;
  QUrl hostUrl;
  bool ltr = true;

  if (type_ == TabTypeFeed) {
    QModelIndex feedIndex = feedsProxyModel_->mapToSource(feedsView_->currentIndex());
    hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();
    ltr = !feedsModel_->dataField(feedIndex, "layoutDirection").toInt();
  }

  if ((refresh == RefreshAll) || (refresh == RefreshWithPos)) {
    QString cssStr = cssString_.
        arg(ltr ? "left" : "right"). // text-align
        arg(ltr ? "ltr" : "rtl"). // direction
        arg(ltr ? "right" : "left"); // "Date" text-align
    htmlStr = newspaperHeadHtml_.arg(cssStr, hostUrl.toString());

    // UI-2: inject image lightbox into the newspaper head page as well. Must
    // run after <body> is parsed (the IIFE appends to document.body).
    QFile lbFile(":/html/lightbox");
    if (lbFile.open(QFile::ReadOnly)) {
      QString lbScript = QString::fromUtf8(lbFile.readAll());
      lbFile.close();
      QString lbTag = QString("<script type=\"text/javascript\">%1</script>").arg(lbScript);
      if (htmlStr.contains("</body>"))
        htmlStr = htmlStr.replace("</body>", lbTag + "</body>");
      else if (htmlStr.contains("</head>"))
        htmlStr = htmlStr.replace("</head>", lbTag + "</head>");
      else
        htmlStr.append(lbTag);
    }

    enableImageLazyLoading(htmlStr);
    webView_->setHtml(htmlStr);
  }

  int idx = -1;
  if ((refresh == RefreshInsert) && (sortOrder == Qt::DescendingOrder))
    idx = newsModel_->rowCount();
  while (1) {
    if ((refresh == RefreshInsert) && (sortOrder == Qt::DescendingOrder)) {
      idx--;
      if (idx < 0)
        break;
    } else {
      idx++;
      if (idx >= newsModel_->rowCount())
        break;
    }

    QModelIndex index = newsModel_->index(idx, newsModel_->fieldIndex("id"));
    QString newsId = newsModel_->dataField(index.row(), "id").toString();

    if (refresh == RefreshInsert) {
      // WebEngine: check for duplicate newsId via runJavaScript
      // We'll do this check asynchronously; for now, skip duplicate check
    }

    linkNewsString_ = getLinkNews(index.row());
    QString linkString = linkNewsString_;

    QString content = newsModel_->dataField(index.row(), "content").toString();
    if (!content.contains(QzRegExp("<html(.*)</html>", Qt::CaseInsensitive))) {
      QString description = newsModel_->dataField(index.row(), "description").toString();
      if (content.isEmpty() || (description.length() > content.length())) {
        content = description;
      }

      // Security: same sanitizer as the single-article view - strip scripts,
      // event handlers and dangerous URL schemes from remote content.
      content = HtmlSanitizer::sanitize(content);

      //      QTextDocumentFragment textDocument = QTextDocumentFragment::fromHtml(content);
      //      content = textDocument.toPlainText();
      //      content = webView_->fontMetrics().elidedText(
      //            content, Qt::ElideRight, 1500);

      QString feedId = newsModel_->dataField(index.row(), "feedId").toString();
      QModelIndex feedIndex = feedsModel_->indexById(feedId.toInt());

      QString iconStr = "qrc:/images/bulletRead";
      QString titleStyle = "read";
      if (newsModel_->dataField(index.row(), "new").toInt() == 1) {
        iconStr = "qrc:/images/bulletNew";
        titleStyle = "unread";
      } else if (newsModel_->dataField(index.row(), "read").toInt() == 0) {
        iconStr = "qrc:/images/bulletUnread";
        titleStyle = "unread";
      }
      QString readImg = QString("<a href=\"quill://read.action.ui?#%1\" title='%3'>"
                                "<img class='quill-img' id=\"readAction%1\" src=\"%2\"/></a>").
          arg(newsId).arg(iconStr).arg(tr("Mark Read/Unread"));

      QString feedImg;
      QByteArray byteArray = feedsModel_->dataField(feedIndex, "image").toByteArray();
      if (!byteArray.isEmpty())
        feedImg = QString("<img class='quill-img' src=\"data:image/png;base64,") % byteArray % "\"/>";
      else
        feedImg = QString("<img class='quill-img' src=\"qrc:/images/feed\"/>");

      QString titleString = newsModel_->dataField(index.row(), "title").toString();
      if (!linkString.isEmpty()) {
        titleString = QString("<a href='%1' class='%2' id='title%3'>%4</a>").
            arg(linkString, titleStyle, newsId, titleString);
      }

      QDateTime dtLocal;
      QString dateString = newsModel_->dataField(index.row(), "published").toString();
      if (!dateString.isNull()) {
        QDateTime dtLocalTime = QDateTime::currentDateTime();
        QDateTime dtUTC = QDateTime(dtLocalTime.date(), dtLocalTime.time(), Qt::UTC);
        int nTimeShift = dtLocalTime.secsTo(dtUTC);

        QDateTime dt = QDateTime::fromString(dateString, Qt::ISODate);
        dtLocal = dt.addSecs(nTimeShift);
      } else {
        dtLocal = QDateTime::fromString(
              newsModel_->dataField(index.row(), "received").toString(),
              Qt::ISODate);
      }
      if (QDateTime::currentDateTime().date() <= dtLocal.date())
        dateString = dtLocal.toString(mainWindow_->formatTime_);
      else
        dateString = dtLocal.toString(mainWindow_->formatDate_ + " " + mainWindow_->formatTime_);

      // Create author panel from news author
      QString authorString;
      QString authorName = newsModel_->dataField(index.row(), "author_name").toString();
      QString authorEmail = newsModel_->dataField(index.row(), "author_email").toString();
      QString authorUri = newsModel_->dataField(index.row(), "author_uri").toString();

      QzRegExp reg("(^\\S+@\\S+\\.\\S+)", Qt::CaseInsensitive);
      int pos = reg.indexIn(authorName);
      if (pos > -1) {
        authorName.replace(reg.cap(1), QString(" <a href='mailto:%1'>%1</a>").arg(reg.cap(1)));
      }
      authorString = authorName;

      if (!authorEmail.isEmpty())
        authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
      if (!authorUri.isEmpty())
        authorString.append(QString(" <a href='%1'>page</a>"). arg(authorUri));

      // If news author is absent, create author panel from feed author
      // @note(arhohryakov:2012.01.03) Author is got from current feed, because
      //   news is belong to it
      if (authorString.isEmpty()) {
        authorName  = feedsModel_->dataField(feedIndex, "author_name").toString();
        authorEmail = feedsModel_->dataField(feedIndex, "author_email").toString();
        authorUri   = feedsModel_->dataField(feedIndex, "author_uri").toString();

        authorString = authorName;
        if (!authorEmail.isEmpty())
          authorString.append(QString(" <a href='mailto:%1'>e-mail</a>").arg(authorEmail));
        if (!authorUri.isEmpty())
          authorString.append(QString(" <a href='%1'>page</a>").arg(authorUri));
      }

      QString commentsStr;
      QString commentsUrl = newsModel_->dataField(index.row(), "comments").toString();
      if (!commentsUrl.isEmpty()) {
        commentsStr = QString("<a href=\"%1\"> %2</a>").arg(commentsUrl, tr("Comments"));
      }

      QString category = newsModel_->dataField(index.row(), "category").toString();

      if (!authorString.isEmpty()) {
        authorString = QString(tr("Author: %1")).arg(authorString);
        if (!commentsStr.isEmpty())
          authorString.append(QString(" | %1").arg(commentsStr));
        if (!category.isEmpty())
          authorString.append(QString(" | %1").arg(category));
      } else {
        if (!commentsStr.isEmpty())
          authorString.append(commentsStr);
        if (!category.isEmpty()) {
          if (!commentsStr.isEmpty())
            authorString.append(QString(" | %1").arg(category));
          else
            authorString.append(category);
        }
      }

      QString labelsString = getHtmlLabels(index.row());
      authorString.append(QString("<table class=\"labels\" id=\"labels%1\"><tr>%2</tr></table>").
                          arg(newsId).arg(labelsString));

      QString enclosureStr;
      QString enclosureUrl = newsModel_->dataField(index.row(), "enclosure_url").toString();
      if (!enclosureUrl.isEmpty()) {
        QString type = newsModel_->dataField(index.row(), "enclosure_type").toString();
        if (type.contains("image")) {
          if (!content.contains(enclosureUrl) && autoLoadImages_) {
            enclosureStr = QString("<IMG SRC=\"%1\" class=\"enclosureImg\"><p>").
                arg(enclosureUrl);
          }
        } else {
          if (type.contains("audio")) {
            type = tr("audio");
            enclosureStr = audioPlayerHtml_.arg(enclosureUrl);
            enclosureStr.append("<p>");
          }
          else if (type.contains("video")) {
            type = tr("video");
            enclosureStr = videoPlayerHtml_.arg(enclosureUrl);
            enclosureStr.append("<p>");
          }
          else type = tr("media");

          enclosureStr.append(QString("<a href=\"%1\" class=\"enclosure\"> %2 %3 </a><p>").
                              arg(enclosureUrl, tr("Link to"), type));

          // Global player link: keeps playing when switching articles.
          enclosureStr.append(QString("<a href=\"podcast://%1\" class=\"podcastPlay\">%2</a><p>").
                              arg(QString::fromLatin1(QUrl::toPercentEncoding(enclosureUrl))).
                              arg(tr("Play in player")));
        }
      }

      content = enclosureStr + content;

      if (!autoLoadImages_) {
        QzRegExp reg("<img[^>]+>", Qt::CaseInsensitive);
        content = content.remove(reg);
      }

      iconStr = "qrc:/images/starOff";
      if (newsModel_->dataField(index.row(), "starred").toInt() == 1) {
        iconStr = "qrc:/images/starOn";
      }
      QString starAction = QString("<div class=\"star-action\">"
                                   "<a href=\"quill://star.action.ui?#%1\" title='%3'>"
                                   "<img class='quill-img' id=\"starAction%1\" src=\"%2\"/></a></div>").
          arg(newsId).arg(iconStr).arg(tr("Mark News Star"));
      QString labelsMenu = QString("<div class=\"labels-menu\">"
                                   "<a href=\"quill://labels.menu.ui?#%1\" title='%2'>"
                                   "<img class='quill-img' id=\"labelsMenu%1\" src=\"qrc:/images/label_5\"/></a></div>").
          arg(newsId).arg(tr("Label"));
      QString shareMenu = QString("<div class=\"share-menu\">"
                                  "<a href=\"quill://share.menu.ui?#%1\" title='%2'>"
                                  "<img class='quill-img' id=\"shareMenu%1\" src=\"qrc:/images/images/share.png\"/></a></div>").
          arg(newsId).arg(tr("Share"));
      QString openBrowserAction = QString("<div class=\"open-browser\">"
                                          "<a href=\"quill://open.browser.ui?#%1\" title='%2'>"
                                          "<img class='quill-img' id=\"openBrowser%1\" src=\"qrc:/images/openBrowser\"'/></a></div>").
          arg(newsId).arg(tr("Open News in External Browser"));
      QString deleteAction = QString("<div class=\"delete-action\">"
                                     "<a href=\"quill://delete.action.ui?#%1\" title='%2'>"
                                     "<img class='quill-img' id=\"deleteAction%1\" src=\"qrc:/images/delete\"/></a></div>").
          arg(newsId).arg(tr("Delete"));
      QString actionNews = starAction % labelsMenu % shareMenu % openBrowserAction %
          deleteAction;

      QString border = "1";
      if (idx + 1 == newsModel_->rowCount())
        border = "0";
      if (ltr) {
        htmlStr = newspaperHtml_.arg(newsId, border, readImg, feedImg, titleString,
                                     dateString, authorString, content, actionNews);
      } else {
        htmlStr = newspaperHtmlRtl_.arg(newsId, border, readImg, feedImg, titleString,
                                        dateString, authorString, content, actionNews);
      }
    } else {
      if (!autoLoadImages_) {
        content = content.remove(QzRegExp("<img[^>]+>", Qt::CaseInsensitive));
      }
      htmlStr = content;
    }

    htmlStr = htmlStr.replace("src=\"//", "src=\"http://");

    // WebEngine: use runJavaScript to append/prepend HTML to body
    QString escapedHtml = QString::fromUtf8(QJsonDocument(QJsonArray() << htmlStr).toJson(QJsonDocument::Compact).trimmed().mid(1).chopped(1));
    QString jsAction;
    if ((refresh == RefreshInsert) && (sortOrder == Qt::DescendingOrder))
      jsAction = QString("document.body.insertAdjacentHTML('afterbegin', %1);").arg(escapedHtml);
    else
      jsAction = QString("document.body.insertAdjacentHTML('beforeend', %1);").arg(escapedHtml);
    webView_->page()->runJavaScript(jsAction);
    qApp->processEvents();
  }

  webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, autoLoadImages_);
  // WebEngine: scroll restoration handled asynchronously
  webView_->setUpdatesEnabled(true);
}

/** @brief Attach native lazy loading to content images
 *
 * The Chromium engine bundled with Qt 5.15 (83) supports the standard
 * loading="lazy" attribute, which defers offscreen images until they scroll
 * into view. Only applied when image autoloading is enabled; tags that
 * already carry a loading attribute are left untouched.
 *---------------------------------------------------------------------------*/
void NewsTabWidget::enableImageLazyLoading(QString &html)
{
  if (!autoLoadImages_)
    return;

  static const QRegularExpression imgRe(
        QStringLiteral("<img(?![^>]*\\bloading\\s*=)"),
        QRegularExpression::CaseInsensitiveOption);
  html.replace(imgRe, QStringLiteral("<img loading=\"lazy\""));
}

/** @brief Asynchorous update web view
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotSetHtmlWebView(const QString &html)
{
  // Stop any in-flight navigation first. Calling history()->clear() while a
  // page is still loading is a known QtWebEngine 5.15 crash trigger when the
  // user quickly switches between articles (e.g. double-clicking to "read the
  // full article" right after opening another one).
  webView_->stop();
  webView_->history()->clear();
  QString pageHtml = html;
  enableImageLazyLoading(pageHtml);
  webView_->setHtml(pageHtml);
}

/** @brief Persist the article scroll position to the news_ex table (UI-3)
 *---------------------------------------------------------------------------*/
void NewsTabWidget::storeArticleScroll(int newsId, int pos)
{
  if (newsId <= 0 || pos < 0) return;

  if (articleScrollCache_.value(newsId, -1) == pos)
    return;
  articleScrollCache_.insert(newsId, pos);

  // news_ex has no UNIQUE(newsId, name) constraint, so "INSERT OR REPLACE"
  // would just append a new row every time (the auto-increment id never
  // collides) and the table would grow unboundedly. Update first, insert only
  // when the row does not exist yet.
  QSqlQuery q(db_);
  q.prepare("UPDATE news_ex SET value=:value, feedId=:feedId "
            "WHERE newsId=:newsId AND name='webScroll'");
  q.bindValue(":value", pos);
  q.bindValue(":feedId", feedId_);
  q.bindValue(":newsId", newsId);
  q.exec();
  if (q.numRowsAffected() == 0) {
    QSqlQuery qi(db_);
    qi.prepare("INSERT INTO news_ex(feedId, newsId, name, value) "
               "VALUES(:feedId, :newsId, 'webScroll', :value)");
    qi.bindValue(":feedId", feedId_);
    qi.bindValue(":newsId", newsId);
    qi.bindValue(":value", pos);
    qi.exec();
  }
}

/** @brief Read the saved article scroll position (cached) (UI-3)
 *---------------------------------------------------------------------------*/
int NewsTabWidget::articleScrollFor(int newsId)
{
  if (newsId <= 0) return 0;

  if (articleScrollCache_.contains(newsId))
    return articleScrollCache_.value(newsId);

  int pos = 0;
  QSqlQuery q(db_);
  q.prepare("SELECT value FROM news_ex WHERE newsId=? AND name='webScroll'");
  q.addBindValue(newsId);
  q.exec();
  if (q.next())
    pos = q.value(0).toInt();
  articleScrollCache_.insert(newsId, pos);
  return pos;
}

/** @brief Asynchronously capture the current WebView scroll offset (UI-3)
 *---------------------------------------------------------------------------*/
void NewsTabWidget::saveArticleScrollAsync(int newsId)
{
  if (newsId <= 0) return;
  if (type_ >= TabTypeWeb) return;
  if (currentShownNewsId_ != newsId) return;

  QPointer<NewsTabWidget> guard(this);
  webView_->page()->runJavaScript(
        "window.pageYOffset || document.documentElement.scrollTop "
        "|| document.body.scrollTop || 0",
        [guard, newsId](const QVariant &result) {
          if (!guard.isNull())
            guard->storeArticleScroll(newsId, result.toInt());
        });
}

/** @brief Restore the article scroll position (UI-3)
 *---------------------------------------------------------------------------*/
void NewsTabWidget::restoreArticleScroll(int newsId)
{
  const int pos = articleScrollFor(newsId);
  if (pos <= 0) return;

  webView_->page()->runJavaScript(
        QString("window.scrollTo(0, %1);").arg(pos));
}

/** @brief Periodic capture of the current article scroll position (UI-3)
 *---------------------------------------------------------------------------*/
void NewsTabWidget::slotSaveArticleScroll()
{
  if (type_ >= TabTypeWeb) return;
  if (currentShownNewsId_ <= 0) return;
  if (!webView_->isVisible()) return;
  // While a new page is loading, the WebView still shows the previous article;
  // capturing now would store the old offset against the new article id.
  if (!articlePageLoaded_) return;
  saveArticleScrollAsync(currentShownNewsId_);
}

void NewsTabWidget::hideWebContent()
{
  if (mainWindow_->newsLayout_ == 1) return;

  emit signalSetHtmlWebView();
  setWebToolbarVisible(false, false);
}

/** @brief Handle external link navigation request (S-4)
 *---------------------------------------------------------------------------*/
void NewsTabWidget::slotNavigationRequested(const QUrl &url)
{
  if (QMessageBox::question(this, tr("Open external link"),
        tr("Open the external link in your browser?\n\n%1").arg(url.toString()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
    return;
  }
  openUrl(url);
}

void NewsTabWidget::slotLinkClicked(QUrl url)
{
  if (url.scheme() == QLatin1String("quill")) {
    actionNewspaper(url);
    return;
  }

  if (url.scheme() == QLatin1String("mailto")) {
    QDesktopServices::openUrl(url);
    return;
  }

  // podcast://<percent-encoded-media-url> -> global player bar
  if (url.scheme() == QLatin1String("podcast")) {
    QString encoded = url.toString(QUrl::FullyEncoded);
    encoded = encoded.mid(QStringLiteral("podcast://").size());
    QUrl mediaUrl(QUrl::fromEncoded(QByteArray::fromPercentEncoding(encoded.toUtf8())));
    QString title;
    if (newsView_->currentIndex().isValid()) {
      QModelIndex curIdx = newsIndexToSource(newsView_->currentIndex());
      title = newsModel_->dataField(curIdx.row(), "title").toString();
    }
    mainWindow_->playPodcast(mediaUrl, title);
    return;
  }

  if (url.isEmpty() || !url.isValid())
    return;

  if (type_ != TabTypeWeb) {
    if ((url.host().isEmpty() || (QUrl(url).host().indexOf('.') == -1)) && newsView_->currentIndex().isValid()) {
      QModelIndex curIdx = newsIndexToSource(newsView_->currentIndex());
      int row = curIdx.row();
      int feedId = newsModel_->dataField(row, "feedId").toInt();
      QModelIndex feedIndex = feedsModel_->indexById(feedId);
      QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

      url.setScheme(hostUrl.scheme());
      url.setHost(hostUrl.host());
    }
  }

  if ((mainWindow_->externalBrowserOn_ <= 0) &&
      (webView_->buttonClick_ != LEFT_BUTTON_ALT)) {
    if (webView_->buttonClick_ == LEFT_BUTTON) {
      if (!webControlPanel_->isVisible()) {
        locationBar_->setText(url.toString());
        setWebToolbarVisible(true, false);
      }
      // Interrupt any in-progress navigation before starting a new one.
      webView_->stop();
      webView_->load(url);
    } else {
      if ((webView_->buttonClick_ == MIDDLE_BUTTON) ||
          (webView_->buttonClick_ == LEFT_BUTTON_CTRL)) {
        mainWindow_->openNewsTab_ = NEW_TAB_BACKGROUND;
      } else {
        mainWindow_->openNewsTab_ = NEW_TAB_FOREGROUND;
      }
      if (!mainWindow_->openLinkInBackgroundEmbedded_) {
        if (mainWindow_->openNewsTab_ == NEW_TAB_BACKGROUND)
          mainWindow_->openNewsTab_ = NEW_TAB_FOREGROUND;
        else
          mainWindow_->openNewsTab_ = NEW_TAB_BACKGROUND;
      }

      mainWindow_->createWebTab(url);
    }
  } else {
    openUrl(url);
  }

  webView_->buttonClick_ = 0;
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotLinkHovered(const QString &link)
{
  if (QUrl(link).scheme() == QLatin1String("quill")) return;

  mainWindow_->statusBar()->showMessage(link.simplified(), 3000);
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotSetValue(int value)
{
  emit loadProgress(value);
  // WebEngine: bytes received not directly available from QWebEnginePage
  webViewProgressLabel_->setText(QString(" %1%").arg(value));
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotLoadStarted()
{
  if (type_ == TabTypeWeb) {
    newsIconTitle_->setMovie(newsIconMovie_);
    newsIconMovie_->start();
  }

  // UI-3: suppress the periodic scroll capture until the new page has
  // finished loading (otherwise the old page's offset is stored against the
  // newly selected article id).
  articlePageLoaded_ = false;
  webViewProgress_->setValue(0);
  webViewProgress_->show();
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotLoadFinished(bool ok)
{
  if (type_ == TabTypeWeb) {
    newsIconMovie_->stop();
    QPixmap iconTab;
    iconTab.load(":/images/webPage");
    newsIconTitle_->setPixmap(iconTab);
  }

  // UI-3: restore the article scroll position after the page has loaded
  if (ok && (type_ < TabTypeWeb) && (pendingRestoreNewsId_ > 0)) {
    const int newsId = pendingRestoreNewsId_;
    pendingRestoreNewsId_ = -1;
    restoreArticleScroll(newsId);
  } else {
    pendingRestoreNewsId_ = -1;
  }

  // UI-3: the page is now rendered; the periodic scroll capture may resume.
  articlePageLoaded_ = true;
  webViewProgress_->hide();
}

void NewsTabWidget::slotUrlEnter()
{
  webView_->setFocus();

  if (!locationBar_->text().startsWith("http://") &&
      !locationBar_->text().startsWith("https://")) {
    locationBar_->setText("http://" + locationBar_->text());
  }
  locationBar_->setCursorPosition(0);

  webView_->load(QUrl(locationBar_->text()));
}

void NewsTabWidget::slotUrlChanged(const QUrl &url)
{
  locationBar_->setText(url.toString());
  locationBar_->setCursorPosition(0);
}

/** @brief Go to short news content
 *----------------------------------------------------------------------------*/
void NewsTabWidget::webHomePage()
{
  if (type_ != TabTypeWeb) {
    switch (mainWindow_->newsLayout_) {
    case 1:
      loadNewspaper();
      break;
    default:
      updateWebView(newsView_->currentIndex());
    }
  } else {
    webView_->history()->goToItem(webView_->history()->itemAt(0));
  }
}

/** @brief Open current web page in external browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openPageInExternalBrowser()
{
  openUrl(webView_->url());
}

/** @brief Open news in browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openInBrowserNews()
{
  if (type_ >= TabTypeWeb) return;

  int externalBrowserOn_ = mainWindow_->externalBrowserOn_;
  mainWindow_->externalBrowserOn_ = 0;
  slotNewsViewDoubleClicked(newsView_->currentIndex());
  mainWindow_->externalBrowserOn_ = externalBrowserOn_;
}

/** @brief Open news in external browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openInExternalBrowserNews()
{
  if (type_ == TabTypeDownloads) return;

  if (type_ != TabTypeWeb) {
    QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);
    QStringList feedIdList;

    int cnt = indexes.count();
    if (cnt == 0) return;

    for (int i = cnt-1; i >= 0; --i) {
      QSqlQuery q;
      QModelIndex curIndex = newsIndexToSource(indexes.at(i));
      if (newsModel_->dataField(curIndex.row(), "read").toInt() == 0) {
        newsModel_->setData(
              newsModel_->index(curIndex.row(), newsModel_->fieldIndex("new")),
              0);
        newsModel_->setData(
              newsModel_->index(curIndex.row(), newsModel_->fieldIndex("read")),
              1);

        int newsId = newsModel_->dataField(curIndex.row(), "id").toInt();
        q.exec(QString("UPDATE news SET new=0, read=1 WHERE id=='%2'").arg(newsId));
        QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
        if (!feedIdList.contains(feedId)) feedIdList.append(feedId);
      }

      QUrl url = QUrl::fromEncoded(getLinkNews(curIndex.row()).toUtf8());
      if (url.host().isEmpty() || (QUrl(url).host().indexOf('.') == -1)) {
        QString feedId = newsModel_->dataField(curIndex.row(), "feedId").toString();
        QModelIndex feedIndex = feedsModel_->indexById(feedId.toInt());
        QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

        url.setScheme(hostUrl.scheme());
        url.setHost(hostUrl.host());
      }

      openUrl(url);
    }

    if (!feedIdList.isEmpty()) {
      foreach (QString feedId, feedIdList) {
        mainWindow_->slotUpdateStatus(feedId.toInt());
      }
      mainWindow_->recountCategoryCounts();
      newsView_->viewport()->update();
    }
  } else {
    openUrl(webView_->url());
  }
}

void NewsTabWidget::setNewsLayout()
{
  if (type_ >= TabTypeWeb) return;

  if (mainWindow_->isFocusMode()) {
    newsWidget_->setVisible(false);
    return;
  }

  switch (mainWindow_->newsLayout_) {
  case 1:
    newsWidget_->setVisible(false);
    loadNewspaper();
    break;
  default:
    newsWidget_->setVisible(true);
    updateWebView(newsView_->currentIndex());
  }
}

/** @brief Set browser position
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setBrowserPosition()
{
  if (type_ >= TabTypeWeb) return;

  int idx = newsTabWidgetSplitter_->indexOf(webWidget_);

  switch (mainWindow_->browserPosition_) {
  case TOP_POSITION: case LEFT_POSITION:
    newsTabWidgetSplitter_->insertWidget(0, newsTabWidgetSplitter_->widget(idx));
    break;
  default:
    newsTabWidgetSplitter_->insertWidget(1, newsTabWidgetSplitter_->widget(idx));
  }

  switch (mainWindow_->browserPosition_) {
  case RIGHT_POSITION: case LEFT_POSITION:
    newsTabWidgetSplitter_->setOrientation(Qt::Horizontal);
    newsTabWidgetSplitter_->setStyleSheet(
          QString("QSplitter::handle {background: qlineargradient("
                  "x1: 0, y1: 0, x2: 0, y2: 1,"
                  "stop: 0 %1, stop: 0.07 %2);}").
          arg(newsPanelWidget_->palette().window().color().name()).
          arg(qApp->palette().color(QPalette::Dark).name()));
    break;
  default:
    newsTabWidgetSplitter_->setOrientation(Qt::Vertical);
    newsTabWidgetSplitter_->setStyleSheet(
          QString("QSplitter::handle {background: %1; margin-top: 1px; margin-bottom: 1px;}").
          arg(qApp->palette().color(QPalette::Dark).name()));
  }

  newsTabWidgetSplitter_->setChildrenCollapsible(false);
}

/** @brief Close tab while press X-button
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotTabClose()
{
  mainWindow_->slotCloseTab(mainWindow_->stackedWidget_->indexOf(this));
}

/** @brief Display browser open page title on tab
 *----------------------------------------------------------------------------*/
void NewsTabWidget::webTitleChanged(QString title)
{
  if ((type_ == TabTypeWeb) && !title.isEmpty()) {
    setTextTab(title);
  }
}

/** @brief Open news in new tab
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openNewsNewTab()
{
  if (type_ >= TabTypeWeb) return;

  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(0);

  int cnt = indexes.count();
  if (cnt == 0) return;

  for (int i = cnt-1; i >= 0; --i) {
    QModelIndex index = newsIndexToSource(indexes.at(i));
    int row = index.row();
    if (mainWindow_->markNewsReadOn_ && mainWindow_->markCurNewsRead_)
      slotSetItemRead(index, 1);

    QUrl url = QUrl::fromEncoded(getLinkNews(row).toUtf8());
    if (url.host().isEmpty() || (QUrl(url).host().indexOf('.') == -1)) {
      int feedId = newsModel_->dataField(row, "feedId").toInt();
      QModelIndex feedIndex = feedsModel_->indexById(feedId);
      QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

      url.setScheme(hostUrl.scheme());
      url.setHost(hostUrl.host());
    }

    mainWindow_->createWebTab(url);
  }
}

/** @brief Open link
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openLink()
{
  slotLinkClicked(linkUrl_);
}

/** @brief Open link in new tab
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openLinkInNewTab()
{
  int externalBrowserOn_ = mainWindow_->externalBrowserOn_;
  mainWindow_->externalBrowserOn_ = 0;

  if (QApplication::keyboardModifiers() == Qt::NoModifier) {
    webView_->buttonClick_ = MIDDLE_BUTTON;
  } else {
    webView_->buttonClick_ = MIDDLE_BUTTON_MOD;
  }

  slotLinkClicked(linkUrl_);
  mainWindow_->externalBrowserOn_ = externalBrowserOn_;
}

/** @brief Open link in browser
 *----------------------------------------------------------------------------*/
bool NewsTabWidget::openUrl(const QUrl &url)
{
  if (!url.isValid())
    return false;

  if (url.scheme() == QLatin1String("mailto"))
    return QDesktopServices::openUrl(url);

  mainWindow_->isOpeningLink_ = true;
  if ((mainWindow_->externalBrowserOn_ == 2) || (mainWindow_->externalBrowserOn_ == -1)) {
#if defined(Q_OS_WIN)
    quintptr returnValue = (quintptr)ShellExecute(
          0, 0,
          (wchar_t *)QString::fromUtf8(mainWindow_->externalBrowser_.toUtf8()).utf16(),
          (wchar_t *)QString::fromUtf8(url.toEncoded().constData()).utf16(),
          0, SW_SHOWNORMAL);
    if (returnValue > 32)
      return true;
#elif defined(Q_OS_MAC)
    return (QProcess::startDetached("open", QStringList() << "-a" <<
                                    QString::fromUtf8(mainWindow_->externalBrowser_.toUtf8()) <<
                                    QString::fromUtf8(url.toEncoded().constData())));
#else
    return (QProcess::startDetached(QString::fromUtf8(mainWindow_->externalBrowser_.toUtf8()) + QLatin1Char(' ') +
                                    QString::fromUtf8(url.toEncoded().constData())));
#endif
  }
  return QDesktopServices::openUrl(url);
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotFindText(const QString &text)
{
  QString objectName = findText_->findGroup_->checkedAction()->objectName();
  if (objectName == "findInBrowserAct") {
    webView_->findText("");
    webView_->findText(text);
  } else {
    int newsId = newsModel_->dataField(
          newsIndexToSource(newsView_->currentIndex()).row(), "id").toInt();

    QString filterStr;
    switch (type_) {
    case TabTypeUnread:
    case TabTypeStar:
    case TabTypeDel:
    case TabTypeLabel:
      filterStr = categoryFilterStr_;
      break;
    default:
      filterStr = mainWindow_->newsFilterStr;
    }

    if (!text.isEmpty()) {
      QString findText = text;
      findText = findText.replace("'", "''").toUpper();
      if (objectName == "findTitleAct") {
        filterStr.append(
              QString(" AND UPPER(title) LIKE '%%1%'").arg(findText));
      } else if (objectName == "findAuthorAct") {
        filterStr.append(
              QString(" AND UPPER(author_name) LIKE '%%1%'").arg(findText));
      } else if (objectName == "findCategoryAct") {
        filterStr.append(
              QString(" AND UPPER(category) LIKE '%%1%'").arg(findText));
      } else if (objectName == "findContentAct") {
        // FTS5 fast path for ASCII terms: the unicode61 tokenizer treats a run
        // of CJK characters as a single token, so Chinese substrings would not
        // match - those keep the LIKE fallback. matchTerm() returns a quoted
        // phrase with no single quotes, safe to embed in this SQL string.
        bool ftsAvailable = false;
        if (FtsSearch::isAsciiOnly(text)) {
          QSqlQuery qfts(db_);
          qfts.setForwardOnly(true);
          qfts.exec("SELECT count(*) FROM sqlite_master "
                    "WHERE name='news_fts' AND type='table'");
          if (qfts.next())
            ftsAvailable = qfts.value(0).toInt() > 0;
        }
        if (ftsAvailable) {
          filterStr.append(QString(
                " AND EXISTS (SELECT 1 FROM news_fts WHERE news_fts MATCH '%1' "
                "AND news_fts.rowid = news.id)").
              arg(FtsSearch::matchTerm(findText)));
        } else {
          filterStr.append(
                QString(" AND (UPPER(content) LIKE '%%1%' OR UPPER(description) LIKE '%%1%')").
                arg(findText));
        }
      } else if (objectName == "findLinkAct") {
        filterStr.append(
              QString(" AND link_href LIKE '%%1%'").
              arg(findText));
      } else {
        filterStr.append(
              QString(" AND (UPPER(title) LIKE '%%1%' OR UPPER(author_name) LIKE '%%1%' "
                      "OR UPPER(category) LIKE '%%1%' OR UPPER(content) LIKE '%%1%' "
                      "OR UPPER(description) LIKE '%%1%')").
              arg(findText));
      }
    }

    newsModel_->setFilter(filterStr);
    // Paging: the filtered result is initially limited to one page; the
    // target row may live further out, so load the whole result set.
    newsModel_->fetchAll();

    QModelIndex index = newsModel_->index(0, newsModel_->fieldIndex("id"));
    QModelIndexList indexList = newsModel_->match(index, Qt::EditRole, newsId);
    if (indexList.count()) {
      int newsRow = indexList.first().row();
      newsView_->setCurrentIndex(newsIndexFromSource(
            newsModel_->index(newsRow, newsModel_->fieldIndex("title"))));
    } else {
      currentNewsIdOld = newsId;
      hideWebContent();
    }
  }
}
//----------------------------------------------------------------------------
void NewsTabWidget::slotSelectFind()
{
  webView_->findText("");
  slotFindText(findText_->text());
}
//----------------------------------------------------------------------------
void NewsTabWidget::showContextWebPage(const QPoint &p)
{
  QMenu menu;
  QMenu *pageMenu = webView_->page()->createStandardContextMenu();
  if (pageMenu) {
    menu.addActions(pageMenu->actions());

    webView_->page()->action(QWebEnginePage::DownloadLinkToDisk)->setText(tr("Save Link..."));
    webView_->page()->action(QWebEnginePage::DownloadImageToDisk)->setText(tr("Save Image..."));
    webView_->page()->action(QWebEnginePage::CopyLinkToClipboard)->setText(tr("Copy Link"));
    webView_->page()->action(QWebEnginePage::Copy)->setText(tr("Copy"));
    webView_->page()->action(QWebEnginePage::Back)->setText(tr("Go Back"));
    webView_->page()->action(QWebEnginePage::Forward)->setText(tr("Go Forward"));
    webView_->page()->action(QWebEnginePage::Stop)->setText(tr("Stop"));
    webView_->page()->action(QWebEnginePage::Reload)->setText(tr("Reload"));
    webView_->page()->action(QWebEnginePage::CopyImageToClipboard)->setText(tr("Copy Image"));
    webView_->page()->action(QWebEnginePage::CopyImageUrlToClipboard)->setText(tr("Copy Image Address"));

    linkUrl_ = QUrl();
    if (mainWindow_->externalBrowserOn_ <= 0) {
      menu.addSeparator();
      menu.addAction(urlExternalBrowserAct_);
    }

    // Add article-specific actions (reload / print / save) for the news view.
    if (pageMenu->actions().indexOf(webView_->pageAction(QWebEnginePage::Reload)) >= 0) {
      if (webView_->title() == "news_descriptions") {
        webView_->pageAction(QWebEnginePage::Reload)->setVisible(false);
      } else {
        webView_->pageAction(QWebEnginePage::Reload)->setVisible(true);
        menu.addSeparator();
      }
      menu.addAction(mainWindow_->autoLoadImagesToggle_);
      menu.addSeparator();
      menu.addAction(mainWindow_->printAct_);
      menu.addAction(mainWindow_->printPreviewAct_);
      menu.addSeparator();
      menu.addAction(mainWindow_->savePageAsAct_);
    }

    {
      menu.addSeparator();
      menu.addAction(mainWindow_->adBlockIcon()->menuAction());
    }

    menu.exec(webView_->mapToGlobal(p));
  }
}

/** @brief Open link in external browser
 *----------------------------------------------------------------------------*/
void NewsTabWidget::openUrlInExternalBrowser()
{
  if (linkUrl_.scheme() == QLatin1String("mailto")) {
    QDesktopServices::openUrl(linkUrl_);
    return;
  }

  if (type_ != TabTypeWeb) {
    if (linkUrl_.host().isEmpty() && newsView_->currentIndex().isValid()) {
      QModelIndex curIdx = newsIndexToSource(newsView_->currentIndex());
      int row = curIdx.row();
      int feedId = newsModel_->dataField(row, "feedId").toInt();
      QModelIndex feedIndex = feedsModel_->indexById(feedId);
      QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

      linkUrl_.setScheme(hostUrl.scheme());
      linkUrl_.setHost(hostUrl.host());
    }
  }
  openUrl(linkUrl_);
}

void NewsTabWidget::setWebToolbarVisible(bool show, bool checked)
{
  if (!checked) webToolbarShow_ = show;
  webControlPanel_->setVisible(webToolbarShow_ &
                               mainWindow_->browserToolbarToggle_->isChecked());

}

/** @brief Set label for selected news
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setLabelNews(int labelId)
{
  if (type_ >= TabTypeWeb) return;

  QList<QModelIndex> indexes = newsView_->selectionModel()->selectedRows(
        newsModel_->fieldIndex("label"));

  int cnt = indexes.count();
  if (cnt == 0) return;

  if (cnt == 1) {
    QModelIndex index = newsIndexToSource(indexes.at(0));
    QString strIdLabels = index.data(Qt::EditRole).toString();
    if (!strIdLabels.contains(QString(",%1,").arg(labelId))) {
      if (strIdLabels.isEmpty()) strIdLabels.append(",");
      strIdLabels.append(QString::number(labelId));
      strIdLabels.append(",");
    } else {
      strIdLabels.replace(QString(",%1,").arg(labelId), ",");
    }
    newsModel_->setData(index, strIdLabels);

    int newsId = newsModel_->dataField(index.row(), "id").toInt();

    if ((newsId == currentNewsIdOld) &&
        (webView_->title() == "news_descriptions")) {
      // WebEngine: DOM update via runJavaScript
      webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
      QString labelsString = getHtmlLabels(index.row());
      QString escapedLabels = QString::fromUtf8(QJsonDocument(QJsonArray() << labelsString).toJson(QJsonDocument::Compact).trimmed().mid(1).chopped(1));
      QString js = QString(
        "(function() {"
        "  var el = document.getElementById('labels%1');"
        "  if (el) { el.innerHTML = %2; }"
        "})();"
      ).arg(newsId).arg(escapedLabels);
      webView_->page()->runJavaScript(js);
      webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, autoLoadImages_);
    }

    QSqlQuery q;
    q.exec(QString("UPDATE news SET label='%1' WHERE id=='%2'").
           arg(strIdLabels).arg(newsId));
    if (newsId != currentNewsIdOld) {
      newsView_->selectionModel()->select(
            index, QItemSelectionModel::Deselect|QItemSelectionModel::Rows);
    }
  } else {
    bool setLabel = false;
    for (int i = cnt-1; i >= 0; --i) {
      QModelIndex index = newsIndexToSource(indexes.at(i));
      QString strIdLabels = index.data(Qt::EditRole).toString();
      if (!strIdLabels.contains(QString(",%1,").arg(labelId))) {
        setLabel = true;
        break;
      }
    }

    db_.transaction();
    for (int i = cnt-1; i >= 0; --i) {
      QModelIndex index = newsIndexToSource(indexes.at(i));
      QString strIdLabels = index.data(Qt::EditRole).toString();
      if (setLabel) {
        if (strIdLabels.contains(QString(",%1,").arg(labelId))) continue;
        if (strIdLabels.isEmpty()) strIdLabels.append(",");
        strIdLabels.append(QString::number(labelId));
        strIdLabels.append(",");
      } else {
        strIdLabels.replace(QString(",%1,").arg(labelId), ",");
      }
      newsModel_->setData(index, strIdLabels);

      int newsId = newsModel_->dataField(index.row(), "id").toInt();

      if ((newsId == currentNewsIdOld) &&
          (webView_->title() == "news_descriptions")) {
        // WebEngine: DOM update via runJavaScript
        QString labelsString = getHtmlLabels(index.row());
        QString escapedLabels = QString::fromUtf8(QJsonDocument(QJsonArray() << labelsString).toJson(QJsonDocument::Compact).trimmed().mid(1).chopped(1));
        QString js = QString(
          "(function() {"
          "  var el = document.getElementById('labels%1');"
          "  if (el) { el.innerHTML = %2; }"
          "})();"
        ).arg(newsId).arg(escapedLabels);
        webView_->page()->runJavaScript(js);
      }

      QSqlQuery q;
      q.exec(QString("UPDATE news SET label='%1' WHERE id=='%2'").
             arg(strIdLabels).arg(newsId));
      if (newsId != currentNewsIdOld) {
        newsView_->selectionModel()->select(
              index, QItemSelectionModel::Deselect|QItemSelectionModel::Rows);
      }
    }
    db_.commit();
  }
  newsView_->viewport()->update();
  mainWindow_->recountCategoryCounts();
}

void NewsTabWidget::slotNewslLabelClicked(QModelIndex index)
{
  if (!newsView_->selectionModel()->isSelected(index)) {
    newsView_->selectionModel()->clearSelection();
    newsView_->selectionModel()->select(
          index, QItemSelectionModel::Select|QItemSelectionModel::Rows);
  }
  mainWindow_->newsLabelMenu_->popup(
        newsView_->viewport()->mapToGlobal(newsView_->visualRect(index).bottomLeft()));
}

void NewsTabWidget::showLabelsMenu()
{
  if (type_ >= TabTypeWeb) return;
  if (!newsView_->currentIndex().isValid()) return;

  for (int i = newsHeader_->count()-1; i >= 0; i--) {
    int lIdx = newsHeader_->logicalIndex(i);
    if (!newsHeader_->isSectionHidden(lIdx)) {
      int row = newsIndexToSource(newsView_->currentIndex()).row();
      slotNewslLabelClicked(newsModel_->index(row, lIdx));
      break;
    }
  }
}

void NewsTabWidget::reduceNewsList()
{
  if (type_ >= TabTypeWeb) return;

  QList <int> sizes = newsTabWidgetSplitter_->sizes();
  sizes.insert(0, sizes.takeAt(0) - RESIZESTEP);
  newsTabWidgetSplitter_->setSizes(sizes);
}

void NewsTabWidget::increaseNewsList()
{
  if (type_ >= TabTypeWeb) return;

  QList <int> sizes = newsTabWidgetSplitter_->sizes();
  sizes.insert(0, sizes.takeAt(0) + RESIZESTEP);
  newsTabWidgetSplitter_->setSizes(sizes);
}

/** @brief Search unread news
 * @param next search condition: true - search next, else - previous
 *----------------------------------------------------------------------------*/
int NewsTabWidget::findUnreadNews(bool next)
{
  int newsRow = -1;

  // Paging: unread may live beyond the loaded page - load everything first.
  newsModel_->fetchAll();

  int newsRowCur = newsIndexToSource(newsView_->currentIndex()).row();
  QModelIndex index;
  QModelIndexList indexList;
  if (next) {
    index = newsModel_->index(newsRowCur+1, newsModel_->fieldIndex("read"));
    indexList = newsModel_->match(index, Qt::EditRole, 0);
    if (indexList.isEmpty()) {
      index = newsModel_->index(0, newsModel_->fieldIndex("read"));
      indexList = newsModel_->match(index, Qt::EditRole, 0);
    }
  } else {
    index = newsModel_->index(newsRowCur, newsModel_->fieldIndex("read"));
    indexList = newsModel_->match(index, Qt::EditRole, 0, -1);
  }
  if (!indexList.isEmpty()) newsRow = indexList.last().row();

  return newsRow;
}

/** @brief Set tab title
 *----------------------------------------------------------------------------*/
void NewsTabWidget::setTextTab(const QString &text)
{
  int padding = 15;

  if (closeButton_->isHidden())
    padding = 0;

  QString textTab = newsTextTitle_->fontMetrics().elidedText(
        text, Qt::ElideRight, newsTitleLabel_->width() - 16 - 3 - padding);
  newsTextTitle_->setText(textTab);
  newsTitleLabel_->setToolTip(text);

  emit signalSetTextTab(text, this);
}

/** @brief Share news
 *----------------------------------------------------------------------------*/
void NewsTabWidget::slotShareNews(QAction *action)
{
  bool externalApp = false;

  QList<QModelIndex> indexes;
  int cnt = 0;
  if (type_ < TabTypeWeb) {
    indexes = newsView_->selectionModel()->selectedRows(0);
    cnt = indexes.count();
  } else if (type_ == TabTypeWeb) {
    cnt = 1;
  }
  if (cnt == 0) return;

  for (int i = cnt-1; i >= 0; --i) {
    QString title;
    QString linkString;
    QString content;
    if (type_ < TabTypeWeb) {
      int row = newsIndexToSource(indexes.at(i)).row();
      title = newsModel_->dataField(row, "title").toString();
      linkString = getLinkNews(row);

      content = newsModel_->dataField(row, "content").toString();
      QString description = newsModel_->dataField(row, "description").toString();
      if (content.isEmpty() || (description.length() > content.length())) {
        content = description;
      }
      QTextDocumentFragment textDocument = QTextDocumentFragment::fromHtml(content);
      content = textDocument.toPlainText();
    } else {
      title = webView_->title();
      linkString = webView_->url().toString();
      content = webView_->page()->selectedText();
    }
#if defined(Q_OS_WIN) || defined(Q_OS_OS2) || defined(Q_OS_MAC)
    content = content.replace("\n", "%0A");
    content = content.replace("\"", "%22");
#endif

    QUrl url;
    if (action->objectName() == "emailShareAct") {
      url.setUrl("mailto:");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("subject", title);
      urlQuery.addQueryItem("body", linkString);
      //#if defined(Q_OS_WIN) || defined(Q_OS_OS2) || defined(Q_OS_MAC)
      //      urlQuery.addQueryItem("body", linkString + "%0A%0A" + content);
      //#else
      //      urlQuery.addQueryItem("body", linkString + "\n\n" + content);
      //#endif
      url.setQuery(urlQuery);
#else
      url.addQueryItem("subject", title);
#if defined(Q_OS_WIN) || defined(Q_OS_OS2) || defined(Q_OS_MAC)
      url.addQueryItem("body", linkString + "%0A%0A" + content);
#else
      url.addQueryItem("body", linkString + "\n\n" + content);
#endif
#endif
      externalApp = true;
    } else if (action->objectName() == "evernoteShareAct") {
      url.setUrl("https://www.evernote.com/clip.action");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      urlQuery.addQueryItem("title", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
      url.addQueryItem("title", title);
#endif
    } else if (action->objectName() == "facebookShareAct") {
      url.setUrl("https://www.facebook.com/sharer.php");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("u", linkString);
      urlQuery.addQueryItem("t", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("u", linkString);
      url.addQueryItem("t", title);
#endif
    } else if (action->objectName() == "livejournalShareAct") {
      url.setUrl("http://www.livejournal.com/update.bml");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("event", linkString);
      urlQuery.addQueryItem("subject", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("event", linkString);
      url.addQueryItem("subject", title);
#endif
    } else if (action->objectName() == "pocketShareAct") {
      url.setUrl("https://getpocket.com/save");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      urlQuery.addQueryItem("title", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
      url.addQueryItem("title", title);
#endif
    } else if (action->objectName() == "twitterShareAct") {
      url.setUrl("https://twitter.com/share");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      urlQuery.addQueryItem("text", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
      url.addQueryItem("text", title);
#endif
    } else if (action->objectName() == "vkShareAct") {
      url.setUrl("https://vk.com/share.php");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      urlQuery.addQueryItem("title", title);
      urlQuery.addQueryItem("description", "");
      urlQuery.addQueryItem("image", "");
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
      url.addQueryItem("title", title);
      url.addQueryItem("description", "");
      url.addQueryItem("image", "");
#endif
    } else if (action->objectName() == "linkedinShareAct") {
      url.setUrl("https://www.linkedin.com/shareArticle?mini=true");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      urlQuery.addQueryItem("title", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
      url.addQueryItem("title", title);
#endif
    } else if (action->objectName() == "bloggerShareAct") {
      url.setUrl("https://www.blogger.com/blog_this.pyra?t");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("u", linkString);
      urlQuery.addQueryItem("n", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("u", linkString);
      url.addQueryItem("n", title);
#endif
    } else if (action->objectName() == "printfriendlyShareAct") {
      url.setUrl("https://www.printfriendly.com/print");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
#endif
    } else if (action->objectName() == "instapaperShareAct") {
      url.setUrl("https://www.instapaper.com/hello2");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      urlQuery.addQueryItem("title", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
      url.addQueryItem("title", title);
#endif
    } else if (action->objectName() == "redditShareAct") {
      url.setUrl("https://reddit.com/submit");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      urlQuery.addQueryItem("title", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
      url.addQueryItem("title", title);
#endif
    } else if (action->objectName() == "hackerNewsShareAct") {
      url.setUrl("http://news.ycombinator.com/submitlink");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("u", linkString);
      urlQuery.addQueryItem("t", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("u", linkString);
      url.addQueryItem("t", title);
#endif
    } else if (action->objectName() == "telegramShareAct") {
      url.setUrl("tg://msg_url");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("url", linkString);
      urlQuery.addQueryItem("text", title);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("url", linkString);
      url.addQueryItem("text", title);
#endif
      externalApp = true;
    } else if (action->objectName() == "viberShareAct") {
      url.setUrl("viber://forward");
#ifdef HAVE_QT5
      QUrlQuery urlQuery;
      urlQuery.addQueryItem("text", title + "%20" + linkString);
      url.setQuery(urlQuery);
#else
      url.addQueryItem("text", title + "%20" + linkString);
#endif
      externalApp = true;
    }

    if ((mainWindow_->externalBrowserOn_ <= 0) && !externalApp) {
      mainWindow_->openNewsTab_ = NEW_TAB_FOREGROUND;
      mainWindow_->createWebTab(url);
    } else {
      QDesktopServices::openUrl(url);
    }
  }
}
//-----------------------------------------------------------------------------
int NewsTabWidget::getUnreadCount(QString countString)
{
  if (countString.isEmpty()) return 0;

  countString.remove(QzRegExp("[()]"));
  switch (type_) {
  case TabTypeUnread:
    return countString.toInt();
  case TabTypeStar:
  case TabTypeLabel:
    return countString.section("/", 0, 0).toInt();
  default:
    return 0;
  }
}

QString NewsTabWidget::getLinkNews(int row)
{
  QString linkString = newsModel_->dataField(row, "link_href").toString();
  if (linkString.isEmpty())
    linkString = newsModel_->dataField(row, "link_alternate").toString();
  return linkString.simplified();
}

void NewsTabWidget::savePageAsDescript()
{
  if (type_ >= TabTypeWeb) return;

  QModelIndex curIndex = newsIndexToSource(newsView_->currentIndex());
  if (!curIndex.isValid()) return;

  // QWebEnginePage::toHtml() is asynchronous - capture row data before callback
  const int row = curIndex.row();
  const int newsId = newsModel_->dataField(row, "id").toInt();
  QPointer<NewsTabWidget> guard(this);

  webView_->page()->toHtml([guard, row, newsId](const QString &result) {
    if (!guard) return;

    QString html = result;
    html.replace("'", "''");
    guard->newsModel_->setData(
          guard->newsModel_->index(row, guard->newsModel_->fieldIndex("content")),
          html);
    QString qStr = QString("UPDATE news SET content='%1' WHERE id=='%2'").
        arg(html).arg(newsId);
    mainApp->sqlQueryExec(qStr);
  });
}

QString NewsTabWidget::getHtmlLabels(int row)
{
  QStringList strLabelIdList = newsModel_->dataField(row, "label").toString().
      split(",", Qt::SkipEmptyParts);
  QString labelsString;
  QList<QTreeWidgetItem *> labelListItems = mainWindow_->categoriesTree_->getLabelListItems();
  foreach (QTreeWidgetItem *item, labelListItems) {
    if (strLabelIdList.contains(item->text(2))) {
      strLabelIdList.removeOne(item->text(2));
      QByteArray byteArray = item->data(0, CategoriesTreeWidget::ImageRole).toByteArray();
      labelsString.append(QString("<td><img class='quill-img' src=\"data:image/png;base64,") % byteArray.toBase64() % "\"/></td>");
      labelsString.append("<td>" % item->text(0));
      if (strLabelIdList.count())
        labelsString.append(",");
      labelsString.append("</td>");
    }
  }
  return labelsString;
}

void NewsTabWidget::actionNewspaper(QUrl url)
{
  QString newsId = url.fragment();
  QModelIndex startIndex = newsModel_->index(0, newsModel_->fieldIndex("id"));
  QModelIndexList indexList = newsModel_->match(startIndex, Qt::EditRole, newsId);
  if (!indexList.isEmpty()) {
    QString iconStr;
    if (url.host() == "read.action.ui") {
      QString titleStyle;
      if (newsModel_->dataField(indexList.first().row(), "read").toInt() == 0) {
        slotSetItemRead(indexList.first(), 1);
        iconStr = "qrc:/images/bulletRead";
        titleStyle = "read";
      } else {
        slotSetItemRead(indexList.first(), 0);
        iconStr = "qrc:/images/bulletUnread";
        titleStyle = "unread";
      }
      // WebEngine: DOM update via runJavaScript
      webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
      QString js = QString(
        "(function() {"
        "  var newsItem = document.getElementById('newsItem%1');"
        "  if (newsItem) {"
        "    var img = document.getElementById('readAction%1');"
        "    if (img) img.setAttribute('src', '%2');"
        "    var title = document.getElementById('title%1');"
        "    if (title) title.setAttribute('class', '%3');"
        "  }"
        "})();"
      ).arg(newsId).arg(iconStr).arg(titleStyle);
      webView_->page()->runJavaScript(js);
      webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, autoLoadImages_);
    } else if (url.host() == "star.action.ui") {
      int row = indexList.first().row();
      if (newsModel_->dataField(row, "starred").toInt() == 0) {
        slotSetItemStar(newsModel_->index(row, newsModel_->fieldIndex("starred")), 1);
        iconStr = "qrc:/images/starOn";
      } else {
        slotSetItemStar(newsModel_->index(row, newsModel_->fieldIndex("starred")), 0);
        iconStr = "qrc:/images/starOff";
      }
      // WebEngine: DOM update via runJavaScript
      webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, true);
      QString jsStar = QString(
        "(function() {"
        "  var img = document.getElementById('starAction%1');"
        "  if (img) img.setAttribute('src', '%2');"
        "})();"
      ).arg(newsId).arg(iconStr);
      webView_->page()->runJavaScript(jsStar);
      webView_->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, autoLoadImages_);
    } else if (url.host() == "labels.menu.ui") {
      newsView_->selectionModel()->clearSelection();
      newsView_->selectionModel()->select(
            indexList.first(), QItemSelectionModel::Select|QItemSelectionModel::Rows);
      currentNewsIdOld = newsId.toInt();
      mainWindow_->newsLabelMenu_->popup(QCursor::pos());
    } else if (url.host() == "share.menu.ui") {
      newsView_->selectionModel()->clearSelection();
      newsView_->selectionModel()->select(
            indexList.first(), QItemSelectionModel::Select|QItemSelectionModel::Rows);
      currentNewsIdOld = newsId.toInt();
      mainWindow_->shareMenu_->popup(QCursor::pos());
    } else if (url.host() == "open.browser.ui") {
      QUrl url = QUrl::fromEncoded(getLinkNews(indexList.first().row()).toUtf8());
      if (url.host().isEmpty() || (QUrl(url).host().indexOf('.') == -1)) {
        QString feedId = newsModel_->dataField(indexList.first().row(), "feedId").toString();
        QModelIndex feedIndex = feedsModel_->indexById(feedId.toInt());
        QUrl hostUrl = feedsModel_->dataField(feedIndex, "htmlUrl").toString();

        if (!hostUrl.isEmpty()) {
          url.setScheme(hostUrl.scheme());
          url.setHost(hostUrl.host());
        }
      }
      if (url.isEmpty() || !url.isValid())
        return;
      openUrl(url);
    } else if (url.host() == "delete.action.ui") {
      newsView_->selectionModel()->clearSelection();
      newsView_->selectionModel()->select(
            indexList.first(), QItemSelectionModel::Select|QItemSelectionModel::Rows);
      deleteNews();
      // WebEngine: DOM update via runJavaScript
      QString jsDel = QString(
        "(function() {"
        "  var el = document.getElementById('newsItem%1');"
        "  if (el) el.remove();"
        "})();"
      ).arg(newsId);
      webView_->page()->runJavaScript(jsDel);
    }
  }
}
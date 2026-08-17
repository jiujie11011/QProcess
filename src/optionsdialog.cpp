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
#include "optionsdialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSpinBox>

#include "mainapplication.h"
#include "../ai/aiassistant.h"
#include "labeldialog.h"
#include "rsshubinstances.h"
#include "settings.h"
#include "VersionNo.h"

OptionsDialog::OptionsDialog(QWidget *parent)
  : Dialog(parent)
  , notificationWidget_(NULL)
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowTitle(tr("Options"));

  db_ = QSqlDatabase::database();
  aiAssistant_ = new AIAssistant(this);

  contentLabel_ = new QLabel();
  contentLabel_->setObjectName("contentLabel_");
  contentLabel_->setAlignment(Qt::AlignCenter);
  contentLabel_->setStyleSheet(
        QString("#contentLabel_ {border-bottom: 1px solid %1;}").
        arg(qApp->palette().color(QPalette::Dark).name()));
  contentLabel_->setMinimumHeight(36);
  contentLabel_->setMargin(4);
  QFont fontContentLabel = contentLabel_->font();
  fontContentLabel.setBold(true);
  fontContentLabel.setPointSize(fontContentLabel.pointSize()+2);
  contentLabel_->setFont(fontContentLabel);
  mainLayout->insertWidget(0, contentLabel_);

  categoriesTree_ = new QTreeWidget();
  categoriesTree_->setObjectName("categoriesTree");
  categoriesTree_->setStyleSheet("QTreeWidget::item { min-height: 20px; }"
                                 "QTreeWidget { padding: 1px; }");
  categoriesTree_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  categoriesTree_->setHeaderHidden(true);
  categoriesTree_->setColumnCount(3);
  categoriesTree_->setColumnHidden(0, true);
  categoriesTree_->header()->setStretchLastSection(false);
  categoriesTree_->header()->resizeSection(2, 5);
#ifdef HAVE_QT5
  categoriesTree_->header()->setSectionResizeMode(1, QHeaderView::Stretch);
#else
  categoriesTree_->header()->setResizeMode(1, QHeaderView::Stretch);
#endif
  categoriesTree_->setMinimumWidth(150);
  QStringList treeItem;
  treeItem << "0" << tr("General");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("System Tray");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("Network Connections");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "3" << tr("Browser");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "4" << tr("Feeds");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "5" << tr("Labels");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "6" << tr("Notifications");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "7" << tr("Passwords");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "8" << tr("Language");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "9" << tr("Fonts & Colors");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "10" << tr("Keyboard Shortcuts");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "11" << tr("Interaction");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "12" << tr("Cleanup");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "13" << tr("AI");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "14" << tr("Manage Subscriptions");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "15" << tr("RSSHub Instances");
  categoriesTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));

  createGeneralWidget();

  createTraySystemWidget();

  createNetworkConnectionsWidget();

  createBrowserWidget();

  createFeedsWidget();

  createLabelsWidget();

  createNotifierWidget();

  createPasswordsWidget();

  createLanguageWidget();

  createFontsColorsWidget();

  createShortcutWidget();

  createInteractionWidget();

  createCleanupWidget();

  createAIWidget();
  loadAiSettings();

  createManageSubscriptionsWidget();
  createRssHubWidget();
  loadRssHubSettings();

  contentStack_ = new QStackedWidget();
  contentStack_->setObjectName("contentStack_");
  contentStack_->addWidget(generalWidget_);
  contentStack_->addWidget(traySystemWidget_);
  contentStack_->addWidget(networkConnectionsWidget_);
  contentStack_->addWidget(browserWidget_);
  contentStack_->addWidget(feedsWidget_);
  contentStack_->addWidget(labelsWidget_);
  contentStack_->addWidget(notifierWidget_);
  contentStack_->addWidget(passwordsWidget_);
  contentStack_->addWidget(languageWidget_);
  contentStack_->addWidget(fontsColorsWidget_);
  contentStack_->addWidget(shortcutWidget_);
  contentStack_->addWidget(interactionWidget_);
  contentStack_->addWidget(cleanupWidget_);
  contentStack_->addWidget(aiWidget_);
  contentStack_->addWidget(manageSubscriptionsWidget_);
  contentStack_->addWidget(rsshubWidget_);

  scrollArea_ = new QScrollArea(this);
  scrollArea_->setWidgetResizable(true);
  scrollArea_->setFrameStyle(QFrame::NoFrame);
  scrollArea_->setWidget(contentStack_);

  QSplitter *splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  splitter->addWidget(categoriesTree_);
  splitter->addWidget(scrollArea_);
  QList<int> sizes;
  sizes << 150 << 600;
  splitter->setSizes(sizes);

  pageLayout->addWidget(splitter, 1);
  pageLayout->setContentsMargins(10, 5, 10, 5);

  buttonBox->addButton(QDialogButtonBox::Ok);
  buttonBox->addButton(QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptDialog()));
  connect(this, SIGNAL(finished(int)), this, SLOT(closeDialog()));

  connect(categoriesTree_, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
          this, SLOT(slotCategoriesItemClicked(QTreeWidgetItem*,int)));
  connect(this, SIGNAL(signalCategoriesTreeKeyUpDownPressed()),
          SLOT(slotCategoriesTreeKeyUpDownPressed()), Qt::QueuedConnection);

  categoriesTree_->installEventFilter(this);

  setMinimumSize(500, 400);
  resize(700, 580);

  Settings settings;
  restoreGeometry(settings.value("options/geometry").toByteArray());
}

void OptionsDialog::showEvent(QShowEvent*event)
{
  const QRect desktop = Common::desktopAvailableGeometry();
  int desktopWidth = desktop.width();
  int desktopHeight = desktop.height();
  int maxWidth = desktopWidth - (frameSize().width() - width());
  int maxHeight = desktopHeight - (frameSize().height() - height());

  setMaximumSize(maxWidth, maxHeight);

  if (frameSize().height() >= desktopHeight) {
    move(desktop.topLeft());
  }

  Dialog::showEvent(event);
}

/** @brief Enable custom date format line edit when "Custom..." selected
 *---------------------------------------------------------------------------*/
void OptionsDialog::slotCustomDateFormatChanged(int index)
{
  bool custom = (formatDate_->itemData(index).toString() == "custom");
  customDateFormat_->setEnabled(custom);
  if (custom)
    customDateFormat_->setFocus();
}

/** @brief Apply accent color preset to the custom hex edit
 *---------------------------------------------------------------------------*/
void OptionsDialog::slotAccentColorChanged(int index)
{
  accentColorEdit_->setText(accentColorCombo_->itemData(index).toString());
}

/** @brief Select accent color in dialog controls
 *---------------------------------------------------------------------------*/
void OptionsDialog::selectAccentColor(const QString &color)
{
  if (color.isEmpty()) {
    accentColorCombo_->setCurrentIndex(0);
    accentColorEdit_->clear();
    return;
  }
  int idx = accentColorCombo_->findData(color);
  accentColorCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
  accentColorEdit_->setText(color);
}

/** @brief Return currently selected accent color ("" = theme default)
 *---------------------------------------------------------------------------*/
QString OptionsDialog::accentColor()
{
  QString preset = accentColorCombo_->itemData(
        accentColorCombo_->currentIndex()).toString();
  if (!preset.isEmpty())
    return preset;
  QString hex = accentColorEdit_->text().trimmed();
  static QRegularExpression hexRe("^#[0-9a-fA-F]{6}$");
  if (hexRe.match(hex).hasMatch())
    return hex.toUpper();
  return QString();
}

void OptionsDialog::acceptDialog()
{
#if defined(Q_OS_WIN)
  if (mainApp->isPortableAppsCom()) {
    if (autoRunEnabled_->isChecked()) {
      QFileInfo file(QCoreApplication::applicationDirPath() % "/../../QuiteRSSPortable.exe");
      autoRunSettings_->setValue("QuiteRSSPortable", QDir::toNativeSeparators(file.absoluteFilePath()));
    } else {
      autoRunSettings_->remove("QuiteRSSPortable");
    }
  } else {
    if (autoRunEnabled_->isChecked())
      autoRunSettings_->setValue("QuiteRSS", QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    else
      autoRunSettings_->remove("QuiteRSS");
  }
#endif

  applyProxy();
  applyWhitelist();
  applyLabels();
  applyNotifier();
  applyPass();
  saveAiSettings();
  saveRssHubSettings();
  accept();
}

void OptionsDialog::closeDialog()
{
  Settings settings;
  settings.setValue("options/geometry", saveGeometry());
}

bool OptionsDialog::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    if (obj != editShortcut_) {
      if ((keyEvent->key() == Qt::Key_Up) ||
          (keyEvent->key() == Qt::Key_Down)) {
        if (obj == categoriesTree_)
          emit signalCategoriesTreeKeyUpDownPressed();
        else if (obj == shortcutTree_)
          emit signalShortcutTreeUpDownPressed();
      }
    } else {
      if ((keyEvent->key() < Qt::Key_Shift) ||
          (keyEvent->key() > Qt::Key_Alt)) {
        QString str;
        if ((keyEvent->modifiers() & Qt::ShiftModifier) ||
            (keyEvent->modifiers() & Qt::ControlModifier) ||
            (keyEvent->modifiers() & Qt::AltModifier))
          str.append(QKeySequence(keyEvent->modifiers()).toString());
        if (keyEvent->key() == Qt::Key_Backtab)
          str.append(QKeySequence(Qt::Key_Tab).toString());
        else
          str.append(QKeySequence(keyEvent->key()).toString());
        editShortcut_->setText(str);

        QModelIndex index = shortcutProxyModel_->mapToSource(shortcutTree_->currentIndex());
        int row = index.row();
        QString shortcutStr = shortcutModel_->item(row, 2)->text();
        QList<QStandardItem *> treeItems;
        treeItems = shortcutModel_->findItems(shortcutStr, Qt::MatchFixedString, 2);
          if (!shortcutStr.isEmpty()) {
            for (int i = 0; i < treeItems.count(); i++) {
              if ((treeItems.count() == 2) || (treeItems.at(i)->row() == row)) {
                treeItems.at(i)->setData(shortcutModel_->item(0, 1)->data(Qt::TextColorRole),
                                         Qt::TextColorRole);
              }
            }
          }

        shortcutModel_->item(row, 2)->setText(str);
        shortcutModel_->item(row, 2)->setData(str);

        if (!str.isEmpty()) {
          treeItems = shortcutModel_->findItems(str, Qt::MatchFixedString, 2);
          if (treeItems.count() > 1) {
            for (int i = 0; i < treeItems.count(); i++) {
              if (treeItems.at(i)->row() != row) {
                warningShortcut_->setText(tr("Warning: key is already assigned to") +
                                          " '" +
                                          shortcutModel_->item(treeItems.at(i)->row(), 0)->text()
                                          + "'");
              }
              treeItems.at(i)->setData(QColor(Qt::red), Qt::TextColorRole);
            }
          } else {
            warningShortcut_->clear();
          }
        }
      }
      return true;
    }
    return false;
  } else {
    return QDialog::eventFilter(obj, event);
  }
}

/** @brief Create windget "General"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createGeneralWidget()
{
  showSplashScreen_ = new QCheckBox(tr("Show splash screen on startup"));
  reopenFeedStartup_ = new QCheckBox(tr("Reopen last opened feeds on startup"));
  openNewTabNextToActive_ = new QCheckBox(tr("Open new tab next to active"));
  hideFeedsOpenTab_ = new QCheckBox(tr("Hide feeds tree when opening tabs"));
  showToggleFeedsTree_ = new QCheckBox(tr("Show feeds tree toggle"));
  defaultIconFeeds_ = new QCheckBox(tr("Show default rss-icon instead of favourite one"));
  autocollapseFolder_ = new QCheckBox(tr("Automatically collapse folders"));
  showCloseButtonTab_ = new QCheckBox(tr("Show close button on tab"));

  updateCheckEnabled_ = new QCheckBox(tr("Automatically check for updates"));
  statisticsEnabled_ = new QCheckBox(tr("Help improve QuiteRSS by sending usage information"));
  storeDBMemory_ = new QCheckBox(tr("Store a DB in memory (requires program restart)"));
  storeDBMemory_->setChecked(false);
  saveDBMemFileInterval_ = new QSpinBox();
  saveDBMemFileInterval_->setRange(1, 999);

  QHBoxLayout *saveDBMemFileLayout = new QHBoxLayout();
  saveDBMemFileLayout->setContentsMargins(15, 0, 0, 0);
  saveDBMemFileLayout->addWidget(new QLabel(tr("Save DB stored in memory to file every")));
  saveDBMemFileLayout->addWidget(saveDBMemFileInterval_);
  saveDBMemFileLayout->addWidget(new QLabel(tr("minutes")));
  saveDBMemFileLayout->addStretch();

  QWidget *saveDBMemFileWidget = new QWidget();
  saveDBMemFileWidget->setEnabled(false);
  saveDBMemFileWidget->setLayout(saveDBMemFileLayout);

  connect(storeDBMemory_, SIGNAL(toggled(bool)),
          saveDBMemFileWidget, SLOT(setEnabled(bool)));

  QVBoxLayout *generalLayout = new QVBoxLayout();
  generalLayout->addWidget(showSplashScreen_);
  generalLayout->addWidget(reopenFeedStartup_);
  generalLayout->addWidget(openNewTabNextToActive_);
  generalLayout->addWidget(hideFeedsOpenTab_);
  generalLayout->addWidget(showToggleFeedsTree_);
  generalLayout->addWidget(defaultIconFeeds_);
  generalLayout->addWidget(autocollapseFolder_);
  generalLayout->addWidget(showCloseButtonTab_);
  generalLayout->addSpacing(20);

#if defined(Q_OS_WIN)
  autoRunEnabled_ = new QCheckBox(tr("Run QuiteRSS at Windows startup"));
  autoRunSettings_ = new QSettings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                                   QSettings::NativeFormat);
  bool isAutoRun;
  if (mainApp->isPortableAppsCom())
    isAutoRun = autoRunSettings_->value("QuiteRSSPortable", false).toBool();
  else
    isAutoRun = autoRunSettings_->value("QuiteRSS", false).toBool();
  autoRunEnabled_->setChecked(isAutoRun);

  generalLayout->addWidget(autoRunEnabled_);

  registerFeedProtocolButton_ = new QPushButton(
        tr("Set QuiteRSS as the default feed: subscriber"));
  registerFeedProtocolButton_->setToolTip(
        tr("Register QuiteRSS to handle feed:// links from browsers "
           "(e.g. RSSHub-Radar). This writes to the Windows registry "
           "for the current user only, no administrator rights needed."));
  connect(registerFeedProtocolButton_, SIGNAL(clicked()),
          this, SLOT(slotRegisterFeedProtocol()));
  QHBoxLayout *feedProtocolLayout = new QHBoxLayout();
  feedProtocolLayout->setContentsMargins(20, 0, 0, 0);
  feedProtocolLayout->addWidget(registerFeedProtocolButton_);
  feedProtocolLayout->addStretch();
  generalLayout->addLayout(feedProtocolLayout);
#endif

  generalLayout->addWidget(updateCheckEnabled_);
  generalLayout->addWidget(statisticsEnabled_);
  generalLayout->addWidget(storeDBMemory_);
  generalLayout->addWidget(saveDBMemFileWidget);
  generalLayout->addStretch(1);

  generalWidget_ = new QFrame();
  generalWidget_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  generalWidget_->setLayout(generalLayout);
}

/** @brief Create widget "System tray"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createTraySystemWidget()
{
  showTrayIconBox_ = new QGroupBox(tr("Show system tray icon"));
  showTrayIconBox_->setCheckable(true);

  startingTray_ = new QCheckBox(tr("starting QuiteRSS"));
  minimizingTray_ = new QCheckBox(tr("minimizing QuiteRSS"));
  closingTray_ = new QCheckBox(tr("closing QuiteRSS"));
  QVBoxLayout *moveTrayLayout = new QVBoxLayout();
  moveTrayLayout->setContentsMargins(15, 0, 5, 10);
  moveTrayLayout->addWidget(startingTray_);
  moveTrayLayout->addWidget(minimizingTray_);
  moveTrayLayout->addWidget(closingTray_);

  staticIconTray_ = new QRadioButton(tr("Show static icon"));
  changeIconTray_ = new QRadioButton(tr("Change icon for incoming new news"));
  newCountTray_ = new QRadioButton(tr("Show count of new news"));
  unreadCountTray_ = new QRadioButton(tr("Show count of unread news"));
  QVBoxLayout *behaviorLayout = new QVBoxLayout();
  behaviorLayout->setContentsMargins(15, 0, 5, 10);
  behaviorLayout->addWidget(staticIconTray_);
  behaviorLayout->addWidget(changeIconTray_);
  behaviorLayout->addWidget(newCountTray_);
  behaviorLayout->addWidget(unreadCountTray_);

  singleClickTray_ = new QCheckBox(
        tr("Single click instead of double click for show window"));
  clearStatusNew_ = new QCheckBox(tr("Clear new status when minimize to tray"));
  emptyWorking_ = new QCheckBox(tr("Empty working set on minimize to tray"));

  QVBoxLayout *trayLayout = new QVBoxLayout(showTrayIconBox_);
#ifndef Q_OS_MAC
  trayLayout->addWidget(new QLabel(tr("Move to the system tray when:")));
  trayLayout->addLayout(moveTrayLayout);
#endif
  trayLayout->addWidget(new QLabel(tr("Tray icon behavior:")));
  trayLayout->addLayout(behaviorLayout);
#ifndef Q_OS_MAC
  trayLayout->addWidget(singleClickTray_);
  trayLayout->addWidget(clearStatusNew_);
#endif
#if defined(Q_OS_WIN)
  trayLayout->addWidget(emptyWorking_);
#endif
  trayLayout->addStretch(1);

  QVBoxLayout *boxTrayLayout = new QVBoxLayout();
  boxTrayLayout->addWidget(showTrayIconBox_);

  traySystemWidget_ = new QFrame();
  traySystemWidget_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  traySystemWidget_->setLayout(boxTrayLayout);
}

/** @brief Create widget "Network connections"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createNetworkConnectionsWidget()
{
  directConnectionButton_ = new QRadioButton(
        tr("Direct connection to the Internet"));
  systemProxyButton_ = new QRadioButton(
        tr("System proxy configuration (if available)"));
  manualProxyButton_ = new QRadioButton(tr("Manual proxy configuration:"));

  QVBoxLayout *networkConnectionsLayout = new QVBoxLayout();
  networkConnectionsLayout->addWidget(directConnectionButton_);
  networkConnectionsLayout->addWidget(systemProxyButton_);
  networkConnectionsLayout->addWidget(manualProxyButton_);

  typeProxy_ = new QComboBox();
  typeProxy_->addItems(QStringList() << "HTTP" << "SOCKS5");
  editHost_ = new LineEdit();
  editPort_ = new LineEdit();
  editPort_->setFixedWidth(60);
  editUser_ = new LineEdit();
  editPassword_ = new LineEdit();
  editPassword_->setEchoMode(QLineEdit::Password);

  QHBoxLayout *addrPortLayout = new QHBoxLayout();
  addrPortLayout->setMargin(0);
  addrPortLayout->addWidget(typeProxy_);
  addrPortLayout->addWidget(new QLabel(tr("Proxy server:")));
  addrPortLayout->addWidget(editHost_);
  addrPortLayout->addWidget(new QLabel(tr("Port:")));
  addrPortLayout->addWidget(editPort_);

  QWidget *addrPortWidget = new QWidget();
  addrPortWidget->setLayout(addrPortLayout);

  QHBoxLayout *userPasswordLayout = new QHBoxLayout();
  userPasswordLayout->setMargin(0);
  userPasswordLayout->addWidget(new QLabel(tr("Username:")));
  userPasswordLayout->addWidget(editUser_);
  userPasswordLayout->addWidget(new QLabel(tr("Password:")));
  userPasswordLayout->addWidget(editPassword_);

  QWidget *userPasswordWidget = new QWidget();
  userPasswordWidget->setLayout(userPasswordLayout);

  QVBoxLayout *manualLayout = new QVBoxLayout();
  manualLayout->setMargin(0);
  manualLayout->addWidget(addrPortWidget);
  manualLayout->addWidget(userPasswordWidget);
  manualLayout->addStretch();

  manualWidget_ = new QWidget();
  manualWidget_->setEnabled(false);  // cause proper radio-button isn't set on creation
  manualWidget_->setLayout(manualLayout);

  networkConnectionsLayout->addWidget(manualWidget_);
  networkConnectionsLayout->addSpacing(20);

  timeoutRequest_ = new QSpinBox();
  timeoutRequest_->setRange(0, 300);
  numberRequests_ = new QSpinBox();
  numberRequests_->setRange(1, 10);
  numberRepeats_ = new QSpinBox();
  numberRepeats_->setRange(1, 10);

  QGridLayout *requestLayout = new QGridLayout();
  requestLayout->setColumnStretch(1, 1);
  requestLayout->setContentsMargins(15, 0, 5, 0);
  requestLayout->addWidget(new QLabel(tr("Request timeout:")), 0, 0);
  requestLayout->addWidget(timeoutRequest_, 0, 1, 1, 1, Qt::AlignLeft);
  requestLayout->addWidget(new QLabel(tr("Number of requests:")), 1, 0);
  requestLayout->addWidget(numberRequests_, 1, 1, 1, 1, Qt::AlignLeft);
  requestLayout->addWidget(new QLabel(tr("Number of retries:")), 2, 0);
  requestLayout->addWidget(numberRepeats_, 2, 1, 1, 1, Qt::AlignLeft);

  networkConnectionsLayout->addWidget(new QLabel(tr("Options network requests when updating feeds (requires program restart):")));
  networkConnectionsLayout->addLayout(requestLayout);
  networkConnectionsLayout->addStretch(1);

  networkConnectionsWidget_ = new QFrame();
  networkConnectionsWidget_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  networkConnectionsWidget_->setLayout(networkConnectionsLayout);

  connect(manualProxyButton_, SIGNAL(toggled(bool)),
          this, SLOT(manualProxyToggle(bool)));
}

/** @brief Create widget "Browser"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createBrowserWidget()
{
  //! tab "General"
  embeddedBrowserOn_ = new QRadioButton(tr("Use embedded browser"));
  externalBrowserOn_ = new QRadioButton(tr("Use external browser"));
  defaultExternalBrowserOn_ = new QRadioButton(tr("Default external browser"));
  otherExternalBrowserOn_ = new QRadioButton(tr("Following external browser:"));

  otherExternalBrowserEdit_ = new LineEdit();
  otherExternalBrowserButton_ = new QPushButton(tr("Browse..."));

  autoLoadImages_ = new QCheckBox(tr("Load images"));
  javaScriptEnable_ = new QCheckBox(tr("Enable JavaScript"));
  pluginsEnable_ = new QCheckBox(tr("Enable plug-ins"));
  defaultZoomPages_ = new QSpinBox();
  defaultZoomPages_->setMaximum(300);
  defaultZoomPages_->setMinimum(30);
  defaultZoomPages_->setSuffix(" %");

  openLinkInBackgroundEmbedded_ = new QCheckBox(tr("Open links in embedded browser in background"));
  openLinkInBackground_ = new QCheckBox(tr("Open links in external browser in background (experimental)"));

  userStyleBrowserEdit_ = new LineEdit();
  QPushButton *userStyleBrowserButton = new QPushButton(tr("Browse..."));
  connect(userStyleBrowserButton, SIGNAL(clicked()),
          this, SLOT(selectionUserStyleBrowser()));

  QGridLayout *browserSelectionLayout = new QGridLayout();
  browserSelectionLayout->setContentsMargins(15, 0, 5, 10);
  browserSelectionLayout->addWidget(embeddedBrowserOn_, 0, 0);
  browserSelectionLayout->addWidget(externalBrowserOn_, 1, 0);
  QButtonGroup *browserSelectionBox = new QButtonGroup();
  browserSelectionBox->addButton(embeddedBrowserOn_);
  browserSelectionBox->addButton(externalBrowserOn_);

  QGridLayout *externalBrowserLayout = new QGridLayout();
  externalBrowserLayout->setContentsMargins(15, 0, 5, 10);
  externalBrowserLayout->addWidget(defaultExternalBrowserOn_, 0, 0);
  externalBrowserLayout->addWidget(otherExternalBrowserOn_, 1, 0);
  externalBrowserLayout->addWidget(otherExternalBrowserEdit_, 2, 0);
  externalBrowserLayout->addWidget(otherExternalBrowserButton_, 2, 1, Qt::AlignRight);
  QButtonGroup *externalBrowserBox = new QButtonGroup();
  externalBrowserBox->addButton(defaultExternalBrowserOn_);
  externalBrowserBox->addButton(otherExternalBrowserOn_);

  QHBoxLayout *zoomLayout = new QHBoxLayout();
  zoomLayout->addWidget(new QLabel(tr("Default zoom on pages:")));
  zoomLayout->addWidget(defaultZoomPages_);
  zoomLayout->addStretch();

  // S-8: highlight code blocks in articles
  highlightCode_ = new QCheckBox(tr("Highlight code blocks in articles (auto-detect language)"));

  QVBoxLayout *contentBrowserLayout = new QVBoxLayout();
  contentBrowserLayout->setContentsMargins(15, 0, 5, 10);
  contentBrowserLayout->addWidget(autoLoadImages_);
  contentBrowserLayout->addWidget(javaScriptEnable_);
  contentBrowserLayout->addWidget(pluginsEnable_);
  contentBrowserLayout->addLayout(zoomLayout);
  contentBrowserLayout->addWidget(highlightCode_);

  QGridLayout *userStyleBrowserLayout = new QGridLayout();
  userStyleBrowserLayout->setContentsMargins(15, 0, 5, 10);
  userStyleBrowserLayout->addWidget(userStyleBrowserEdit_, 0, 0);
  userStyleBrowserLayout->addWidget(userStyleBrowserButton, 0, 1, Qt::AlignRight);

  QVBoxLayout *browserLayoutV = new QVBoxLayout();
  browserLayoutV->setMargin(10);
  browserLayoutV->addWidget(new QLabel(tr("Browser selection:")));
  browserLayoutV->addLayout(browserSelectionLayout);
  browserLayoutV->addWidget(new QLabel(tr("External browser:")));
  browserLayoutV->addLayout(externalBrowserLayout);
  browserLayoutV->addWidget(new QLabel(tr("Content:")));
  browserLayoutV->addLayout(contentBrowserLayout);
  browserLayoutV->addWidget(new QLabel(tr("User style sheet:")));
  browserLayoutV->addLayout(userStyleBrowserLayout);
  browserLayoutV->addWidget(openLinkInBackgroundEmbedded_);
  browserLayoutV->addWidget(openLinkInBackground_);
  browserLayoutV->addStretch();

  QWidget *generalBrowserWidget = new QWidget();
  generalBrowserWidget->setLayout(browserLayoutV);

  connect(otherExternalBrowserOn_, SIGNAL(toggled(bool)),
          otherExternalBrowserEdit_, SLOT(setEnabled(bool)));
  connect(otherExternalBrowserOn_, SIGNAL(toggled(bool)),
          otherExternalBrowserButton_, SLOT(setEnabled(bool)));
  otherExternalBrowserOn_->setChecked(true);

  connect(otherExternalBrowserButton_, SIGNAL(clicked()),
          this, SLOT(selectionBrowser()));

#if defined(Q_OS_OS2)
  otherExternalBrowserOn_->setVisible(false);
  otherExternalBrowserEdit_->setVisible(false);
  otherExternalBrowserButton_->setVisible(false);
#endif

  //! tab "History"
  maxPagesInCache_ = new QSpinBox();
  maxPagesInCache_->setRange(0, 20);

  QHBoxLayout *historyLayout1 = new QHBoxLayout();
  historyLayout1->addWidget(new QLabel(tr("Maximum pages in cache")));
  historyLayout1->addWidget(maxPagesInCache_);
  historyLayout1->addStretch();

  dirDiskCacheEdit_ = new LineEdit();
  dirDiskCacheButton_ = new QPushButton(tr("Browse..."));

  connect(dirDiskCacheButton_, SIGNAL(clicked()),
          this, SLOT(selectionDirDiskCache()));

  QHBoxLayout *historyLayout2 = new QHBoxLayout();
  historyLayout2->addWidget(new QLabel(tr("Store cache in:")));
  historyLayout2->addWidget(dirDiskCacheEdit_, 1);
  historyLayout2->addWidget(dirDiskCacheButton_);

  maxDiskCache_ = new QSpinBox();
  maxDiskCache_->setRange(10, 300);

  QHBoxLayout *historyLayout3 = new QHBoxLayout();
  historyLayout3->addWidget(new QLabel(tr("Maximum size of disk cache")));
  historyLayout3->addWidget(maxDiskCache_);
  historyLayout3->addWidget(new QLabel(tr("MB")), 1);

  QVBoxLayout *historyLayout4 = new QVBoxLayout();
  historyLayout4->addLayout(historyLayout2);
  historyLayout4->addLayout(historyLayout3);

  diskCacheOn_ = new QGroupBox(tr("Use disk cache"));
  diskCacheOn_->setCheckable(true);
  diskCacheOn_->setChecked(false);
  diskCacheOn_->setLayout(historyLayout4);

  saveCookies_ = new QRadioButton(tr("Allow local data to be set"));
  deleteCookiesOnClose_ = new QRadioButton(tr("Keep local data only until quit application"));
  blockCookies_ = new QRadioButton(tr("Block sites from setting any data"));
  clearCookies_ = new QPushButton(tr("Clear"));
  connect(clearCookies_, SIGNAL(clicked()), mainApp->cookieJar(), SLOT(clearCookies()));

  QGridLayout *cookiesLayout = new QGridLayout();
  cookiesLayout->setContentsMargins(15, 0, 5, 10);
  cookiesLayout->addWidget(saveCookies_, 0, 0);
  cookiesLayout->addWidget(deleteCookiesOnClose_, 1, 0);
  cookiesLayout->addWidget(blockCookies_, 2, 0);
  cookiesLayout->addWidget(clearCookies_, 3, 0, Qt::AlignLeft);
  QButtonGroup *cookiesBox = new QButtonGroup();
  cookiesBox->addButton(saveCookies_);
  cookiesBox->addButton(deleteCookiesOnClose_);
  cookiesBox->addButton(blockCookies_);

  QVBoxLayout *historyMainLayout = new QVBoxLayout();
  historyMainLayout->setMargin(10);
  historyMainLayout->addLayout(historyLayout1);
  historyMainLayout->addWidget(diskCacheOn_);
  historyMainLayout->addSpacing(10);
  historyMainLayout->addWidget(new QLabel(tr("Cookies:")));
  historyMainLayout->addLayout(cookiesLayout);
  historyMainLayout->addStretch();

  QWidget *historyBrowserWidget_ = new QWidget();
  historyBrowserWidget_->setLayout(historyMainLayout);


  //! tab "Click to Flash"
  QLabel *c2fInfo = new QLabel(tr("Click To Flash is a plugin which blocks auto loading of "
                                 "Flash content at page. You can always load it manually "
                                 "by clicking on the Flash play icon."));
  c2fInfo->setWordWrap(true);

  c2fEnabled_ = new QCheckBox(tr("Use Click to Flash"));
  c2fEnabled_->setChecked(false);

  c2fWhitelist_ = new QTreeWidget(this);
  c2fWhitelist_->setObjectName("c2fWhiteList_");
  c2fWhitelist_->setRootIsDecorated(false);
  c2fWhitelist_->setColumnCount(1);

  QStringList treeItem;
  treeItem << "Whitelist";
  c2fWhitelist_->setHeaderLabels(treeItem);

  QPushButton *addButton = new QPushButton(tr("Add..."), this);
  connect(addButton, SIGNAL(clicked()), this, SLOT(addWhitelist()));
  QPushButton *removeButton = new QPushButton(tr("Remove..."), this);
  connect(removeButton, SIGNAL(clicked()), this, SLOT(removeWhitelist()));

  QVBoxLayout *click2FlashLayout1 = new QVBoxLayout();
  click2FlashLayout1->addWidget(addButton);
  click2FlashLayout1->addWidget(removeButton);
  click2FlashLayout1->addStretch(1);

  QHBoxLayout *click2FlashLayout2 = new QHBoxLayout();
  click2FlashLayout2->setMargin(0);
  click2FlashLayout2->addWidget(c2fWhitelist_, 1);
  click2FlashLayout2->addLayout(click2FlashLayout1);

  QWidget *c2fWhitelistWidget = new QWidget(this);
  c2fWhitelistWidget->setLayout(click2FlashLayout2);
  c2fWhitelistWidget->setEnabled(false);

  connect(c2fEnabled_, SIGNAL(toggled(bool)),
          c2fWhitelistWidget, SLOT(setEnabled(bool)));

  QVBoxLayout *click2FlashLayout = new QVBoxLayout();
  click2FlashLayout->setMargin(10);
  click2FlashLayout->addWidget(c2fInfo);
  click2FlashLayout->addWidget(c2fEnabled_);
  click2FlashLayout->addWidget(c2fWhitelistWidget, 1);

  QWidget *click2FlashWidget_ = new QWidget(this);
  click2FlashWidget_->setLayout(click2FlashLayout);

  c2fEnabled_->setChecked(mainApp->c2fIsEnabled());
  foreach(const QString & site, mainApp->c2fGetWhitelist()) {
    QTreeWidgetItem* item = new QTreeWidgetItem(c2fWhitelist_);
    item->setText(0, site);
  }

  //! tab "Downloads"
  downloadLocationEdit_ = new LineEdit();
  QPushButton *downloadLocationButton = new QPushButton(tr("Browse..."));
  connect(downloadLocationButton, SIGNAL(clicked()),
          this, SLOT(selectionDownloadLocation()));

  askDownloadLocation_ = new QCheckBox(tr("Ask where to save each file before downloading"));

  QGridLayout *downLocationLayout = new QGridLayout();
  downLocationLayout->setContentsMargins(15, 0, 5, 10);
  downLocationLayout->addWidget(downloadLocationEdit_, 0, 0);
  downLocationLayout->addWidget(downloadLocationButton, 0, 1, Qt::AlignRight);
  downLocationLayout->addWidget(askDownloadLocation_, 1, 0);

  QVBoxLayout *downloadsLayout = new QVBoxLayout();
  downloadsLayout->setMargin(10);
  downloadsLayout->addWidget(new QLabel(tr("Download location:")));
  downloadsLayout->addLayout(downLocationLayout);
  downloadsLayout->addStretch();

  QWidget *downloadsWidget = new QWidget(this);
  downloadsWidget->setLayout(downloadsLayout);

  browserWidget_ = new QTabWidget();
  browserWidget_->addTab(generalBrowserWidget, tr("General"));
  browserWidget_->addTab(historyBrowserWidget_, tr("History"));
#if QT_VERSION < 0x050900
  browserWidget_->addTab(click2FlashWidget_, tr("Click to Flash"));
#endif
  browserWidget_->addTab(downloadsWidget, tr("Downloads"));
}

/** @brief Create windet "Feeds"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createFeedsWidget()
{
//! tab "General"
  updateFeedsStartUp_ = new QCheckBox(
        tr("Automatically update the feeds on startup"));
  updateFeedsEnable_ = new QCheckBox(tr("Automatically update the feeds every"));
  updateFeedsInterval_ = new QSpinBox();
  updateFeedsInterval_->setEnabled(false);
  updateFeedsInterval_->setRange(1, 9999);
  connect(updateFeedsEnable_, SIGNAL(toggled(bool)),
          updateFeedsInterval_, SLOT(setEnabled(bool)));

  updateIntervalType_ = new QComboBox(this);
  updateIntervalType_->setEnabled(false);
  QStringList intervalList;
  intervalList << tr("seconds") << tr("minutes")  << tr("hours");
  updateIntervalType_->addItems(intervalList);
  connect(updateFeedsEnable_, SIGNAL(toggled(bool)),
          updateIntervalType_, SLOT(setEnabled(bool)));

  QHBoxLayout *updateFeedsLayout = new QHBoxLayout();
  updateFeedsLayout->setMargin(0);
  updateFeedsLayout->addWidget(updateFeedsEnable_);
  updateFeedsLayout->addWidget(updateFeedsInterval_);
  updateFeedsLayout->addWidget(updateIntervalType_);
  updateFeedsLayout->addStretch();

  positionLastNews_ = new QRadioButton(tr("Set focus on the last opened news"));
  positionFirstNews_ = new QRadioButton(tr("Set focus at the top of news list"));
  positionUnreadNews_ = new QRadioButton(tr("Set focus on the unread news"));
  openNewsWebViewOn_ = new QCheckBox(tr("Open the news"));
  nottoOpenNews_ = new QRadioButton(tr("Nothing to do"));
  connect(nottoOpenNews_, SIGNAL(toggled(bool)),
          openNewsWebViewOn_, SLOT(setDisabled(bool)));

  QGridLayout *openingFeedsLayout = new QGridLayout();
  openingFeedsLayout->setContentsMargins(15, 0, 0, 10);
  openingFeedsLayout->setColumnStretch(1, 1);
  openingFeedsLayout->addWidget(positionLastNews_, 0, 0, 1, 1);
  openingFeedsLayout->addWidget(openNewsWebViewOn_, 0, 1, 1, 1);
  openingFeedsLayout->addWidget(positionFirstNews_, 1, 0, 1, 1);
  openingFeedsLayout->addWidget(positionUnreadNews_, 2, 0, 1, 1);
  openingFeedsLayout->addWidget(nottoOpenNews_, 3, 0, 1, 1);

  markIdenticalNewsRead_ = new QCheckBox(tr("Automatically mark identical news as read"));

  avoidedOldNewsDate_ = new QCalendarWidget();
  avoidedOldNewsDate_->setSelectedDate(QDate::currentDate());
  avoidedOldNewsDate_->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
  avoidedOldNewsDate_->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
  QHBoxLayout *avoidedOldNewsDateLayout = new QHBoxLayout();
  avoidedOldNewsDateLayout->setMargin(5);
  avoidedOldNewsDateLayout->addWidget(avoidedOldNewsDate_);
  avoidedOldNewsDateLayout->addStretch();

  avoidedOldNewsDateOn_ = new QGroupBox(tr("Avoid adding news before this date into the database:"));
  avoidedOldNewsDateOn_->setCheckable(true);
  avoidedOldNewsDateOn_->setChecked(false);
  avoidedOldNewsDateOn_->setLayout(avoidedOldNewsDateLayout);

  QVBoxLayout *generalFeedsLayout = new QVBoxLayout();
  generalFeedsLayout->addWidget(updateFeedsStartUp_);
  generalFeedsLayout->addLayout(updateFeedsLayout);
  generalFeedsLayout->addSpacing(10);
  generalFeedsLayout->addWidget(new QLabel(tr("Action on feed opening:")));
  generalFeedsLayout->addLayout(openingFeedsLayout);
  generalFeedsLayout->addWidget(markIdenticalNewsRead_);
  generalFeedsLayout->addSpacing(15);
  generalFeedsLayout->addWidget(avoidedOldNewsDateOn_);
  generalFeedsLayout->addStretch();

  QWidget *generalFeedsWidget = new QWidget();
  generalFeedsWidget->setLayout(generalFeedsLayout);


  //! tab "Display"
  QStringList itemList;
  itemList << "31.12.99" << "31.12.1999"
           << QString("31. %1. 1999").arg(tr("Dec"))
           << QString("31. %1 1999").arg(tr("December"))
           << "99-12-31" << "1999-12-31" << "12/31/1999";
  formatDate_ = new QComboBox(this);
  formatDate_->addItems(itemList);
  formatDate_->setItemData(0, "dd.MM.yy");
  formatDate_->setItemData(1, "dd.MM.yyyy");
  formatDate_->setItemData(2, "dd. MMM. yyyy");
  formatDate_->setItemData(3, "dd. MMMM yyyy");
  formatDate_->setItemData(4, "yy-MM-dd");
  formatDate_->setItemData(5, "yyyy-MM-dd");
  formatDate_->setItemData(6, "MM/dd/yyyy");

  itemList.clear();
  itemList << "13:37"  << "13:37:09" << "01:37 PM"<< "01:37:09 PM";
  formatTime_ = new QComboBox(this);
  formatTime_->addItems(itemList);
  formatTime_->setItemData(0, "hh:mm");
  formatTime_->setItemData(1, "hh:mm:ss");
  formatTime_->setItemData(2, "hh:mm AP");
  formatTime_->setItemData(3, "hh:mm:ss AP");

  QHBoxLayout *formatDateLayout = new QHBoxLayout();
  formatDateLayout->setMargin(0);
  formatDateLayout->addWidget(new QLabel(tr("Display format for date:")));
  formatDateLayout->addWidget(formatDate_);
  formatDateLayout->addSpacing(10);
  formatDateLayout->addWidget(new QLabel(tr("time:")));
  formatDateLayout->addWidget(formatTime_);
  formatDateLayout->addStretch();

  alternatingRowColorsNews_ = new QCheckBox(tr("Alternating row background colors"));
  simplifiedDateTime_ = new QCheckBox(tr("Simplified representation of date and time"));

  itemList.clear();
  itemList << tr("Show All")  << tr("Show New") << tr("Show Unread")
           << tr("Show Starred") << tr("Show Not Starred")
           << tr("Show Unread or Starred") << tr("Show Last Day")
           << tr("Show Last 7 Days");
  mainNewsFilter_ = new QComboBox(this);
  mainNewsFilter_->addItems(itemList);
  mainNewsFilter_->setItemData(0, "filterNewsAll_");
  mainNewsFilter_->setItemData(1, "filterNewsNew_");
  mainNewsFilter_->setItemData(2, "filterNewsUnread_");
  mainNewsFilter_->setItemData(3, "filterNewsStar_");
  mainNewsFilter_->setItemData(4, "filterNewsNotStarred_");
  mainNewsFilter_->setItemData(5, "filterNewsUnreadStar_");
  mainNewsFilter_->setItemData(6, "filterNewsLastDay_");
  mainNewsFilter_->setItemData(7, "filterNewsLastWeek_");

  QHBoxLayout *mainNewsFilterLayout = new QHBoxLayout();
  mainNewsFilterLayout->setMargin(0);
  mainNewsFilterLayout->addWidget(new QLabel(tr("Default news filter:")));
  mainNewsFilterLayout->addWidget(mainNewsFilter_);
  mainNewsFilterLayout->addStretch();

  styleSheetNewsEdit_ = new LineEdit();
  QPushButton *styleSheetNewsButton = new QPushButton(tr("Browse..."));
  connect(styleSheetNewsButton, SIGNAL(clicked()),
          this, SLOT(selectionUserStyleNews()));
  QPushButton *styleSheetNewsResetButton = new QPushButton(tr("Reset"));
  connect(styleSheetNewsResetButton, SIGNAL(clicked()),
          this, SLOT(resetUserStyleNews()));

  QGridLayout *styleSheetNewsLayout = new QGridLayout();
  styleSheetNewsLayout->setContentsMargins(15, 0, 5, 10);
  styleSheetNewsLayout->addWidget(styleSheetNewsEdit_, 0, 0);
  styleSheetNewsLayout->addWidget(styleSheetNewsButton, 0, 1, Qt::AlignRight);
  styleSheetNewsLayout->addWidget(styleSheetNewsResetButton, 0, 2, Qt::AlignRight);

  showDescriptionNews_ = new QCheckBox(tr("Show news description instead of loading web page"));

  // S-7: custom date format
  formatDate_->addItem(tr("Custom..."), "custom");
  customDateFormat_ = new QLineEdit();
  customDateFormat_->setPlaceholderText(tr("e.g. yyyy-MM-dd HH:mm"));
  customDateFormat_->setEnabled(false);
  connect(formatDate_, SIGNAL(currentIndexChanged(int)),
          this, SLOT(slotCustomDateFormatChanged(int)));
  QHBoxLayout *customDateLayout = new QHBoxLayout();
  customDateLayout->setMargin(0);
  customDateLayout->addWidget(customDateFormat_);
  customDateLayout->addStretch();

  // S-1: dim read news in the list
  dimRead_ = new QCheckBox(tr("Dim read news in the list"));

  // S-2: group news by date
  groupByDate_ = new QCheckBox(tr("Group news by date (Today / Yesterday / Earlier)"));

  // S-9: wide reader mode
  wideMode_ = new QCheckBox(tr("Wide reader mode"));

  // S-6: reader font size / line height
  QHBoxLayout *readerFontLayout = new QHBoxLayout();
  readerFontLayout->setMargin(0);
  readerFontLayout->addWidget(new QLabel(tr("Reader font size:")));
  readerFontSize_ = new QSpinBox();
  readerFontSize_->setRange(0, 40);
  readerFontSize_->setSuffix(" pt");
  readerFontSize_->setSpecialValueText(tr("Default"));
  readerFontLayout->addWidget(readerFontSize_);
  readerFontLayout->addSpacing(10);
  readerFontLayout->addWidget(new QLabel(tr("Line height:")));
  readerLineHeight_ = new QSpinBox();
  readerLineHeight_->setRange(0, 200);
  readerLineHeight_->setSuffix(" %");
  readerLineHeight_->setSpecialValueText(tr("Default"));
  readerFontLayout->addWidget(readerLineHeight_);
  readerFontLayout->addStretch();

  QVBoxLayout *displayFeedsLayout = new QVBoxLayout();
  displayFeedsLayout->addWidget(alternatingRowColorsNews_);
  displayFeedsLayout->addSpacing(10);
  displayFeedsLayout->addWidget(simplifiedDateTime_);
  displayFeedsLayout->addLayout(formatDateLayout);
  displayFeedsLayout->addLayout(customDateLayout);
  displayFeedsLayout->addSpacing(10);
  displayFeedsLayout->addLayout(mainNewsFilterLayout);
  displayFeedsLayout->addSpacing(10);
  displayFeedsLayout->addWidget(dimRead_);
  displayFeedsLayout->addWidget(groupByDate_);
  displayFeedsLayout->addWidget(wideMode_);
  displayFeedsLayout->addLayout(readerFontLayout);
  displayFeedsLayout->addSpacing(10);
  displayFeedsLayout->addWidget(new QLabel(tr("Style sheet for news:")));
  displayFeedsLayout->addLayout(styleSheetNewsLayout);
  displayFeedsLayout->addWidget(showDescriptionNews_);
  displayFeedsLayout->addStretch();

  QWidget *displayFeedsWidget = new QWidget();
  displayFeedsWidget->setLayout(displayFeedsLayout);


  //! tab "Reading"
  QVBoxLayout* readingMainLayout = new QVBoxLayout();

  {
    markNewsReadOn_ = new QGroupBox(tr("Mark news as read:"));

    markNewsReadOn_->setCheckable(true);

    {
      QVBoxLayout* radioLayout = new QVBoxLayout();

      {
        QHBoxLayout* curLayout = new QHBoxLayout();

        markCurNewsRead_ = new QRadioButton(tr("on selecting. With timeout"));

        markNewsReadTime_ = new QSpinBox();
        markNewsReadTime_->setEnabled(false);
        markNewsReadTime_->setRange(0, 100);
        connect(markCurNewsRead_, SIGNAL(toggled(bool)),
          markNewsReadTime_, SLOT(setEnabled(bool)));

        curLayout->setMargin(0);
        curLayout->addWidget(markCurNewsRead_);
        curLayout->addWidget(markNewsReadTime_);
        curLayout->addWidget(new QLabel(tr("seconds")));
        curLayout->addStretch();

        radioLayout->addLayout(curLayout);
      }

      markPrevNewsRead_ = new QRadioButton(tr("after switching to another news"));

      radioLayout->addWidget(markPrevNewsRead_);
      markNewsReadOn_->setLayout(radioLayout);
    }

    markReadSwitchingFeed_ = new QCheckBox(tr("Mark displayed news as read when switching feeds"));
    markReadClosingTab_ = new QCheckBox(tr("Mark displayed news as read when closing tab"));
    markReadMinimize_ = new QCheckBox(tr("Mark displayed news as read on minimize"));

    QGridLayout* curLayout = new QGridLayout();

    notDeleteStarred_ = new QCheckBox(tr("starred news"));
    notDeleteLabeled_ = new QCheckBox(tr("labeled news"));

    curLayout->setContentsMargins(15, 0, 0, 10);
    curLayout->addWidget(notDeleteStarred_, 0, 0, 1, 1);
    curLayout->addWidget(notDeleteLabeled_, 1, 0, 1, 1);

    changeBehaviorActionNUN_ = new QCheckBox(tr("Change behavior of action 'Next Unread News'"));

    readingMainLayout->addWidget(markNewsReadOn_);
    readingMainLayout->addWidget(markReadSwitchingFeed_);
    readingMainLayout->addWidget(markReadClosingTab_);
    readingMainLayout->addWidget(markReadMinimize_);
    readingMainLayout->addSpacing(10);
    readingMainLayout->addWidget(new QLabel(tr("Prevent accidental deletion of:")));
    readingMainLayout->addLayout(curLayout);
    readingMainLayout->addWidget(changeBehaviorActionNUN_);
    readingMainLayout->addStretch();
  }

  QWidget* readingFeedsWidget = new QWidget();
  readingFeedsWidget->setLayout(readingMainLayout);


//! tab "Clean Up"
  QWidget *cleanUpFeedsWidget = new QWidget();

  cleanupOnShutdownBox_ = new QGroupBox(tr("Enable cleanup on shutdown"));
  cleanupOnShutdownBox_->setCheckable(true);

  dayCleanUpOn_ = new QCheckBox(tr("Maximum age of news in days to keep:"));
  maxDayCleanUp_ = new QSpinBox();
  maxDayCleanUp_->setEnabled(false);
  maxDayCleanUp_->setRange(0, 9999);
  connect(dayCleanUpOn_, SIGNAL(toggled(bool)),
          maxDayCleanUp_, SLOT(setEnabled(bool)));

  newsCleanUpOn_ = new QCheckBox(tr("Maximum number of news to keep:"));
  maxNewsCleanUp_ = new QSpinBox();
  maxNewsCleanUp_->setEnabled(false);
  maxNewsCleanUp_->setRange(0, 9999);
  connect(newsCleanUpOn_, SIGNAL(toggled(bool)),
          maxNewsCleanUp_, SLOT(setEnabled(bool)));

  readCleanUp_ = new QCheckBox(tr("Delete read news"));
  neverUnreadCleanUp_ = new QCheckBox(tr("Never delete unread news"));
  neverStarCleanUp_ = new QCheckBox(tr("Never delete starred news"));
  neverLabelCleanUp_ = new QCheckBox(tr("Never delete labeled news"));

  cleanUpDeleted_ = new QCheckBox(tr("Clean up 'Deleted'"));
  optimizeDB_ = new QCheckBox(tr("Enable DB optimization (slower shutdown)"));

  QGridLayout *cleanUpFeedsLayout = new QGridLayout();
  cleanUpFeedsLayout->setColumnStretch(1, 1);
  cleanUpFeedsLayout->addWidget(dayCleanUpOn_, 0, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(maxDayCleanUp_, 0, 1, 1, 1, Qt::AlignLeft);
  cleanUpFeedsLayout->addWidget(newsCleanUpOn_, 1, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(maxNewsCleanUp_, 1, 1, 1, 1, Qt::AlignLeft);
  cleanUpFeedsLayout->addWidget(readCleanUp_, 2, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(neverUnreadCleanUp_, 3, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(neverStarCleanUp_, 4, 0, 1, 1);
  cleanUpFeedsLayout->addWidget(neverLabelCleanUp_, 5, 0, 1, 1);

  QVBoxLayout *cleanUpFeedsLayout2 = new QVBoxLayout();
  cleanUpFeedsLayout2->addWidget(cleanUpDeleted_);
  cleanUpFeedsLayout2->addWidget(optimizeDB_);

  QVBoxLayout *cleanUpFeedsLayout3 = new QVBoxLayout(cleanupOnShutdownBox_);
  cleanUpFeedsLayout3->addLayout(cleanUpFeedsLayout);
  cleanUpFeedsLayout3->addSpacing(10);
  cleanUpFeedsLayout3->addLayout(cleanUpFeedsLayout2);
  cleanUpFeedsLayout3->addStretch();

  QVBoxLayout *boxCleanUpFeedsLayout = new QVBoxLayout(cleanUpFeedsWidget);
  boxCleanUpFeedsLayout->addWidget(cleanupOnShutdownBox_);

  feedsWidget_ = new QTabWidget();
  feedsWidget_->addTab(generalFeedsWidget, tr("General"));
  feedsWidget_->addTab(displayFeedsWidget, tr("Display"));
  feedsWidget_->addTab(readingFeedsWidget, tr("Reading"));
  feedsWidget_->addTab(cleanUpFeedsWidget, tr("Clean Up"));
}

/** @brief Create widget "Labels"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createLabelsWidget()
{
  labelsTree_ = new QTreeWidget(this);
  labelsTree_->setObjectName("labelsTree_");
  labelsTree_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  labelsTree_->setColumnCount(5);
  labelsTree_->setColumnHidden(0, true);
  labelsTree_->setColumnHidden(2, true);
  labelsTree_->setColumnHidden(3, true);
  labelsTree_->setColumnHidden(4, true);
  labelsTree_->header()->hide();

  newLabelButton_ = new QPushButton(tr("New..."), this);
  connect(newLabelButton_, SIGNAL(clicked()), this, SLOT(newLabel()));
  editLabelButton_ = new QPushButton(tr("Edit..."), this);
  editLabelButton_->setEnabled(false);
  connect(editLabelButton_, SIGNAL(clicked()), this, SLOT(editLabel()));
  deleteLabelButton_ = new QPushButton(tr("Delete..."), this);
  deleteLabelButton_->setEnabled(false);
  connect(deleteLabelButton_, SIGNAL(clicked()), this, SLOT(deleteLabel()));

  moveUpLabelButton_ = new QPushButton(tr("Move up"), this);
  moveUpLabelButton_->setEnabled(false);
  connect(moveUpLabelButton_, SIGNAL(clicked()), this, SLOT(moveUpLabel()));
  moveDownLabelButton_ = new QPushButton(tr("Move down"), this);
  moveDownLabelButton_->setEnabled(false);
  connect(moveDownLabelButton_, SIGNAL(clicked()), this, SLOT(moveDownLabel()));

  QVBoxLayout *buttonsLayout = new QVBoxLayout();
  buttonsLayout->addWidget(newLabelButton_);
  buttonsLayout->addWidget(editLabelButton_);
  buttonsLayout->addWidget(deleteLabelButton_);
  buttonsLayout->addSpacing(10);
  buttonsLayout->addWidget(moveUpLabelButton_);
  buttonsLayout->addWidget(moveDownLabelButton_);
  buttonsLayout->addStretch();

  QHBoxLayout *labelsLayout = new QHBoxLayout();
  labelsLayout->setMargin(0);
  labelsLayout->addWidget(labelsTree_);
  labelsLayout->addLayout(buttonsLayout);

  labelsWidget_ = new QWidget(this);
  labelsWidget_->setLayout(labelsLayout);

  connect(labelsTree_, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(slotCurrentLabelChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(labelsTree_, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(editLabel()));

  loadLabelsOk_ = false;
}

/** @brief Create widget "Notifier"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createNotifierWidget()
{
  showNotifyOn_ = new QGroupBox(tr("Display notification for incoming news"));
  showNotifyOn_->setCheckable(true);
  showNotifyOn_->setChecked(false);

  screenNotify_ = new QComboBox();
  screenNotify_->addItem("-1");
  for (int i = 0; i < Common::desktopScreenCount(); ++i) {
    screenNotify_->addItem(QString::number(i));
  }
  screenNotify_->setCurrentIndex(1);

  positionNotify_ = new QComboBox();
  QStringList positionList;
  positionList << tr("Top Left") << tr("Top Right")
               << tr("Bottom Left") << tr("Bottom Right");
  positionNotify_->addItems(positionList);

  fullscreenModeNotify_ = new QCheckBox(tr("Do not show notification in fullscreen mode"));
#if !defined(Q_OS_WIN)
  fullscreenModeNotify_->hide();
#endif
  showNotifyInactiveApp_ = new QCheckBox(tr("Show when inactive main window"));
  closeNotify_ = new QCheckBox(tr("Close notification after opening news"));

  transparencyNotify_ = new QSpinBox();
  transparencyNotify_->setRange(0, 100);
  countShowNewsNotify_ = new QSpinBox();
  countShowNewsNotify_->setRange(1, 30);
  widthTitleNewsNotify_ = new QSpinBox();
  widthTitleNewsNotify_->setRange(50, 500);
  timeShowNewsNotify_ = new QSpinBox();
  timeShowNewsNotify_->setRange(0, 99999);

  QPushButton *showNotifer = new QPushButton(tr("Review"));
  connect(showNotifer, SIGNAL(clicked()), this, SLOT(showNotification()));

  QHBoxLayout *notifierLayout1 = new QHBoxLayout();
  notifierLayout1->addWidget(new QLabel(tr("Screen")));
  notifierLayout1->addWidget(screenNotify_);
  notifierLayout1->addSpacing(10);
  notifierLayout1->addWidget(new QLabel(tr("Position")));
  notifierLayout1->addWidget(positionNotify_);
  notifierLayout1->addStretch(1);
  notifierLayout1->addWidget(showNotifer);

  QGridLayout *notifierLayout2 = new QGridLayout();
  notifierLayout2->setColumnStretch(2, 1);
  notifierLayout2->addWidget(new QLabel(tr("Transparency")), 0, 0);
  notifierLayout2->addWidget(transparencyNotify_, 0, 1);
  notifierLayout2->addWidget(new QLabel("%"), 0, 2);
  notifierLayout2->addWidget(new QLabel(tr("Show maximum of")), 1, 0);
  notifierLayout2->addWidget(countShowNewsNotify_, 1, 1);
  notifierLayout2->addWidget(new QLabel(tr("item on page notification")), 1, 2);
  notifierLayout2->addWidget(new QLabel(tr("Width list items")), 2, 0);
  notifierLayout2->addWidget(widthTitleNewsNotify_, 2, 1);
  notifierLayout2->addWidget(new QLabel(tr("pixels")), 2, 2);
  notifierLayout2->addWidget(new QLabel(tr("Close notification after")), 3, 0);
  notifierLayout2->addWidget(timeShowNewsNotify_, 3, 1);
  notifierLayout2->addWidget(new QLabel(tr("seconds")), 3, 2);


  showTitlesFeedsNotify_ = new QCheckBox(tr("Show titles feeds"));
  showIconFeedNotify_ = new QCheckBox(tr("Show icon feed"));
  showButtonMarkAllNotify_ = new QCheckBox(tr("Show button 'Mark All News Read'"));
  showButtonMarkReadNotify_ = new QCheckBox(tr("Show button 'Mark Read/Unread'"));
  showButtonExBrowserNotify_ = new QCheckBox(tr("Show button 'Open in External Browser'"));
  showButtonDeleteNotify_ = new QCheckBox(tr("Show button 'Delete News'"));

  QVBoxLayout *notifierLayout3 = new QVBoxLayout();
  notifierLayout3->addWidget(showTitlesFeedsNotify_);
  notifierLayout3->addWidget(showIconFeedNotify_);
  notifierLayout3->addWidget(showButtonMarkAllNotify_);
  notifierLayout3->addWidget(showButtonMarkReadNotify_);
  notifierLayout3->addWidget(showButtonExBrowserNotify_);
  notifierLayout3->addWidget(showButtonDeleteNotify_);

  onlySelectedFeeds_ = new QCheckBox(tr("Only show selected feeds:"));
  QPushButton *feedsNotiferButton = new QPushButton(tr("Feeds"));
  feedsNotiferButton->setEnabled(false);

  QHBoxLayout *onlySelectedFeedsLayout = new QHBoxLayout();
  onlySelectedFeedsLayout->addWidget(onlySelectedFeeds_);
  onlySelectedFeedsLayout->addWidget(feedsNotiferButton);
  onlySelectedFeedsLayout->addStretch(1);

  createFeedsNotifierDlg();

  connect(onlySelectedFeeds_, SIGNAL(toggled(bool)),
          feedsNotiferButton, SLOT(setEnabled(bool)));
  connect(feedsNotiferButton, SIGNAL(clicked()),
          feedsNotifierDlg_, SLOT(exec()));

  // UI-6: quiet hours (do not disturb) window
  quietHoursNotify_ = new QCheckBox(tr("Quiet hours (do not disturb)"));
  quietHoursStart_ = new QTimeEdit();
  quietHoursStart_->setDisplayFormat("HH:mm");
  quietHoursStart_->setTime(QTime(22, 0));
  quietHoursEnd_ = new QTimeEdit();
  quietHoursEnd_->setDisplayFormat("HH:mm");
  quietHoursEnd_->setTime(QTime(7, 0));

  QHBoxLayout *quietHoursLayout = new QHBoxLayout();
  quietHoursLayout->addWidget(quietHoursNotify_);
  quietHoursLayout->addSpacing(10);
  quietHoursLayout->addWidget(new QLabel(tr("From")));
  quietHoursLayout->addWidget(quietHoursStart_);
  quietHoursLayout->addWidget(new QLabel(tr("To")));
  quietHoursLayout->addWidget(quietHoursEnd_);
  quietHoursLayout->addStretch(1);

  QVBoxLayout *notificationLayoutV = new QVBoxLayout();
  notificationLayoutV->setMargin(10);
  notificationLayoutV->addLayout(notifierLayout1);
  notificationLayoutV->addWidget(fullscreenModeNotify_);
  notificationLayoutV->addWidget(showNotifyInactiveApp_);
  notificationLayoutV->addWidget(closeNotify_);
  notificationLayoutV->addSpacing(10);
  notificationLayoutV->addLayout(notifierLayout2);
  notificationLayoutV->addSpacing(10);
  notificationLayoutV->addLayout(notifierLayout3);
  notificationLayoutV->addSpacing(10);
  notificationLayoutV->addLayout(onlySelectedFeedsLayout);
  notificationLayoutV->addSpacing(10);
  notificationLayoutV->addLayout(quietHoursLayout);
  notificationLayoutV->addStretch(1);

  showNotifyOn_->setLayout(notificationLayoutV);

  QWidget *notificationWidget = new QWidget();
  QVBoxLayout *notificationLayout = new QVBoxLayout(notificationWidget);
  notificationLayout->addWidget(showNotifyOn_);


  soundNotifyBox_ = new QGroupBox(tr("Play sound for incoming new news"));
  soundNotifyBox_->setCheckable(true);
  soundNotifyBox_->setChecked(false);

  editSoundNotifer_ = new LineEdit();
  selectionSoundNotifer_ = new QPushButton(tr("Browse..."));
  playSoundNotifer_ = new QPushButton(tr("Play"));

  connect(selectionSoundNotifer_, SIGNAL(clicked()),
          this, SLOT(selectionSoundNotifer()));
  connect(playSoundNotifer_, SIGNAL(clicked()), this, SLOT(slotPlaySoundNotifer()));
  connect(this, SIGNAL(signalPlaySound(QString)), parent(), SLOT(slotPlaySound(QString)));

  QHBoxLayout *soundNotifyLayoutH = new QHBoxLayout();
  soundNotifyLayoutH->addWidget(editSoundNotifer_, 1);
  soundNotifyLayoutH->addWidget(selectionSoundNotifer_);
  soundNotifyLayoutH->addWidget(playSoundNotifer_);

  QVBoxLayout *soundNotifyLayout = new QVBoxLayout(soundNotifyBox_);
  soundNotifyLayout->setMargin(10);
  soundNotifyLayout->addLayout(soundNotifyLayoutH);
  soundNotifyLayout->addStretch(1);

  QWidget *soundNotifyWidget = new QWidget();
  QVBoxLayout *boxCleanUpFeedsLayout = new QVBoxLayout(soundNotifyWidget);
  boxCleanUpFeedsLayout->addWidget(soundNotifyBox_);


  notifierWidget_ = new QTabWidget();
  notifierWidget_->addTab(notificationWidget, tr("Notification"));
  notifierWidget_->addTab(soundNotifyWidget, tr("Sound"));

  loadNotifierOk_ = false;
}

/** @brief Create dialog "Feeds" for Notifier
 *----------------------------------------------------------------------------*/
void OptionsDialog::createFeedsNotifierDlg()
{
  feedsNotifierDlg_ = new Dialog(this);
  feedsNotifierDlg_->setWindowFlags (windowFlags() & ~Qt::WindowContextHelpButtonHint);
  feedsNotifierDlg_->setWindowTitle(tr("Selection of feeds"));
  feedsNotifierDlg_->setMinimumSize(300, 300);

  feedsTreeNotify_ = new QTreeWidget(this);
  feedsTreeNotify_->setObjectName("feedsTreeNotify_");
  feedsTreeNotify_->setColumnCount(2);
  feedsTreeNotify_->setColumnHidden(1, true);
  feedsTreeNotify_->header()->hide();

  itemNotChecked_ = false;

  QStringList treeItem;
  treeItem << "Feeds" << "Id";
  feedsTreeNotify_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << tr("All Feeds") << "0";
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);
  treeWidgetItem->setCheckState(0, Qt::Checked);
  feedsTreeNotify_->addTopLevelItem(treeWidgetItem);

  feedsNotifierDlg_->pageLayout->addWidget(feedsTreeNotify_);
  feedsNotifierDlg_->buttonBox->addButton(QDialogButtonBox::Close);

  connect(feedsTreeNotify_, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
          this, SLOT(feedsTreeNotifyItemChanged(QTreeWidgetItem*,int)));
}

/** @brief Create widget "Passwords"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createPasswordsWidget()
{
  passTree_ = new QTreeWidget(this);
  passTree_->setObjectName("labelsTree_");
  passTree_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  passTree_->setColumnCount(4);
  passTree_->setColumnHidden(0, true);
  passTree_->setColumnHidden(3, true);
  passTree_->setColumnWidth(1, 250);
  passTree_->header()->setMinimumSectionSize(22);

  QStringList strTreeItem;
  strTreeItem.clear();
  strTreeItem << "Id" << tr("Site") << tr("User") << tr("Password");
  passTree_->setHeaderLabels(strTreeItem);

  QSqlQuery q;
  q.exec("SELECT id, server, username, password FROM passwords");
  while (q.next()) {
    QString id = q.value(0).toString();
    QString server = q.value(1).toString();
    QString user = q.value(2).toString();
    QString pass = QString::fromUtf8(QByteArray::fromBase64(q.value(3).toByteArray()));

    strTreeItem.clear();
    strTreeItem << id << server << user << pass;
    QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(strTreeItem);
    passTree_->addTopLevelItem(treeWidgetItem);
  }

  QPushButton *deletePass = new QPushButton(tr("Delete"));
  connect(deletePass, SIGNAL(clicked()), this, SLOT(slotDeletePass()));
  QPushButton *deleteAllPass = new QPushButton(tr("Delete All"));
  connect(deleteAllPass, SIGNAL(clicked()), this, SLOT(slotDeleteAllPass()));
  QPushButton *showPass = new QPushButton(tr("Show Passwords"));
  connect(showPass, SIGNAL(clicked()), this, SLOT(slotShowPass()));
  QVBoxLayout *passButtonLayout = new QVBoxLayout();
  passButtonLayout->addWidget(deletePass);
  passButtonLayout->addWidget(deleteAllPass);
  passButtonLayout->addSpacing(10);
  passButtonLayout->addWidget(showPass);
  passButtonLayout->addStretch(1);

  QHBoxLayout *passLayout = new QHBoxLayout();
  passLayout->setMargin(0);
  passLayout->addWidget(passTree_);
  passLayout->addLayout(passButtonLayout);

  passwordsWidget_ = new QWidget();
  passwordsWidget_->setLayout(passLayout);
}

/** @brief Create widget "Language"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createLanguageWidget()
{
  languageFileList_ = new QTreeWidget();
  languageFileList_->setObjectName("languageFileList_");
  languageFileList_->setColumnCount(5);
  languageFileList_->setColumnHidden(0, true);
  languageFileList_->setColumnWidth(1, 180);
  languageFileList_->setColumnWidth(2, 60);
  languageFileList_->setColumnWidth(3, 120);

  QStringList treeItem;
  treeItem.clear();
  treeItem << "Id" << tr("Language") << tr("Version")
           << tr("Author") << tr("Contact");
  languageFileList_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "en" << QString::fromUtf8("English [EN]")
           << QString(STRPRODUCTVER)
           << "QuiteRSS Team" << "";
  QTreeWidgetItem *languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_EN"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "ar" << QString::fromUtf8("العربية [AR]")
           << "0.15.2"
           << "ahmadzxc" << "ahmad.almomani5@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_AR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "bg" << QString::fromUtf8("Български [BG]")
           << "0.19.4"
           << QString::fromUtf8("Nikolai Tsvetkov") << "koko@cybcom.net";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_BG"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "cs" << QString::fromUtf8("Čeština [CS]")
           << "0.18.6"
           << QString::fromUtf8("Matej Szendi") << "matej.szendi@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_CZ"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "de" << QString::fromUtf8("Deutsch [DE]")
           << "0.18.12"
           << "oxyd24" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_DE"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "el_GR" << QString::fromUtf8("Ελληνικά (Greece) [el_GR]")
           << QString(STRPRODUCTVER)
           << "Dimitris Siakavelis" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_GR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "es" << QString::fromUtf8("Español [ES]")
           << "0.18.6"
           << QString::fromUtf8("Cesar Muñoz") << "csarg@live.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_ES"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "fa" << QString::fromUtf8("فارسی [FA]")
           << "0.18.10"
           << "H.Mohamadi" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_FA"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "fi" << QString::fromUtf8("Suomi [FI]")
           << "0.18.12"
           << "J. S. Tuomisto" << "jstuomisto@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_FI"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "fr" << QString::fromUtf8("Français [FR]")
           << "0.19.4"
           << "Glad Deschrijver" << "glad.deschrijver@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_FR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "gl" << QString::fromUtf8("Galego [GL]")
           << "0.18.12"
           << QString::fromUtf8("Xesús M. Mosquera Carregal") << "xesusmosquera@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_GL"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "hi" << QString::fromUtf8("हिन्दी [HI]")
           << "0.16.0"
           << QString::fromUtf8("") << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_HI"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "hu" << QString::fromUtf8("Magyar [HU]")
           << "0.19.4"
           << "ZityiSoft" << "zityisoft@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_HU"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "it" << QString::fromUtf8("Italiano [IT]")
           << "0.19.4"
           << "ZeroWis" << "lightflash@hotmail.it";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_IT"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "ja" << QString::fromUtf8("日本語 [JA]")
           << "0.19.4"
           << "Masato Hashimoto" << "cabezon.hashimoto@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_JA"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "ko" << QString::fromUtf8("한국어 [KO]")
           << "0.19.4"
           << QString::fromUtf8("Yonghee Lee") << "v4321v@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_KO"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "lt" << QString::fromUtf8("Lietuvių [LT]")
           << "0.18.6"
           << QString::fromUtf8("keturidu") << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_LT"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "nl" << QString::fromUtf8("Nederlands [NL]")
           << "0.19.4"
           << "TeLLie" << "elbert.pol@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_NL"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "pl" << QString::fromUtf8("Polski [PL]")
           << "0.18.12"
           << QString::fromUtf8("Piotr Pecka") << "piotr.pecka@outlook.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_PL"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "pt_BR" << QString::fromUtf8("Português (Brazil) [pt_BR]")
           << "0.19.4"
           << QString::fromUtf8("Marcos M. Ribeiro") << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_BR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "pt_PT" << QString::fromUtf8("Português (Portugal) [pt_PT]")
           << "0.18.12"
           << QString::fromUtf8("Sérgio Marques") << "smarquespt@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_PT"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "ro_RO" << QString::fromUtf8("Limba română [ro_RO]")
           << QString(STRPRODUCTVER)
           << QString::fromUtf8("Jaff (Oprea Nicolae)") << "Jaff2002@yahoo.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_RO"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "ru" << QString::fromUtf8("Русский [RU]")
           << QString(STRPRODUCTVER)
           << "QuiteRSS Team" << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_RU"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "sk" << QString::fromUtf8("Slovenčina [SK]")
           << "0.19.4"
           << QString::fromUtf8("DAG Software (Ďanovský Ján)") << "dagsoftware@yahoo.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_SK"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "sr" << QString::fromUtf8("Српски [SR]")
           << "0.18.12"
           << "Ozzii" << "ozzii.translate@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_SR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "sv" << QString::fromUtf8("Svenska [SV]")
           << QString(STRPRODUCTVER)
           << QString::fromUtf8("Åke Engelbrektson") << "eson57@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_SV"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "tg_TJ" << QString::fromUtf8("Тоҷикӣ [tg_TJ]")
           << "0.17.5"
           << QString::fromUtf8("Kobilov Iskandar") << "kabilov.iskandar@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_TJ"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "tr" << QString::fromUtf8("Türkçe [TR]")
           << "0.19.4"
           << QString::fromUtf8("Mert Başaranoğlu") << "mertbasaranoglu@gmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_TR"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "uk" << QString::fromUtf8("Українська [UK]")
           << "0.19.4"
           << QString::fromUtf8("Сергій Левицький") << "leon21sl@yandex.ua";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_UK"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "vi" << QString::fromUtf8("Tiếng Việt [VI]")
           << "0.14.1"
           << QString::fromUtf8("Phan Anh") << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_VI"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "zh_CN" << QString::fromUtf8("中文 (China) [zh_CN]")
           << "0.18.12"
           << QString::fromUtf8("wwj402") << "";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_CN"));
  languageFileList_->addTopLevelItem(languageItem);

  treeItem.clear();
  treeItem << "zh_TW" << QString::fromUtf8("中文 (Taiwan) [zh_TW]")
           << "0.19.4"
           << QString::fromUtf8("Hulen (破滅刃)") << "shift0106@hotmail.com";
  languageItem = new QTreeWidgetItem(treeItem);
  languageItem->setIcon(1, QIcon(":/flags/flag_TW"));
  languageFileList_->addTopLevelItem(languageItem);

  QString linkWikiStr =
      QString("<a href='https://quiterss.org/en/development'>Link for translators</a>");
  QLabel *linkTranslators = new QLabel(linkWikiStr);
  linkTranslators->setOpenExternalLinks(true);

  QVBoxLayout *languageLayout = new QVBoxLayout();
  languageLayout->setMargin(0);
  languageLayout->addWidget(new QLabel(tr("Choose language:")));
  languageLayout->addWidget(languageFileList_);
  languageLayout->addWidget(linkTranslators);

  languageWidget_ = new QWidget();
  languageWidget_->setLayout(languageLayout);
}

/** @brief Create widget "Fonts and Colors"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createFontsColorsWidget()
{
  //! tab "Fonts"

  fontsTree_ = new QTreeWidget();
  fontsTree_->setObjectName("fontTree");
  fontsTree_->setColumnCount(3);
  fontsTree_->setColumnHidden(0, true);
  fontsTree_->setColumnWidth(1, 260);

  QStringList treeItem;
  treeItem << "Id" << tr("Type") << tr("Font");
  fontsTree_->setHeaderLabels(treeItem);

  treeItem.clear();
  treeItem << "0" << tr("Feeds list font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "1" << tr("News list font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "2" << tr("News title font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "3" << tr("News text font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << "4" << tr("Notification font");
  fontsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));

  fontsTree_->setCurrentItem(fontsTree_->topLevelItem(0));
  connect(fontsTree_, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(slotFontChange()));

  QPushButton *fontChange = new QPushButton(tr("Change..."));
  connect(fontChange, SIGNAL(clicked()), this, SLOT(slotFontChange()));
  QPushButton *fontReset = new QPushButton(tr("Reset"));
  connect(fontReset, SIGNAL(clicked()), this, SLOT(slotFontReset()));
  QVBoxLayout *fontsButtonLayout = new QVBoxLayout();
  fontsButtonLayout->addWidget(fontChange);
  fontsButtonLayout->addWidget(fontReset);
  fontsButtonLayout->addStretch(1);

  QHBoxLayout *mainFontsLayout = new QHBoxLayout();
  mainFontsLayout->addWidget(fontsTree_, 1);
  mainFontsLayout->addLayout(fontsButtonLayout);

  QWidget *fontsWidget = new QWidget();
  fontsWidget->setLayout(mainFontsLayout);

  //! tab "Colors"

  colorsTree_ = new QTreeWidget(this);
  colorsTree_->setObjectName("colorsTree_");
  colorsTree_->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
  colorsTree_->setRootIsDecorated(false);
  colorsTree_->setColumnCount(2);
  colorsTree_->setColumnHidden(1, true);
  colorsTree_->setHeaderHidden(true);

  treeItem.clear();
  treeItem << tr("Feeds list color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Feeds list background");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("News list color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("News list background");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Focused news color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Focused news background color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Link color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Title color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Date color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Author color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("News text color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("News title background");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("News background");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Feed with new news");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Count of unread news in feeds tree");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Text color of new news");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Text color of unread news");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Focused feed color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Focused feed background color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Disabled feed");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Alternating row colors");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Notification text color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));
  treeItem.clear();
  treeItem << tr("Notification background color");
  colorsTree_->addTopLevelItem(new QTreeWidgetItem(treeItem));

  colorsTree_->setCurrentItem(colorsTree_->topLevelItem(0));

  connect(colorsTree_, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(slotColorChange()));

  QPushButton *colorChange = new QPushButton(tr("Change..."));
  connect(colorChange, SIGNAL(clicked()), this, SLOT(slotColorChange()));
  QPushButton *colorReset = new QPushButton(tr("Reset"));
  connect(colorReset, SIGNAL(clicked()), this, SLOT(slotColorReset()));
  QVBoxLayout *colorsButtonLayout = new QVBoxLayout();
  colorsButtonLayout->addWidget(colorChange);
  colorsButtonLayout->addWidget(colorReset);
  colorsButtonLayout->addStretch(1);

  QHBoxLayout *colorsLayout = new QHBoxLayout();
  colorsLayout->addWidget(colorsTree_);
  colorsLayout->addLayout(colorsButtonLayout);

  // S-5: accent color presets + custom hex
  accentColorCombo_ = new QComboBox();
  accentColorCombo_->addItem(tr("Theme default"), "");
  accentColorCombo_->addItem(tr("Blue"), "#0F62FE");
  accentColorCombo_->addItem(tr("Green"), "#0F9D58");
  accentColorCombo_->addItem(tr("Red"), "#E5484D");
  accentColorCombo_->addItem(tr("Orange"), "#F76B15");
  accentColorCombo_->addItem(tr("Purple"), "#8E4EC6");
  accentColorCombo_->addItem(tr("Teal"), "#0FA3A3");
  accentColorCombo_->addItem(tr("Pink"), "#E84C88");
  accentColorCombo_->addItem(tr("Yellow"), "#D4A72C");
  accentColorEdit_ = new QLineEdit();
  accentColorEdit_->setPlaceholderText("#RRGGBB");
  accentColorEdit_->setMaxLength(7);
  accentColorEdit_->setFixedWidth(110);
  QHBoxLayout *accentLayout = new QHBoxLayout();
  accentLayout->setMargin(0);
  accentLayout->addWidget(new QLabel(tr("Accent color:")));
  accentLayout->addWidget(accentColorCombo_);
  accentLayout->addWidget(new QLabel(tr("Custom hex:")));
  accentLayout->addWidget(accentColorEdit_);
  accentLayout->addStretch();
  connect(accentColorCombo_, SIGNAL(currentIndexChanged(int)),
          this, SLOT(slotAccentColorChanged(int)));

  QWidget *colorsWidget_ = new QWidget(this);
  QVBoxLayout *colorsVLayout = new QVBoxLayout(colorsWidget_);
  colorsVLayout->addLayout(colorsLayout);
  colorsVLayout->addWidget(new QLabel(
      tr("Accent color (applies to selected items, links and highlights):")));
  colorsVLayout->addLayout(accentLayout);

  //! tab "Fonts Browser"

  browserStandardFont_ = new QFontComboBox();
  browserFixedFont_ = new QFontComboBox();
  browserSerifFont_ = new QFontComboBox();
  browserSansSerifFont_ = new QFontComboBox();
  browserCursiveFont_ = new QFontComboBox();
  browserFantasyFont_ = new QFontComboBox();

  QGridLayout *browserFontFamiliesLayout = new QGridLayout();
  browserFontFamiliesLayout->setColumnStretch(2, 1);
  browserFontFamiliesLayout->setContentsMargins(15, 0, 5, 10);
  browserFontFamiliesLayout->addWidget(new QLabel(tr("Standard")), 0, 0);
  browserFontFamiliesLayout->addWidget(browserStandardFont_, 0, 1);
  browserFontFamiliesLayout->addWidget(new QLabel(tr("Fixed")), 1, 0);
  browserFontFamiliesLayout->addWidget(browserFixedFont_, 1, 1);
  browserFontFamiliesLayout->addWidget(new QLabel(tr("Serif")), 2, 0);
  browserFontFamiliesLayout->addWidget(browserSerifFont_, 2, 1);
  browserFontFamiliesLayout->addWidget(new QLabel(tr("Sans Serif")), 3, 0);
  browserFontFamiliesLayout->addWidget(browserSansSerifFont_, 3, 1);
  browserFontFamiliesLayout->addWidget(new QLabel(tr("Cursive")), 4, 0);
  browserFontFamiliesLayout->addWidget(browserCursiveFont_, 4, 1);
  browserFontFamiliesLayout->addWidget(new QLabel(tr("Fantasy")), 5, 0);
  browserFontFamiliesLayout->addWidget(browserFantasyFont_, 5, 1);

  browserDefaultFontSize_ = new QSpinBox();
  browserDefaultFontSize_->setRange(0, 99);
  browserFixedFontSize_ = new QSpinBox();
  browserFixedFontSize_->setRange(0, 99);
  browserMinFontSize_ = new QSpinBox();
  browserMinFontSize_->setRange(0, 99);
  browserMinLogFontSize_ = new QSpinBox();
  browserMinLogFontSize_->setRange(0, 99);

  QGridLayout *browserFontSizesLayout = new QGridLayout();
  browserFontSizesLayout->setColumnStretch(2, 1);
  browserFontSizesLayout->setContentsMargins(15, 0, 5, 0);
  browserFontSizesLayout->addWidget(new QLabel(tr("Default font size")), 0, 0);
  browserFontSizesLayout->addWidget(browserDefaultFontSize_, 0, 1);
  browserFontSizesLayout->addWidget(new QLabel(tr("Fixed font size")), 1, 0);
  browserFontSizesLayout->addWidget(browserFixedFontSize_, 1, 1);
  browserFontSizesLayout->addWidget(new QLabel(tr("Minimum font size")), 2, 0);
  browserFontSizesLayout->addWidget(browserMinFontSize_, 2, 1);
  browserFontSizesLayout->addWidget(new QLabel(tr("Minimum logical font size")), 3, 0);
  browserFontSizesLayout->addWidget(browserMinLogFontSize_, 3, 1);

  QVBoxLayout *fontsBrowserLayout = new QVBoxLayout();
  fontsBrowserLayout->addWidget(new QLabel(tr("Font families:")));
  fontsBrowserLayout->addLayout(browserFontFamiliesLayout);
  fontsBrowserLayout->addWidget(new QLabel(tr("Font sizes:")));
  fontsBrowserLayout->addLayout(browserFontSizesLayout);
  fontsBrowserLayout->addStretch();

  QWidget *fontsBrowserWidget_ = new QWidget(this);
  fontsBrowserWidget_->setLayout(fontsBrowserLayout);


  fontsColorsWidget_ = new QTabWidget();
  fontsColorsWidget_->addTab(fontsWidget, tr("Fonts"));
  fontsColorsWidget_->addTab(fontsBrowserWidget_, tr("Fonts Browser"));
  fontsColorsWidget_->addTab(colorsWidget_, tr("Colors"));
}

/** @brief Create widget "Shortcuts"
 *----------------------------------------------------------------------------*/
void OptionsDialog::createShortcutWidget()
{
  filterShortcut_ = new LineEdit(this, tr("Filter"));

  shortcutTree_ = new QTreeView(this);
  shortcutTree_->setObjectName("shortcutTree");
  shortcutTree_->setSortingEnabled(false);
  shortcutTree_->setSelectionBehavior(QAbstractItemView::SelectRows);
  shortcutTree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  shortcutTree_->setRootIsDecorated(false);

  shortcutModel_ = new QStandardItemModel();
  shortcutModel_->setColumnCount(5);
  shortcutProxyModel_ = new QSortFilterProxyModel();
  shortcutProxyModel_->setSourceModel(shortcutModel_);
  shortcutProxyModel_->setFilterKeyColumn(-1);
  shortcutProxyModel_->setFilterRole(Qt::UserRole + 1);
  shortcutProxyModel_->setFilterCaseSensitivity(Qt::CaseInsensitive);
  shortcutTree_->setModel(shortcutProxyModel_);

  shortcutTree_->hideColumn(3);
  shortcutTree_->hideColumn(4);
  shortcutTree_->header()->setStretchLastSection(true);
  shortcutTree_->setColumnWidth(0, 215);
  shortcutTree_->setColumnWidth(1, 165);
  shortcutTree_->setColumnWidth(2, 120);

  QStringList treeItem;
  treeItem << tr("Action") << tr("Description") << tr("Shortcut")
           << "ObjectName" << "Data";
  shortcutModel_->setHorizontalHeaderLabels(treeItem);

  editShortcut_ = new LineEdit();
  QPushButton *resetShortcutButton = new QPushButton(tr("Reset"));

  QHBoxLayout *editShortcutLayout = new QHBoxLayout();
  editShortcutLayout->addWidget(new QLabel(tr("Shortcut:")));
  editShortcutLayout->addWidget(editShortcut_, 1);
  editShortcutLayout->addWidget(resetShortcutButton);

  editShortcutBox = new QGroupBox();
  editShortcutBox->setEnabled(false);
  editShortcutBox->setLayout(editShortcutLayout);

  warningShortcut_ = new QLabel();

  QVBoxLayout *shortcutLayout = new QVBoxLayout();
  shortcutLayout->setMargin(0);
  shortcutLayout->addWidget(filterShortcut_);
  shortcutLayout->addWidget(shortcutTree_, 1);
  shortcutLayout->addWidget(warningShortcut_);
  shortcutLayout->addWidget(editShortcutBox);

  shortcutWidget_ = new QWidget();
  shortcutWidget_->setLayout(shortcutLayout);

  connect(shortcutTree_, SIGNAL(clicked(QModelIndex)),
          this, SLOT(shortcutTreeClicked(QModelIndex)));
  connect(this, SIGNAL(signalShortcutTreeUpDownPressed()),
          SLOT(slotShortcutTreeUpDownPressed()), Qt::QueuedConnection);
  connect(editShortcut_, SIGNAL(signalClear()),
          this, SLOT(slotClearShortcut()));
  connect(resetShortcutButton, SIGNAL(clicked()),
          this, SLOT(slotResetShortcut()));
  connect(filterShortcut_, SIGNAL(textChanged(QString)),
          this, SLOT(filterShortcutChanged(QString)));

  editShortcut_->installEventFilter(this);
}
//----------------------------------------------------------------------------
void OptionsDialog::createInteractionWidget()
{
  hoverMarkRead_ = new QCheckBox(tr("Mark news as read on hover"));
  hoverMarkDelay_ = new QComboBox();
  hoverMarkDelay_->addItem("0.5 s", 500);
  hoverMarkDelay_->addItem("1 s", 1000);
  hoverMarkDelay_->addItem("2 s", 2000);
  hoverMarkDelay_->setEnabled(false);
  connect(hoverMarkRead_, SIGNAL(toggled(bool)),
          hoverMarkDelay_, SLOT(setEnabled(bool)));

  scrollMarkRead_ = new QCheckBox(tr("Mark news as read when scrolled out of view"));
  viewportMarkRead_ = new QCheckBox(tr("Mark news as read when entering the viewport"));
  markExcludeOnlyStarred_ = new QCheckBox(tr("Skip auto-marking for starred news"));

  excludedGroups_ = new QLineEdit();
  excludedGroups_->setPlaceholderText(tr("Comma-separated group IDs"));
  excludedFeeds_ = new QLineEdit();
  excludedFeeds_->setPlaceholderText(tr("Comma-separated feed IDs"));

  undoMarkButton_ = new QPushButton(tr("Undo last bulk mark"));
  undoMarkButton_->setEnabled(false);

  QGridLayout *interactionLayout = new QGridLayout();
  interactionLayout->setColumnStretch(1, 1);
  interactionLayout->addWidget(hoverMarkRead_, 0, 0, 1, 1);
  interactionLayout->addWidget(hoverMarkDelay_, 0, 1, 1, 1, Qt::AlignLeft);
  interactionLayout->addWidget(scrollMarkRead_, 1, 0, 1, 1);
  interactionLayout->addWidget(viewportMarkRead_, 2, 0, 1, 1);
  interactionLayout->addWidget(markExcludeOnlyStarred_, 3, 0, 1, 1);
  interactionLayout->addWidget(new QLabel(tr("Excluded groups:")), 4, 0, 1, 1);
  interactionLayout->addWidget(excludedGroups_, 4, 1, 1, 1);
  interactionLayout->addWidget(new QLabel(tr("Excluded feeds:")), 5, 0, 1, 1);
  interactionLayout->addWidget(excludedFeeds_, 5, 1, 1, 1);
  interactionLayout->addWidget(undoMarkButton_, 6, 0, 1, 1);

  // S-4: warn before jumping to external links
  jumpOutLinkWarn_ = new QCheckBox(tr("Warn before jumping to external links"));
  // S-10: reduce animations and transitions
  reduceMotion_ = new QCheckBox(tr("Reduce animations and transitions"));
  interactionLayout->addWidget(jumpOutLinkWarn_, 7, 0, 1, 2);
  interactionLayout->addWidget(reduceMotion_, 8, 0, 1, 2);

  interactionWidget_ = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(interactionWidget_);
  layout->addLayout(interactionLayout);
  layout->addStretch();
}
//----------------------------------------------------------------------------
void OptionsDialog::createCleanupWidget()
{
  cleanupEnabled_ = new QCheckBox(tr("Enable automatic cleanup"));
  cleanupStrategyTime_ = new QRadioButton(tr("By age (days)"));
  cleanupStrategyCount_ = new QRadioButton(tr("By count (keep N news)"));
  cleanupStrategyTime_->setChecked(true);

  cleanupDays_ = new QSpinBox();
  cleanupDays_->setRange(1, 9999);
  cleanupDays_->setValue(30);
  cleanupKeepCount_ = new QSpinBox();
  cleanupKeepCount_->setRange(1, 99999);
  cleanupKeepCount_->setValue(1000);
  cleanupKeepCount_->setEnabled(false);
  connect(cleanupStrategyCount_, SIGNAL(toggled(bool)),
          cleanupKeepCount_, SLOT(setEnabled(bool)));
  connect(cleanupStrategyTime_, SIGNAL(toggled(bool)),
          cleanupDays_, SLOT(setEnabled(bool)));

  cleanupRecycleDays_ = new QSpinBox();
  cleanupRecycleDays_->setRange(1, 999);
  cleanupRecycleDays_->setValue(7);

  autoCleanUpEnable_ = new QCheckBox(tr("Run cleanup automatically during updates"));
  autoCleanUpEnable_->setChecked(false);
  autoCleanUpIntervalDays_ = new QSpinBox();
  autoCleanUpIntervalDays_->setRange(1, 999);
  autoCleanUpIntervalDays_->setValue(7);
  autoCleanUpIntervalDays_->setEnabled(false);
  connect(autoCleanUpEnable_, SIGNAL(toggled(bool)),
          autoCleanUpIntervalDays_, SLOT(setEnabled(bool)));

  QGridLayout *cleanupLayout = new QGridLayout();
  cleanupLayout->setColumnStretch(1, 1);
  cleanupLayout->addWidget(cleanupEnabled_, 0, 0, 1, 2);
  cleanupLayout->addWidget(cleanupStrategyTime_, 1, 0, 1, 1);
  cleanupLayout->addWidget(cleanupDays_, 1, 1, 1, 1, Qt::AlignLeft);
  cleanupLayout->addWidget(cleanupStrategyCount_, 2, 0, 1, 1);
  cleanupLayout->addWidget(cleanupKeepCount_, 2, 1, 1, 1, Qt::AlignLeft);
  cleanupLayout->addWidget(new QLabel(tr("Recycle bin retention (days):")), 3, 0, 1, 1);
  cleanupLayout->addWidget(cleanupRecycleDays_, 3, 1, 1, 1, Qt::AlignLeft);
  cleanupLayout->addWidget(autoCleanUpEnable_, 4, 0, 1, 2);
  cleanupLayout->addWidget(new QLabel(tr("Cleanup interval (days):")), 5, 0, 1, 1);
  cleanupLayout->addWidget(autoCleanUpIntervalDays_, 5, 1, 1, 1, Qt::AlignLeft);

  cleanupWidget_ = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(cleanupWidget_);
  layout->addLayout(cleanupLayout);
  layout->addStretch();
}
//----------------------------------------------------------------------------
void OptionsDialog::createAIWidget()
{
  aiEnabled_ = new QCheckBox(tr("Enable AI assistant"));

  aiProvider_ = new QComboBox();
  aiProvider_->addItem(tr("OpenAI"), QString("openai"));
  aiProvider_->addItem(tr("Ollama (local)"), QString("ollama"));
  aiProvider_->addItem(tr("DeepSeek"), QString("deepseek"));
  aiProvider_->addItem(tr("Moonshot"), QString("moonshot"));
  aiProvider_->addItem(tr("Zhipu GLM"), QString("zhipu"));
  aiProvider_->addItem(tr("Alibaba Qwen"), QString("qwen"));
  aiProvider_->addItem(tr("Custom"), QString("custom"));

  aiApiKey_ = new QLineEdit();
  aiApiKey_->setEchoMode(QLineEdit::Password);
  aiApiKey_->setPlaceholderText(tr("Your API key (empty for Ollama)"));

  aiBaseUrl_ = new QLineEdit();
  aiBaseUrl_->setPlaceholderText(
        "https://api.openai.com/v1/chat/completions");

  aiModel_ = new QComboBox();
  aiModel_->setEditable(true);
  aiModel_->addItem("gpt-4o-mini");
  aiModel_->addItem("gpt-4o");
  aiModel_->addItem("deepseek-chat");

  aiSummaryLength_ = new QSpinBox();
  aiSummaryLength_->setRange(1, 2000);
  aiSummaryLength_->setValue(200);

  aiMaxTokens_ = new QSpinBox();
  aiMaxTokens_->setRange(0, 128000);
  aiMaxTokens_->setValue(4096);
  aiMaxTokens_->setSpecialValueText(tr("No limit"));

  aiHistoryRetention_ = new QComboBox();
  aiHistoryRetention_->addItem(tr("Keep last 50 messages"), 50);
  aiHistoryRetention_->addItem(tr("Keep last 200 messages"), 200);
  aiHistoryRetention_->addItem(tr("Keep all"), 0);

  aiOfflineCache_ = new QCheckBox(tr("Cache AI responses offline"));

  aiAutoTranslate_ = new QCheckBox(tr("Auto-translate articles on open"));
  aiTranslateLang_ = new QComboBox();
  aiTranslateLang_->setEditable(true);
  aiTranslateLang_->addItem(tr("Chinese (Simplified)"), "zh");
  aiTranslateLang_->addItem(tr("Chinese (Traditional)"), "zh-TW");
  aiTranslateLang_->addItem(tr("English"), "en");
  aiTranslateLang_->addItem(tr("Japanese"), "ja");
  aiTranslateLang_->addItem(tr("Korean"), "ko");
  aiTranslateLang_->addItem(tr("Russian"), "ru");

  aiTranslationEngine_ = new QComboBox();
  aiTranslationEngine_->addItem(tr("Google (free)"), "google");
  aiTranslationEngine_->addItem(tr("DeepL"), "deepl");
  aiTranslationEngine_->addItem(tr("Baidu"), "baidu");
  aiTranslationEngine_->addItem(tr("AI (LLM)"), "ai");

  aiDeeplKey_ = new QLineEdit();
  aiDeeplKey_->setEchoMode(QLineEdit::Password);
  aiDeeplKey_->setPlaceholderText(tr("DeepL API key"));

  aiBaiduAppId_ = new QLineEdit();
  aiBaiduAppId_->setPlaceholderText(tr("Baidu app ID"));

  aiBaiduKey_ = new QLineEdit();
  aiBaiduKey_->setEchoMode(QLineEdit::Password);
  aiBaiduKey_->setPlaceholderText(tr("Baidu secret key"));

  connect(aiTranslationEngine_, SIGNAL(currentIndexChanged(int)),
          this, SLOT(slotTranslationEngineChanged(int)));
  slotTranslationEngineChanged(aiTranslationEngine_->currentIndex());

  aiAutoSummary_ = new QCheckBox(tr("Auto-generate summary on open"));

  aiAutoRecommend_ = new QCheckBox(tr("Auto-generate related article recommendations on open"));

  connect(aiProvider_, SIGNAL(currentIndexChanged(int)),
          this, SLOT(aiProviderChanged(int)));
  connect(aiAutoTranslate_, SIGNAL(toggled(bool)),
          this, SLOT(aiAutoTranslateToggled(bool)));
  aiProviderChanged(0);

  QGridLayout *aiLayout = new QGridLayout();
  aiLayout->setColumnStretch(1, 1);
  int row = 0;
  aiLayout->addWidget(aiEnabled_, row++, 0, 1, 2);
  aiLayout->addWidget(new QLabel(tr("Service:")), row, 0, 1, 1);
  aiLayout->addWidget(aiProvider_, row++, 1, 1, 1);
  aiLayout->addWidget(new QLabel(tr("API key:")), row, 0, 1, 1);
  aiLayout->addWidget(aiApiKey_, row++, 1, 1, 1);
  aiLayout->addWidget(new QLabel(tr("Base URL:")), row, 0, 1, 1);
  aiLayout->addWidget(aiBaseUrl_, row++, 1, 1, 1);
  aiLayout->addWidget(new QLabel(tr("Model:")), row, 0, 1, 1);
  aiLayout->addWidget(aiModel_, row++, 1, 1, 1);

  aiTestButton_ = new QPushButton(tr("Test API"));
  aiFetchModelsButton_ = new QPushButton(tr("Fetch Models"));
  aiTestStatusLabel_ = new QLabel();
  aiTestStatusLabel_->setTextFormat(Qt::RichText);
  aiTestStatusLabel_->setWordWrap(true);

  QHBoxLayout *aiTestRow = new QHBoxLayout();
  aiTestRow->addWidget(aiTestButton_);
  aiTestRow->addWidget(aiFetchModelsButton_);
  aiTestRow->addStretch();

  connect(aiTestButton_, SIGNAL(clicked()), this, SLOT(slotTestAiConnection()));
  connect(aiFetchModelsButton_, SIGNAL(clicked()),
          this, SLOT(slotFetchAiModels()));
  connect(aiAssistant_, SIGNAL(connectionTested(bool, QString)),
          this, SLOT(aiConnectionTested(bool, QString)));
  connect(aiAssistant_, SIGNAL(modelsFetched(QStringList)),
          this, SLOT(aiModelsFetched(QStringList)));

  aiLayout->addLayout(aiTestRow, row++, 0, 1, 2);
  aiLayout->addWidget(aiTestStatusLabel_, row++, 0, 1, 2);

  aiLayout->addWidget(new QLabel(tr("Summary length:")), row, 0, 1, 1);
  aiLayout->addWidget(aiSummaryLength_, row++, 1, 1, 1, Qt::AlignLeft);
  aiLayout->addWidget(new QLabel(tr("Max tokens:")), row, 0, 1, 1);
  aiLayout->addWidget(aiMaxTokens_, row++, 1, 1, 1, Qt::AlignLeft);
  aiLayout->addWidget(new QLabel(tr("History retention:")), row, 0, 1, 1);
  aiLayout->addWidget(aiHistoryRetention_, row++, 1, 1, 1, Qt::AlignLeft);
  aiLayout->addWidget(aiOfflineCache_, row++, 0, 1, 2);
  aiLayout->addWidget(aiAutoTranslate_, row++, 0, 1, 2);
  aiLayout->addWidget(new QLabel(tr("Target language:")), row, 0, 1, 1);
  aiLayout->addWidget(aiTranslateLang_, row++, 1, 1, 1, Qt::AlignLeft);
  aiLayout->addWidget(new QLabel(tr("Translation engine:")), row, 0, 1, 1);
  aiLayout->addWidget(aiTranslationEngine_, row++, 1, 1, 1, Qt::AlignLeft);
  aiLayout->addWidget(new QLabel(tr("DeepL API key:")), row, 0, 1, 1);
  aiLayout->addWidget(aiDeeplKey_, row++, 1, 1, 1);
  aiLayout->addWidget(new QLabel(tr("Baidu app ID:")), row, 0, 1, 1);
  aiLayout->addWidget(aiBaiduAppId_, row++, 1, 1, 1);
  aiLayout->addWidget(new QLabel(tr("Baidu secret key:")), row, 0, 1, 1);
  aiLayout->addWidget(aiBaiduKey_, row++, 1, 1, 1);
  aiLayout->addWidget(aiAutoSummary_, row++, 0, 1, 2);
  aiLayout->addWidget(aiAutoRecommend_, row++, 0, 1, 2);

  aiWidget_ = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(aiWidget_);
  layout->addLayout(aiLayout);
  layout->addStretch();
}

//----------------------------------------------------------------------------
void OptionsDialog::loadAiSettings()
{
  Settings settings;
  aiEnabled_->setChecked(settings.value("AI/aiEnabled", false).toBool());
  aiApiKey_->setText(settings.value("AI/apiKey").toString());
  aiBaseUrl_->setText(settings.value("AI/baseUrl").toString());
  aiModel_->setEditText(settings.value("AI/model").toString());
  aiSummaryLength_->setValue(settings.value("AI/summaryLength", 200).toInt());
  aiMaxTokens_->setValue(settings.value("AI/maxTokens", 0).toInt());
  int retentionIndex = aiHistoryRetention_->findData(
        settings.value("AI/historyRetention", 50).toInt());
  aiHistoryRetention_->setCurrentIndex(retentionIndex < 0 ? 0 : retentionIndex);
  aiOfflineCache_->setChecked(settings.value("AI/offlineCache", false).toBool());
  aiAutoTranslate_->setChecked(settings.value("AI/autoTranslate", false).toBool());
  int langIndex = aiTranslateLang_->findData(
        settings.value("AI/translateLang", "zh").toString());
  aiTranslateLang_->setCurrentIndex(langIndex < 0 ? 0 : langIndex);
  int engineIndex = aiTranslationEngine_->findData(
        settings.value("AI/translationEngine", "google").toString());
  aiTranslationEngine_->setCurrentIndex(engineIndex < 0 ? 0 : engineIndex);
  aiDeeplKey_->setText(settings.value("AI/deeplKey").toString());
  aiBaiduAppId_->setText(settings.value("AI/baiduAppId").toString());
  aiBaiduKey_->setText(settings.value("AI/baiduKey").toString());
  aiAutoSummary_->setChecked(settings.value("AI/autoSummary", true).toBool());
  aiAutoRecommend_->setChecked(settings.value("AI/autoRecommend", false).toBool());

  // Set provider last; block the signal so the preset filling in
  // aiProviderChanged() does not overwrite the stored baseUrl/model.
  int providerIndex = aiProvider_->findData(
        settings.value("AI/provider", "openai").toString());
  if (providerIndex < 0)
    providerIndex = aiProvider_->findData("custom");
  if (providerIndex < 0)
    providerIndex = 0;
  QSignalBlocker blocker(aiProvider_);
  aiProvider_->setCurrentIndex(providerIndex);

  aiAutoTranslateToggled(aiAutoTranslate_->isChecked());
  slotTranslationEngineChanged(aiTranslationEngine_->currentIndex());
}

//----------------------------------------------------------------------------
void OptionsDialog::saveAiSettings()
{
  Settings settings;
  settings.setValue("AI/aiEnabled", aiEnabled_->isChecked());
  settings.setValue("AI/provider",
                    aiProvider_->itemData(aiProvider_->currentIndex()).toString());
  settings.setValue("AI/apiKey", aiApiKey_->text());
  settings.setValue("AI/baseUrl", aiBaseUrl_->text());
  settings.setValue("AI/model", aiModel_->currentText());
  settings.setValue("AI/summaryLength", aiSummaryLength_->value());
  settings.setValue("AI/maxTokens", aiMaxTokens_->value());
  settings.setValue("AI/historyRetention",
                    aiHistoryRetention_->itemData(aiHistoryRetention_->currentIndex()).toInt());
  settings.setValue("AI/offlineCache", aiOfflineCache_->isChecked());
  settings.setValue("AI/autoTranslate", aiAutoTranslate_->isChecked());
  settings.setValue("AI/translateLang",
                    aiTranslateLang_->itemData(aiTranslateLang_->currentIndex()).toString());
  settings.setValue("AI/translationEngine",
                    aiTranslationEngine_->itemData(aiTranslationEngine_->currentIndex()).toString());
  settings.setValue("AI/deeplKey", aiDeeplKey_->text());
  settings.setValue("AI/baiduAppId", aiBaiduAppId_->text());
  settings.setValue("AI/baiduKey", aiBaiduKey_->text());
  settings.setValue("AI/autoSummary", aiAutoSummary_->isChecked());
  settings.setValue("AI/autoRecommend", aiAutoRecommend_->isChecked());
}

//----------------------------------------------------------------------------
void OptionsDialog::slotTestAiConnection()
{
  aiTestStatusLabel_->setText(tr("Testing connection..."));
  aiTestButton_->setEnabled(false);
  aiAssistant_->testConnection(aiBaseUrl_->text(), aiApiKey_->text(),
                               aiModel_->currentText());
}

//----------------------------------------------------------------------------
void OptionsDialog::aiConnectionTested(bool ok, const QString &message)
{
  aiTestButton_->setEnabled(true);
  QString escaped = message;
  escaped.replace('&', "&amp;");
  escaped.replace('<', "&lt;");
  escaped.replace('>', "&gt;");
  aiTestStatusLabel_->setText(
        ok ? QString("<font color='green'>%1</font>").arg(escaped)
           : QString("<font color='red'>%1</font>").arg(escaped));
}

//----------------------------------------------------------------------------
void OptionsDialog::slotFetchAiModels()
{
  aiTestStatusLabel_->setText(tr("Fetching available models..."));
  aiFetchModelsButton_->setEnabled(false);
  aiAssistant_->fetchModels(aiBaseUrl_->text(), aiApiKey_->text());
}

//----------------------------------------------------------------------------
void OptionsDialog::aiModelsFetched(const QStringList &models)
{
  aiFetchModelsButton_->setEnabled(true);
  if (models.isEmpty()) {
    aiTestStatusLabel_->setText(
          tr("No models were returned. Check the Base URL and API key, "
             "then try again."));
    return;
  }
  const QString current = aiModel_->currentText();
  aiModel_->clear();
  aiModel_->addItems(models);
  if (!current.isEmpty()) {
    const int idx = aiModel_->findText(current);
    if (idx >= 0)
      aiModel_->setCurrentIndex(idx);
    else
      aiModel_->setEditText(current);
  }
  aiTestStatusLabel_->setText(tr("Fetched %n model(s).", "", models.count()));
}

//----------------------------------------------------------------------------
void OptionsDialog::aiProviderChanged(int index)
{
  QString provider = aiProvider_->itemData(index).toString();
  if (provider == "ollama") {
    aiBaseUrl_->setText("http://localhost:11434/v1/chat/completions");
    aiModel_->setEditText("llama3.2:1b");
    aiModel_->clear();
    aiModel_->addItem("llama3.2:1b");
    aiModel_->addItem("llama3.1:8b");
  } else if (provider == "deepseek") {
    aiBaseUrl_->setText("https://api.deepseek.com/v1/chat/completions");
    aiModel_->setEditText("deepseek-chat");
  } else if (provider == "moonshot") {
    aiBaseUrl_->setText("https://api.moonshot.cn/v1/chat/completions");
    aiModel_->setEditText("moonshot-v1-8k");
  } else if (provider == "zhipu") {
    aiBaseUrl_->setText("https://open.bigmodel.cn/api/paas/v4/chat/completions");
    aiModel_->setEditText("glm-4-flash");
  } else if (provider == "qwen") {
    aiBaseUrl_->setText("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions");
    aiModel_->setEditText("qwen-turbo");
  } else if (provider == "custom") {
    // keep whatever the user already typed for a custom endpoint
  } else { // openai
    aiBaseUrl_->setText("https://api.openai.com/v1/chat/completions");
    aiModel_->setEditText("gpt-4o-mini");
  }
}

//----------------------------------------------------------------------------
void OptionsDialog::aiAutoTranslateToggled(bool checked)
{
  aiTranslateLang_->setEnabled(checked);
}

//----------------------------------------------------------------------------
void OptionsDialog::slotTranslationEngineChanged(int index)
{
  QString engine = aiTranslationEngine_->itemData(index).toString();
  bool deepl = (engine == "deepl");
  bool baidu = (engine == "baidu");
  aiDeeplKey_->setEnabled(deepl);
  aiBaiduAppId_->setEnabled(baidu);
  aiBaiduKey_->setEnabled(baidu);
}
//----------------------------------------------------------------------------
void OptionsDialog::slotCategoriesTreeKeyUpDownPressed()
{
  slotCategoriesItemClicked(categoriesTree_->currentItem(), 1);
}
//----------------------------------------------------------------------------
void OptionsDialog::slotCategoriesItemClicked(QTreeWidgetItem* item, int)
{
  contentLabel_->setText(item->data(1, Qt::DisplayRole).toString());
  contentStack_->setCurrentIndex(item->data(0, Qt::DisplayRole).toInt());

  if (item->data(1, Qt::DisplayRole).toString() == tr("Labels")) {
    loadLabels();
  } else if (item->data(1, Qt::DisplayRole).toString() == tr("Notifications")) {
    loadNotifier();
  } else if (item->data(1, Qt::DisplayRole).toString() ==
             tr("Manage Subscriptions")) {
    loadManageSubscriptions();
  }
}
//----------------------------------------------------------------------------
int OptionsDialog::currentIndex()
{
  return categoriesTree_->currentItem()->text(0).toInt();
}
//----------------------------------------------------------------------------
void OptionsDialog::setCurrentItem(int index)
{
  categoriesTree_->setCurrentItem(categoriesTree_->topLevelItem(index), 1);
  slotCategoriesTreeKeyUpDownPressed();
}
//----------------------------------------------------------------------------
void OptionsDialog::manualProxyToggle(bool checked)
{
  manualWidget_->setEnabled(checked);
}
//----------------------------------------------------------------------------
QNetworkProxy OptionsDialog::proxy()
{
  return networkProxy_;
}
//----------------------------------------------------------------------------
void OptionsDialog::setProxy(const QNetworkProxy proxy)
{
  networkProxy_ = proxy;
  updateProxy();
}
//----------------------------------------------------------------------------
void OptionsDialog::updateProxy()
{
  switch (networkProxy_.type()) {
  case QNetworkProxy::HttpProxy: case QNetworkProxy::Socks5Proxy:
    manualProxyButton_->setChecked(true);
    if (networkProxy_.type() == QNetworkProxy::Socks5Proxy)
      typeProxy_->setCurrentIndex(1);
    break;
  case QNetworkProxy::NoProxy:
    directConnectionButton_->setChecked(true);
    break;
  case QNetworkProxy::DefaultProxy:
  default:
    systemProxyButton_->setChecked(true);
  }
  editHost_->setText(networkProxy_.hostName());
  editPort_->setText(QVariant(networkProxy_.port()).toString());
  editUser_->setText(networkProxy_.user());
  editPassword_->setText(networkProxy_.password());
}
//----------------------------------------------------------------------------
void OptionsDialog::applyProxy()
{
  if (systemProxyButton_->isChecked()) {
    networkProxy_.setType(QNetworkProxy::DefaultProxy);
  } else if (manualProxyButton_->isChecked()) {
    if (typeProxy_->currentIndex() == 1)
      networkProxy_.setType(QNetworkProxy::Socks5Proxy);
    else
      networkProxy_.setType(QNetworkProxy::HttpProxy);
  } else {
    networkProxy_.setType(QNetworkProxy::NoProxy);
  }
  networkProxy_.setHostName(editHost_->text());
  networkProxy_.setPort(editPort_->text().toInt());
  networkProxy_.setUser(editUser_->text());
  networkProxy_.setPassword(editPassword_->text());
}
//----------------------------------------------------------------------------
QString OptionsDialog::language()
{
  QString langFileName = languageFileList_->currentItem()->data(0, Qt::DisplayRole).toString();
  return langFileName;
}
//----------------------------------------------------------------------------
void OptionsDialog::setLanguage(const QString &langFileName)
{
  // Set focus on selected language-file
  QList<QTreeWidgetItem*> list =
      languageFileList_->findItems(langFileName, Qt::MatchFixedString, 0);
  if (list.count()) {
    languageFileList_->setCurrentItem(list.at(0));
  } else {
    // can't find file, choose english
    QList<QTreeWidgetItem*> list =
        languageFileList_->findItems("en", Qt::MatchFixedString, 0);
    languageFileList_->setCurrentItem(list.at(0));
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::slotFontChange()
{
  bool bOk;
  QFont curFont;
  curFont.setFamily(fontsTree_->currentItem()->text(2).section(", ", 0, 0));
  curFont.setPointSize(fontsTree_->currentItem()->text(2).section(", ", 1).toInt());

  QFont font = QFontDialog::getFont(&bOk, curFont);
  if (bOk) {
    QString strFont = QString("%1, %2").
        arg(font.family()).
        arg(font.pointSize());
    fontsTree_->currentItem()->setText(2, strFont);
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::slotFontReset()
{
  switch (fontsTree_->currentItem()->text(0).toInt()) {
  case 2: case 3: fontsTree_->currentItem()->setText(
          2, QString("%1, %2").arg(qApp->font().family()).arg(qApp->font().pointSize()+2));
    break;
  default: fontsTree_->currentItem()->setText(
          2, QString("%1, %2").arg(qApp->font().family()).arg(qApp->font().pointSize()));
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::slotColorChange()
{
  QString colorStr = colorsTree_->currentItem()->text(1);
  QColorDialog *colorDialog = new QColorDialog(QColor(colorStr), this);
  if (colorDialog->exec() == QDialog::Rejected) {
    delete colorDialog;
    return;
  }
  QColor color = colorDialog->selectedColor();
  delete colorDialog;

  QPixmap pixmapColor(14, 14);
  pixmapColor.fill(color.name());
  colorsTree_->currentItem()->setIcon(0, pixmapColor);
  colorsTree_->currentItem()->setText(1, color.name());
}
//----------------------------------------------------------------------------
void OptionsDialog::slotColorReset()
{
  QString colorName;
  int row = colorsTree_->currentIndex().row();
  switch (row) {
  case 1: case 3: case 5: case 18:
    colorName = "";
    break;
  case 6: case 7:
    colorName = "#0066CC";
    break;
  case 8: case 9:
    colorName = "#666666";
    break;
  case 10:
    colorName = "#000000";
    break;
  case 11: case 12: case 22:
    colorName = "#FFFFFF";
    break;
  case 13: case 14:
    colorName = qApp->palette().brush(QPalette::Link).color().name();
    break;
  case 19:
    colorName = "#999999";
    break;
  case 20:
    colorName = qApp->palette().color(QPalette::AlternateBase).name();
    break;
  default:
    colorName = qApp->palette().brush(QPalette::WindowText).color().name();
  }
  QPixmap pixmapColor(14, 14);
  if (colorName.isEmpty())
    pixmapColor.fill(QColor(0, 0, 0, 0));
  else
    pixmapColor.fill(colorName);
  colorsTree_->currentItem()->setIcon(0, pixmapColor);
  colorsTree_->currentItem()->setText(1, colorName);
}
//----------------------------------------------------------------------------
void OptionsDialog::setBehaviorIconTray(int behavior)
{
  switch (behavior) {
  case CHANGE_ICON_TRAY:       changeIconTray_->setChecked(true);  break;
  case NEW_COUNT_ICON_TRAY:    newCountTray_->setChecked(true);    break;
  case UNREAD_COUNT_ICON_TRAY: unreadCountTray_->setChecked(true); break;
  default: staticIconTray_->setChecked(true);
  }
}
//----------------------------------------------------------------------------
int OptionsDialog::behaviorIconTray()
{
  if (staticIconTray_->isChecked())       return STATIC_ICON_TRAY;
  else if (changeIconTray_->isChecked())  return CHANGE_ICON_TRAY;
  else if (newCountTray_->isChecked())    return NEW_COUNT_ICON_TRAY;
  else if (unreadCountTray_->isChecked()) return UNREAD_COUNT_ICON_TRAY;
  else return STATIC_ICON_TRAY;
}
//----------------------------------------------------------------------------
void OptionsDialog::loadActionShortcut(QList<QAction *> actions, QStringList *list)
{
  QListIterator<QAction *> iter(actions);
  while (iter.hasNext()) {
    QAction *pAction = iter.next();

    QStringList treeItem;
    treeItem << pAction->text().remove("&")
             << pAction->toolTip() << pAction->shortcut().toString()
             << pAction->objectName() << pAction->data().toString();

    QList<QStandardItem *> treeItems;
    for (int i = 0; i < treeItem.count(); i++) {
      QStandardItem *item = new QStandardItem(treeItem.at(i));
      if (i == 0) {
        if (pAction->icon().isNull())
          item->setIcon(QIcon(":/images/images/noicon.png"));
        else {
          if (pAction->objectName() == "autoLoadImagesToggle") {
            item->setIcon(QIcon(":/images/imagesOn"));
            item->setText(tr("Load images"));
          } else item->setIcon(pAction->icon());
        }
      } else if (i == 1) {
        if (pAction->objectName() == "autoLoadImagesToggle") {
          item->setText(tr("Auto load images in news view"));
        }
      }
      if (i >= 0 && i <= 2) {
        item->setData(treeItem.at(i));
      }
      treeItems.append(item);
    }
    shortcutModel_->appendRow(treeItems);

    QString str = pAction->shortcut().toString();
    treeItems = shortcutModel_->findItems(str, Qt::MatchFixedString, 2);
    if ((treeItems.count() > 1) && !str.isEmpty()) {
      for (int i = 0; i < treeItems.count(); i++) {
        treeItems.at(i)->setData(QColor(Qt::red), Qt::TextColorRole);
      }
    }
  }

  listDefaultShortcut_ = list;
}
//----------------------------------------------------------------------------
void OptionsDialog::saveActionShortcut(QList<QAction *> actions, QActionGroup *labelGroup)
{
  for (int i = 0; i < shortcutModel_->rowCount(); i++) {
    QString objectName = shortcutModel_->item(i, 3)->text();
    if (objectName.contains("labelAction_")) {
      QAction *action = new QAction(parent());
      action->setIcon(shortcutModel_->item(i, 0)->icon());
      action->setText(shortcutModel_->item(i, 0)->text());
      action->setShortcut(QKeySequence(shortcutModel_->item(i, 2)->text()));
      action->setObjectName(shortcutModel_->item(i, 3)->text());
      action->setCheckable(true);
      action->setData(shortcutModel_->item(i, 4)->text());
      labelGroup->addAction(action);
      actions.append(action);
    } else {
      actions.at(i)->setShortcut(
            QKeySequence(shortcutModel_->item(i, 2)->text()));
    }
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::slotShortcutTreeUpDownPressed()
{
  shortcutTreeClicked(shortcutTree_->currentIndex());
}
//----------------------------------------------------------------------------
void OptionsDialog::shortcutTreeClicked(const QModelIndex &index)
{
  QModelIndex indexCur = shortcutProxyModel_->mapToSource(index);
  editShortcut_->setText(shortcutModel_->item(indexCur.row(), 2)->text());
  editShortcutBox->setEnabled(true);
  editShortcut_->setFocus();
  warningShortcut_->clear();
}
//----------------------------------------------------------------------------
void OptionsDialog::slotClearShortcut()
{
  QModelIndex index = shortcutProxyModel_->mapToSource(shortcutTree_->currentIndex());
  int row = index.row();
  QString str = shortcutModel_->item(row, 2)->text();
  QList<QStandardItem *> treeItems;
  treeItems = shortcutModel_->findItems(str, Qt::MatchFixedString, 2);
  if ((treeItems.count() > 1) && !str.isEmpty()) {
    for (int i = 0; i < treeItems.count(); i++) {
      if ((treeItems.count() == 2) || (treeItems.at(i)->row() == row)) {
        treeItems.at(i)->setData(shortcutModel_->item(0, 1)->data(Qt::TextColorRole),
                                 Qt::TextColorRole);
      }
    }
  }

  editShortcut_->clear();
  shortcutModel_->item(row, 2)->setText("");
  shortcutModel_->item(row, 2)->setData("");
  warningShortcut_->clear();
}
//----------------------------------------------------------------------------
void OptionsDialog::slotResetShortcut()
{
  QModelIndex index = shortcutProxyModel_->mapToSource(shortcutTree_->currentIndex());
  int row = index.row();
  QString objectName = shortcutModel_->item(row, 3)->text();
  if (objectName.contains("labelAction_"))
    return;

  QString str = shortcutModel_->item(row, 2)->text();
  QList<QStandardItem *> treeItems;
  treeItems = shortcutModel_->findItems(str, Qt::MatchFixedString, 2);
  if (!str.isEmpty()) {
    for (int i = 0; i < treeItems.count(); i++) {
      if ((treeItems.count() == 2) || (treeItems.at(i)->row() == row)) {
        treeItems.at(i)->setData(shortcutModel_->item(0, 1)->data(Qt::TextColorRole),
                                 Qt::TextColorRole);
      }
    }
  }

  str = listDefaultShortcut_->at(row);
  editShortcut_->setText(str);
  shortcutModel_->item(row, 2)->setText(str);
  shortcutModel_->item(row, 2)->setData(str);
  warningShortcut_->clear();

  if (!str.isEmpty()) {
    treeItems = shortcutModel_->findItems(str, Qt::MatchFixedString, 2);
    for (int i = 0; i < treeItems.count(); i++) {
      if (treeItems.at(i)->row() != row) {
        warningShortcut_->setText(tr("Warning: key is already assigned to") +
                                  " '" +
                                  shortcutModel_->item(treeItems.at(i)->row(), 0)->text() +
                                  "'");
      }
      if (treeItems.count() > 1) {
        treeItems.at(i)->setData(QColor(Qt::red), Qt::TextColorRole);
      }
    }
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::filterShortcutChanged(const QString & text)
{
  shortcutProxyModel_->setFilterFixedString(text);

  if (shortcutTree_->currentIndex().isValid()) {
    QModelIndex indexCur = shortcutProxyModel_->mapToSource(shortcutTree_->currentIndex());
    editShortcut_->setText(shortcutModel_->item(indexCur.row(), 2)->text());
    editShortcutBox->setEnabled(true);
  } else {
    editShortcut_->setText("");
    editShortcutBox->setEnabled(false);
  }
  warningShortcut_->clear();
}
//----------------------------------------------------------------------------
void OptionsDialog::setOpeningFeed(int action)
{
  switch (action) {
  case 1: positionFirstNews_->setChecked(true); break;
  case 2: nottoOpenNews_->setChecked(true); break;
  case 3: positionUnreadNews_->setChecked(true); break;
  default: positionLastNews_->setChecked(true);
  }
}
//----------------------------------------------------------------------------
int OptionsDialog::getOpeningFeed()
{
  if (positionLastNews_->isChecked())     return 0;
  else if (positionFirstNews_->isChecked()) return 1;
  else if (nottoOpenNews_->isChecked()) return 2;
  else if (positionUnreadNews_->isChecked()) return 3;
  else return 0;
}
//----------------------------------------------------------------------------
void OptionsDialog::selectionBrowser()
{
  QString path;

  QFileInfo file(otherExternalBrowserEdit_->text());
  if (file.isFile()) path = otherExternalBrowserEdit_->text();
  else path = file.path();

  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Open File..."),
                                                  path);
  if (!fileName.isEmpty())
    otherExternalBrowserEdit_->setText(fileName);
}
//----------------------------------------------------------------------------
void OptionsDialog::applyWhitelist()
{
  mainApp->c2fSetEnabled(c2fEnabled_->isChecked());
  QStringList whitelist;
  for (int i = 0; i < c2fWhitelist_->topLevelItemCount(); i++) {
    whitelist.append(c2fWhitelist_->topLevelItem(i)->text(0));
  }
  mainApp->c2fSetWhitelist(whitelist);
}
//----------------------------------------------------------------------------
void OptionsDialog::selectionUserStyleNews()
{
  QString path(mainApp->resourcesDir() % "/style");
  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Select Style Sheet File"),
                                                  path, "*.css");
  if (!fileName.isEmpty())
    styleSheetNewsEdit_->setText(fileName);
}
//----------------------------------------------------------------------------
void OptionsDialog::resetUserStyleNews()
{
  styleSheetNewsEdit_->clear();
}
//----------------------------------------------------------------------------
void OptionsDialog::selectionSoundNotifer()
{
  QString path;

  QFileInfo file(editSoundNotifer_->text());
  if (file.isFile()) path = editSoundNotifer_->text();
  else path = file.path();

  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Open File..."),
                                                  path, "*.wav");
  if (!fileName.isEmpty())
    editSoundNotifer_->setText(fileName);
}

void OptionsDialog::slotPlaySoundNotifer()
{
  if (!editSoundNotifer_->text().isEmpty())
    emit signalPlaySound(editSoundNotifer_->text());
}

//----------------------------------------------------------------------------
void OptionsDialog::feedsTreeNotifyItemChanged(QTreeWidgetItem *item, int column)
{
  if ((column != 0) || itemNotChecked_) return;

  itemNotChecked_ = true;
  if (item->checkState(0) == Qt::Unchecked) {
    setCheckStateItem(item, Qt::Unchecked);

    QTreeWidgetItem *parentItem = item->parent();
    while (parentItem) {
      parentItem->setCheckState(0, Qt::Unchecked);
      parentItem = parentItem->parent();
    }
  } else {
    setCheckStateItem(item, Qt::Checked);
  }
  itemNotChecked_ = false;
}
//----------------------------------------------------------------------------
void OptionsDialog::setCheckStateItem(QTreeWidgetItem *item, Qt::CheckState state)
{
  for (int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem *childItem = item->child(i);
    childItem->setCheckState(0, state);
    setCheckStateItem(childItem, state);
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::loadLabels()
{
  if (loadLabelsOk_) return;
  loadLabelsOk_ = true;

  idLabels_.clear();

  QSqlQuery q;
  q.exec("SELECT id, name, image, color_text, color_bg, num FROM labels ORDER BY num");
  while (q.next()) {
    int idLabel = q.value(0).toInt();
    QString nameLabel = q.value(1).toString();
    if ((idLabel <= 6) && (MainWindow::nameLabels().at(idLabel-1) == nameLabel)) {
      nameLabel = MainWindow::trNameLabels().at(idLabel-1);
    }
    QByteArray byteArray = q.value(2).toByteArray();
    QString colorText = q.value(3).toString();
    QString colorBg = q.value(4).toString();
    int numLabel = q.value(5).toInt();
    QPixmap imageLabel;
    if (!byteArray.isNull())
      imageLabel.loadFromData(byteArray);
    QStringList strTreeItem;
    strTreeItem << QString::number(idLabel) << nameLabel
                << colorText << colorBg << QString::number(numLabel);
    QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(strTreeItem);
    treeWidgetItem->setIcon(1, QIcon(imageLabel));
    if (!colorText.isEmpty())
      treeWidgetItem->setTextColor(1, QColor(colorText));
    if (!colorBg.isEmpty())
      treeWidgetItem->setBackgroundColor(1, QColor(colorBg));
    labelsTree_->addTopLevelItem(treeWidgetItem);
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::newLabel()
{
  LabelDialog *labelDialog = new LabelDialog(this);

  if (labelDialog->exec() == QDialog::Rejected) {
    delete labelDialog;
    return;
  }

  QString nameLabel = labelDialog->nameEdit_->text();
  QString colorText = labelDialog->colorTextStr_;
  QString colorBg = labelDialog->colorBgStr_;

  int idLabel = 0;
  for (int i = 0; i < labelsTree_->topLevelItemCount(); i++) {
    QString str = labelsTree_->topLevelItem(i)->text(0);
    if (idLabel < str.toInt())
      idLabel = str.toInt();
  }
  idLabel = idLabel + 1;

  QStringList itemStr;
  itemStr << QString::number(idLabel) << nameLabel
              << colorText << colorBg
              << QString::number(idLabel);
  QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(itemStr);
  treeWidgetItem->setIcon(1, labelDialog->icon_);
  if (!colorText.isEmpty())
    treeWidgetItem->setTextColor(1, QColor(colorText));
  if (!colorBg.isEmpty())
    treeWidgetItem->setBackgroundColor(1, QColor(colorBg));
  labelsTree_->addTopLevelItem(treeWidgetItem);
  addIdLabelList(treeWidgetItem->text(0));

  itemStr.clear();
  itemStr << nameLabel << nameLabel << ""
          << QString("labelAction_%1").arg(idLabel) << QString::number(idLabel);
  QList<QStandardItem *> treeItems;
  for (int i = 0; i < itemStr.count(); i++) {
    QStandardItem *item = new QStandardItem(itemStr.at(i));
    if (i == 0) {
      item->setIcon(labelDialog->icon_);
    }
    if (i >= 0 && i <= 2) {
      item->setData(itemStr.at(i));
    }
    treeItems.append(item);
  }
  shortcutModel_->appendRow(treeItems);

  delete labelDialog;
}
//----------------------------------------------------------------------------
void OptionsDialog::editLabel()
{
  QTreeWidgetItem *treeWidgetItem = labelsTree_->currentItem();

  QString idLabelStr = treeWidgetItem->text(0);
  QString nameLabel = treeWidgetItem->text(1);
  QString colorText = treeWidgetItem->text(2);
  QString colorBg = treeWidgetItem->text(3);

  LabelDialog *labelDialog = new LabelDialog(this);

  labelDialog->nameEdit_->setText(nameLabel);
  labelDialog->icon_ = treeWidgetItem->icon(1);
  labelDialog->colorTextStr_ = colorText;
  labelDialog->colorBgStr_ = colorBg;
  labelDialog->loadData();

  if (labelDialog->exec() == QDialog::Rejected) {
    delete labelDialog;
    return;
  }

  nameLabel = labelDialog->nameEdit_->text();
  colorText = labelDialog->colorTextStr_;
  colorBg = labelDialog->colorBgStr_;

  treeWidgetItem->setText(1, nameLabel);
  treeWidgetItem->setText(2, colorText);
  treeWidgetItem->setText(3, colorBg);
  treeWidgetItem->setIcon(1, labelDialog->icon_);
  if (!colorText.isEmpty())
    treeWidgetItem->setTextColor(1, QColor(colorText));
  if (!colorBg.isEmpty())
    treeWidgetItem->setBackgroundColor(1, QColor(colorBg));
  addIdLabelList(idLabelStr);

  QList<QStandardItem *> treeItems;
  treeItems = shortcutModel_->findItems(idLabelStr, Qt::MatchFixedString, 4);
  QStandardItem *item = treeItems.first();
  shortcutModel_->item(item->row(), 0)->setIcon(labelDialog->icon_);
  shortcutModel_->item(item->row(), 0)->setText(nameLabel);
  shortcutModel_->item(item->row(), 0)->setData(nameLabel);

  delete labelDialog;
}
//----------------------------------------------------------------------------
void OptionsDialog::deleteLabel()
{
  int labelRow = labelsTree_->currentIndex().row();
  addIdLabelList(labelsTree_->topLevelItem(labelRow)->text(0));

  QString idLabelStr = labelsTree_->topLevelItem(labelRow)->text(0);
  QList<QStandardItem *> treeItems;
  treeItems = shortcutModel_->findItems(idLabelStr, Qt::MatchFixedString, 4);
  QStandardItem *item = treeItems.first();
  shortcutModel_->removeRow(item->row());

  QTreeWidgetItem *treeItem = labelsTree_->takeTopLevelItem(labelRow);
  delete treeItem;
}
//----------------------------------------------------------------------------
void OptionsDialog::moveUpLabel()
{
  int labelRow = labelsTree_->currentIndex().row();

  addIdLabelList(labelsTree_->topLevelItem(labelRow)->text(0));
  addIdLabelList(labelsTree_->topLevelItem(labelRow-1)->text(0));

  QString idLabelStr = labelsTree_->topLevelItem(labelRow)->text(0);
  QList<QStandardItem *> treeItems;
  treeItems = shortcutModel_->findItems(idLabelStr, Qt::MatchFixedString, 4);
  int row = treeItems.first()->row();
  treeItems = shortcutModel_->takeRow(row-1);
  shortcutModel_->insertRow(row, treeItems);

  int num1 = labelsTree_->topLevelItem(labelRow)->text(4).toInt();
  int num2 = labelsTree_->topLevelItem(labelRow-1)->text(4).toInt();
  labelsTree_->topLevelItem(labelRow-1)->setText(4, QString::number(num1));
  labelsTree_->topLevelItem(labelRow)->setText(4, QString::number(num2));

  QTreeWidgetItem *treeItem = labelsTree_->takeTopLevelItem(labelRow-1);
  labelsTree_->insertTopLevelItem(labelRow, treeItem);

  if (labelsTree_->currentIndex().row() == 0)
    moveUpLabelButton_->setEnabled(false);
  if (labelsTree_->currentIndex().row() != (labelsTree_->topLevelItemCount()-1))
    moveDownLabelButton_->setEnabled(true);
}
//----------------------------------------------------------------------------
void OptionsDialog::moveDownLabel()
{
  int labelRow = labelsTree_->currentIndex().row();

  addIdLabelList(labelsTree_->topLevelItem(labelRow)->text(0));
  addIdLabelList(labelsTree_->topLevelItem(labelRow+1)->text(0));

  QString idLabelStr = labelsTree_->topLevelItem(labelRow)->text(0);
  QList<QStandardItem *> treeItems;
  treeItems = shortcutModel_->findItems(idLabelStr, Qt::MatchFixedString, 4);
  int row = treeItems.first()->row();
  treeItems = shortcutModel_->takeRow(row+1);
  shortcutModel_->insertRow(row, treeItems);

  int num1 = labelsTree_->topLevelItem(labelRow)->text(4).toInt();
  int num2 = labelsTree_->topLevelItem(labelRow+1)->text(4).toInt();
  labelsTree_->topLevelItem(labelRow+1)->setText(4, QString::number(num1));
  labelsTree_->topLevelItem(labelRow)->setText(4, QString::number(num2));

  QTreeWidgetItem *treeItem = labelsTree_->takeTopLevelItem(labelRow+1);
  labelsTree_->insertTopLevelItem(labelRow, treeItem);

  if (labelsTree_->currentIndex().row() == (labelsTree_->topLevelItemCount()-1))
    moveDownLabelButton_->setEnabled(false);
  if (labelsTree_->currentIndex().row() != 0)
    moveUpLabelButton_->setEnabled(true);
}
//----------------------------------------------------------------------------
void OptionsDialog::slotCurrentLabelChanged(QTreeWidgetItem *current,
                                           QTreeWidgetItem *)
{
  if (labelsTree_->indexOfTopLevelItem(current) == 0)
    moveUpLabelButton_->setEnabled(false);
  else moveUpLabelButton_->setEnabled(true);

  if (labelsTree_->indexOfTopLevelItem(current) == (labelsTree_->topLevelItemCount()-1))
    moveDownLabelButton_->setEnabled(false);
  else moveDownLabelButton_->setEnabled(true);

  if (labelsTree_->indexOfTopLevelItem(current) < 0) {
    editLabelButton_->setEnabled(false);
    deleteLabelButton_->setEnabled(false);
    moveUpLabelButton_->setEnabled(false);
    moveDownLabelButton_->setEnabled(false);
  } else {
    editLabelButton_->setEnabled(true);
    deleteLabelButton_->setEnabled(true);
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::applyLabels()
{
  db_.transaction();
  QSqlQuery q;

  foreach (QString idLabel, idLabels_) {
    QList<QTreeWidgetItem *> treeItems =
        labelsTree_->findItems(idLabel, Qt::MatchFixedString, 0);
    if (treeItems.count() == 0) {
      q.exec(QString("DELETE FROM labels WHERE id=='%1'").arg(idLabel));
      q.exec(QString("SELECT id, label FROM news WHERE label LIKE '%,%1,%'").arg(idLabel));
      while (q.next()) {
        QString strIdLabels = q.value(1).toString();
        strIdLabels.replace(QString(",%1,").arg(idLabel), ",");
        QSqlQuery q1;
        q1.exec(QString("UPDATE news SET label='%1' WHERE id=='%2'").
               arg(strIdLabels).arg(q.value(0).toInt()));
      }
    } else {
      QString nameLabel = treeItems.at(0)->text(1);
      const int labelId = idLabel.toInt();
      if ((labelId > 0) && (labelId <= 6) &&
          (MainWindow::trNameLabels().at(labelId - 1) == nameLabel)) {
        nameLabel = MainWindow::nameLabels().at(labelId - 1);
      }
      QPixmap icon = treeItems.at(0)->icon(1).pixmap(16, 16);
      QByteArray iconData;
      QBuffer    buffer(&iconData);
      buffer.open(QIODevice::WriteOnly);
      icon.save(&buffer, "PNG");
      buffer.close();
      QString colorText = treeItems.at(0)->text(2);
      QString colorBg = treeItems.at(0)->text(3);
      int numLabel = treeItems.at(0)->text(4).toInt();

      q.exec(QString("SELECT * FROM labels WHERE id=='%1'").arg(idLabel));
      if (q.next()) {
        q.prepare("UPDATE labels SET name=?, image=?, color_text=?, color_bg=?, num=? "
                  "WHERE id=?");
        q.addBindValue(nameLabel);
        q.addBindValue(iconData);
        q.addBindValue(colorText);
        q.addBindValue(colorBg);
        q.addBindValue(numLabel);
        q.addBindValue(idLabel);
        q.exec();
      } else {
        q.prepare("INSERT INTO labels(name, image, color_text, color_bg, num) "
                  "VALUES (:name, :image, :color_text, :color_bg, :num)");
        q.bindValue(":name", nameLabel);
        q.bindValue(":image", iconData);
        q.bindValue(":color_text", colorText);
        q.bindValue(":color_bg", colorBg);
        q.bindValue(":num", numLabel);
        q.exec();
      }
    }
  }

  db_.commit();
}

/** @brief Add id for editing label
 * @param idLabel id for label
 *----------------------------------------------------------------------------*/
void OptionsDialog::addIdLabelList(const QString &idLabel)
{
  if (!idLabels_.contains(idLabel)) idLabels_.append(idLabel);
}
//----------------------------------------------------------------------------
void OptionsDialog::loadNotifier()
{
  if (loadNotifierOk_) return;
  loadNotifierOk_ = true;

  itemNotChecked_ = true;

  QSqlQuery q;
  QQueue<int> parentIds;
  parentIds.enqueue(0);
  while (!parentIds.empty()) {
    int parentId = parentIds.dequeue();
    QString qStr = QString("SELECT text, id, image, xmlUrl FROM feeds WHERE parentId='%1' ORDER BY rowToParent").
        arg(parentId);
    q.exec(qStr);
    while (q.next()) {
      QString feedText = q.value(0).toString();
      QString feedId = q.value(1).toString();
      QByteArray byteArray = q.value(2).toByteArray();
      QString xmlUrl = q.value(3).toString();

      QStringList treeItem;
      treeItem << feedText << feedId;
      QTreeWidgetItem *treeWidgetItem = new QTreeWidgetItem(treeItem);

      treeWidgetItem->setCheckState(0, Qt::Unchecked);
      QSqlQuery q1;
      qStr = QString("SELECT value FROM feeds_ex WHERE feedId='%1' AND name='showNotification'").
          arg(feedId);
      q1.exec(qStr);
      if (q1.next()) {
        if (q1.value(0).toInt() == 1)
          treeWidgetItem->setCheckState(0, Qt::Checked);
      } else {
        qStr = QString("INSERT INTO feeds_ex(feedId, name, value) VALUES ('%1', 'showNotification', '0')").
            arg(feedId);
        q1.exec(qStr);
      }
      if (treeWidgetItem->checkState(0) == Qt::Unchecked)
        feedsTreeNotify_->topLevelItem(0)->setCheckState(0, Qt::Unchecked);

      QPixmap iconItem;
      if (xmlUrl.isEmpty()) {
        iconItem.load(":/images/folder");
      } else {
        if (byteArray.isNull() || mainApp->mainWindow()->defaultIconFeeds_) {
          iconItem.load(":/images/feed");
        } else {
          iconItem.loadFromData(QByteArray::fromBase64(byteArray));
        }
      }
      treeWidgetItem->setIcon(0, iconItem);

      QList<QTreeWidgetItem *> treeItems =
            feedsTreeNotify_->findItems(QString::number(parentId),
                                                       Qt::MatchFixedString | Qt::MatchRecursive,
                                                       1);
      treeItems.at(0)->addChild(treeWidgetItem);
      if (xmlUrl.isEmpty())
        parentIds.enqueue(feedId.toInt());
    }
  }
  feedsTreeNotify_->expandAll();
  itemNotChecked_ = false;
}
//----------------------------------------------------------------------------
void OptionsDialog::applyNotifier()
{
  mainApp->mainWindow()->idFeedsNotifyList_.clear();

  // The notifier page is loaded lazily; guard against an empty tree.
  if (feedsTreeNotify_->topLevelItemCount() == 0)
    return;

  feedsTreeNotify_->expandAll();
  QTreeWidgetItem *treeWidgetItem =
      feedsTreeNotify_->itemBelow(feedsTreeNotify_->topLevelItem(0));
  db_.transaction();
  while (treeWidgetItem) {
    int check = 0;
    if (treeWidgetItem->checkState(0) == Qt::Checked)
      check = 1;
    QSqlQuery q;
    QString qStr = QString("UPDATE feeds_ex SET value='%1' WHERE feedId='%2' AND name='showNotification'").
        arg(check).arg(treeWidgetItem->text(1).toInt());
    q.exec(qStr);

    if (check && onlySelectedFeeds_->isChecked())
      mainApp->mainWindow()->idFeedsNotifyList_.append(treeWidgetItem->text(1).toInt());

    treeWidgetItem = feedsTreeNotify_->itemBelow(treeWidgetItem);
  }
  db_.commit();
}

/** @brief Show notifier test window
 *----------------------------------------------------------------------------*/
void OptionsDialog::showNotification()
{
  if (notificationWidget_) delete notificationWidget_;
  QList<int> idFeedList;
  QList<int> cntNewNewsList;
  QList<int> idColorList;
  QStringList colorList;
  notificationWidget_ = new NotificationWidget(idFeedList, cntNewNewsList,
                                               idColorList, colorList,
                                               this, this);

  connect(notificationWidget_, SIGNAL(signalClose()),
          this, SLOT(deleteNotification()));

  notificationWidget_->show();
}

/** @brief Destroy notifier test window
 *----------------------------------------------------------------------------*/
void OptionsDialog::deleteNotification()
{
  notificationWidget_->deleteLater();
  notificationWidget_ = NULL;
}
//----------------------------------------------------------------------------
void OptionsDialog::slotDeletePass()
{
  if (passTree_->topLevelItemCount())
    passTree_->currentItem()->setHidden(true);
}
//----------------------------------------------------------------------------
void OptionsDialog::slotDeleteAllPass()
{
  for (int i = 0; i < passTree_->topLevelItemCount(); i++) {
    passTree_->topLevelItem(i)->setHidden(true);
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::slotShowPass()
{
  if (passTree_->isColumnHidden(3)) {
    passTree_->showColumn(3);
    passTree_->setColumnWidth(1, passTree_->columnWidth(1) - passTree_->columnWidth(3));
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::applyPass()
{
  db_.transaction();
  QSqlQuery q;
  for (int i = 0; i < passTree_->topLevelItemCount(); i++) {
    if (passTree_->isItemHidden(passTree_->topLevelItem(i))) {
      QString id = passTree_->topLevelItem(i)->text(0);
      q.exec(QString("DELETE FROM passwords WHERE id=='%1'").arg(id));
    }
  }
  db_.commit();
}
//----------------------------------------------------------------------------
void OptionsDialog::selectionUserStyleBrowser()
{
  QString path;

  QFileInfo file(userStyleBrowserEdit_->text());
  if (file.isFile()) path = userStyleBrowserEdit_->text();
  else path = file.path();

  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Select Style Sheet File"),
                                                  path, "*.css");

  if (!fileName.isEmpty())
    userStyleBrowserEdit_->setText(fileName);
}
//----------------------------------------------------------------------------
void OptionsDialog::selectionDirDiskCache()
{
  QString dirStr = QFileDialog::getExistingDirectory(this, tr("Open Directory..."),
                                                       dirDiskCacheEdit_->text(),
                                                       QFileDialog::ShowDirsOnly
                                                       | QFileDialog::DontResolveSymlinks);
  if (!dirStr.isEmpty())
    dirDiskCacheEdit_->setText(dirStr);
}
//----------------------------------------------------------------------------
void OptionsDialog::addWhitelist()
{
  QString site = QInputDialog::getText(this, tr("Add site to whitelist"),
                                       tr("Site without 'http://' (ex. youtube.com)"));
  if (site.isEmpty())
    return;

  c2fWhitelist_->insertTopLevelItem(0, new QTreeWidgetItem(QStringList(site)));
}
//----------------------------------------------------------------------------
void OptionsDialog::removeWhitelist()
{
  QTreeWidgetItem* item = c2fWhitelist_->currentItem();
  if (!item)
    return;

  delete item;
}
//----------------------------------------------------------------------------
void OptionsDialog::selectionDownloadLocation()
{
  QString dirStr = QFileDialog::getExistingDirectory(this, tr("Open Directory..."),
                                                     downloadLocationEdit_->text(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);
  if (!dirStr.isEmpty())
    downloadLocationEdit_->setText(dirStr);
}
//----------------------------------------------------------------------------
void OptionsDialog::createDataManagementWidget()
{
  importFeedsButton_ = new QPushButton(tr("Import Subscriptions..."));
  importFeedsButton_->setObjectName("importFeedsButton");
  exportFeedsButton_ = new QPushButton(tr("Export Subscriptions..."));
  exportFeedsButton_->setObjectName("exportFeedsButton");
  cleanupDatabaseButton_ = new QPushButton(tr("Clean Up Database..."));
  cleanupDatabaseButton_->setObjectName("cleanupDatabaseButton");

  connect(importFeedsButton_, SIGNAL(clicked()),
          this, SIGNAL(signalImportFeedsRequested()));
  connect(exportFeedsButton_, SIGNAL(clicked()),
          this, SIGNAL(signalExportFeedsRequested()));
  connect(cleanupDatabaseButton_, SIGNAL(clicked()),
          this, SIGNAL(signalCleanupDatabaseRequested()));
}
//----------------------------------------------------------------------------
void OptionsDialog::createManageSubscriptionsWidget()
{
  manageSubscriptionsWidget_ = new QWidget(this);
  manageSubscriptionsWidget_->setObjectName("manageSubscriptionsWidget");

  subscriptionsWidget_ = new SubscriptionManagerWidget(manageSubscriptionsWidget_);

  // Data management (import/export/cleanup) merged into this page
  createDataManagementWidget();

  QFrame *separator = new QFrame(manageSubscriptionsWidget_);
  separator->setFrameShape(QFrame::HLine);
  separator->setFrameShadow(QFrame::Sunken);

  QLabel *infoLabel = new QLabel(tr("Import or export your subscriptions as "
                                    "OPML, or clean up the database."),
                                 manageSubscriptionsWidget_);
  infoLabel->setWordWrap(true);
  infoLabel->setObjectName("dataManagementInfoLabel");

  QHBoxLayout *buttonsLayout = new QHBoxLayout();
  buttonsLayout->addWidget(importFeedsButton_);
  buttonsLayout->addWidget(exportFeedsButton_);
  buttonsLayout->addWidget(cleanupDatabaseButton_);
  buttonsLayout->addStretch();

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setContentsMargins(0, 6, 0, 6);
  layout->addWidget(subscriptionsWidget_, 1);
  layout->addSpacing(10);
  layout->addWidget(separator);
  layout->addSpacing(6);
  layout->addWidget(infoLabel);
  layout->addSpacing(6);
  layout->addLayout(buttonsLayout);
  manageSubscriptionsWidget_->setLayout(layout);

  connect(subscriptionsWidget_, SIGNAL(addFeedRequested()),
          this, SIGNAL(signalAddFeedRequested()));
  connect(subscriptionsWidget_, SIGNAL(feedsChanged()),
          this, SIGNAL(signalFeedsChanged()));
  connect(subscriptionsWidget_, SIGNAL(checkStatusRequested(QList<int>)),
          this, SIGNAL(signalCheckStatusRequested(QList<int>)));
  connect(subscriptionsWidget_, SIGNAL(manageLabelsRequested()),
          this, SLOT(slotManageSubscriptionsLabels()));
  connect(subscriptionsWidget_, SIGNAL(statusMessage(QString)),
          this, SLOT(slotRssHubStatusMessage(QString)));
}
//----------------------------------------------------------------------------
void OptionsDialog::loadManageSubscriptions()
{
  subscriptionsWidget_->refresh();
}
//----------------------------------------------------------------------------
void OptionsDialog::slotManageSubscriptionsLabels()
{
  for (int i = 0; i < categoriesTree_->topLevelItemCount(); ++i) {
    QTreeWidgetItem *item = categoriesTree_->topLevelItem(i);
    if (item->data(1, Qt::DisplayRole).toString() == tr("Labels")) {
      categoriesTree_->setCurrentItem(item, 1);
      slotCategoriesTreeKeyUpDownPressed();
      break;
    }
  }
}
//----------------------------------------------------------------------------
void OptionsDialog::createRssHubWidget()
{
  rsshubWidget_ = new QWidget(this);
  rsshubWidget_->setObjectName("rsshubWidget");

  rsshubAutoSwapEnabled_ = new QCheckBox(
        tr("Automatically switch to another instance when an RSSHub instance fails"));
  rsshubAutoSwapEnabled_->setObjectName("rsshubAutoSwapEnabled");

  rsshubInstancesList_ = new QListWidget();
  rsshubInstancesList_->setObjectName("rsshubInstancesList");
  rsshubInstancesList_->setSortingEnabled(false);

  rsshubAddButton_ = new QPushButton(tr("Add"));
  rsshubAddButton_->setObjectName("rsshubAddButton");
  rsshubRemoveButton_ = new QPushButton(tr("Remove"));
  rsshubRemoveButton_->setObjectName("rsshubRemoveButton");
  rsshubCheckButton_ = new QPushButton(tr("Check Availability"));
  rsshubCheckButton_->setObjectName("rsshubCheckButton");
  rsshubFetchButton_ = new QPushButton(tr("Update From URL"));
  rsshubFetchButton_->setObjectName("rsshubFetchButton");

  QLabel *remoteLabel = new QLabel(tr("Remote instance list URL:"));
  rsshubRemoteUrlEdit_ = new QLineEdit();
  rsshubRemoteUrlEdit_->setObjectName("rsshubRemoteUrlEdit");
  rsshubRemoteUrlEdit_->setPlaceholderText(tr("https://example.com/instances.txt"));

  rsshubStatusLabel_ = new QLabel();
  rsshubStatusLabel_->setObjectName("rsshubStatusLabel");
  rsshubStatusLabel_->setWordWrap(true);

  QVBoxLayout *buttonLayout = new QVBoxLayout();
  buttonLayout->setContentsMargins(0, 0, 0, 0);
  buttonLayout->addWidget(rsshubAddButton_);
  buttonLayout->addWidget(rsshubRemoveButton_);
  buttonLayout->addSpacing(10);
  buttonLayout->addWidget(rsshubCheckButton_);
  buttonLayout->addSpacing(10);
  buttonLayout->addWidget(rsshubFetchButton_);
  buttonLayout->addStretch();

  QHBoxLayout *listLayout = new QHBoxLayout();
  listLayout->addWidget(rsshubInstancesList_, 1);
  listLayout->addLayout(buttonLayout);

  QHBoxLayout *remoteLayout = new QHBoxLayout();
  remoteLayout->addWidget(remoteLabel);
  remoteLayout->addWidget(rsshubRemoteUrlEdit_, 1);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setContentsMargins(4, 6, 4, 6);
  layout->addWidget(rsshubAutoSwapEnabled_);
  layout->addSpacing(10);
  layout->addLayout(listLayout);
  layout->addLayout(remoteLayout);
  layout->addWidget(rsshubStatusLabel_);

  // Frozen instances (auto-disabled after too many monthly failures).
  QLabel *frozenTitle = new QLabel(tr("Frozen instances (auto-disabled):"));
  layout->addWidget(frozenTitle);
  rsshubFrozenList_ = new QListWidget();
  rsshubFrozenList_->setObjectName("rsshubFrozenList");
  rsshubFrozenList_->setMaximumHeight(90);
  layout->addWidget(rsshubFrozenList_);
  rsshubUnfreezeButton_ = new QPushButton(tr("Re-enable this instance"));
  connect(rsshubUnfreezeButton_, SIGNAL(clicked()),
          this, SLOT(slotRssHubUnfreeze()));
  layout->addWidget(rsshubUnfreezeButton_);

  layout->addStretch();
  rsshubWidget_->setLayout(layout);

  connect(rsshubAddButton_, SIGNAL(clicked()),
          this, SLOT(slotRssHubAddInstance()));
  connect(rsshubRemoveButton_, SIGNAL(clicked()),
          this, SLOT(slotRssHubRemoveInstance()));
  connect(rsshubCheckButton_, SIGNAL(clicked()),
          this, SLOT(slotRssHubCheckInstances()));
  connect(rsshubFetchButton_, SIGNAL(clicked()),
          this, SLOT(slotRssHubFetchRemote()));
}
//----------------------------------------------------------------------------
void OptionsDialog::loadRssHubSettings()
{
  rsshubAutoSwapEnabled_->setChecked(RssHubInstances::autoSwapEnabled());
  rsshubInstancesList_->clear();
  // Show only active (non-frozen) instances in the main list; frozen ones are
  // displayed exclusively in the frozen list below (no double display).
  foreach (const QString &base, RssHubInstances::loadActiveInstances()) {
    QListWidgetItem *item = new QListWidgetItem(base, rsshubInstancesList_);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
  }
  rsshubFrozenList_->clear();
  foreach (const QString &base, RssHubInstances::frozenInstances()) {
    QListWidgetItem *item = new QListWidgetItem(base, rsshubFrozenList_);
    item->setForeground(Qt::gray);
    item->setToolTip(tr("Frozen: exceeded 5 refresh failures in the last "
                        "30 days. Not used for automatic switching. "
                        "Re-enable it from the list below."));
  }
  rsshubStatusLabel_->clear();
}
//----------------------------------------------------------------------------
void OptionsDialog::saveRssHubSettings()
{
  RssHubInstances::setAutoSwapEnabled(rsshubAutoSwapEnabled_->isChecked());
  QStringList instances;
  for (int i = 0; i < rsshubInstancesList_->count(); ++i)
    instances << rsshubInstancesList_->item(i)->text().trimmed();
  instances.removeAll(QString());
  // Preserve frozen instances in the saved list: saveInstances() clears the
  // frozen flag of any instance missing from the list, so omitting them here
  // would silently unfreeze everything when the user just clicks OK.
  for (int i = 0; i < rsshubFrozenList_->count(); ++i)
    instances << rsshubFrozenList_->item(i)->text().trimmed();
  RssHubInstances::saveInstances(instances);
}
//----------------------------------------------------------------------------
void OptionsDialog::slotRssHubAddInstance()
{
  QDialog dialog(this);
  dialog.setWindowTitle(tr("Add RSSHub Instances"));

  QVBoxLayout *layout = new QVBoxLayout(&dialog);

  QLabel *info = new QLabel(
        tr("Enter one instance URL per line. You may also paste a list using "
           "commas, spaces, tabs or semicolons as separators.\n"
           "Example:\n"
           "  https://rsshub.app, https://rsshub.example.com\n"
           "  https://third.example.org"), &dialog);
  info->setWordWrap(true);
  layout->addWidget(info);

  QPlainTextEdit *editor = new QPlainTextEdit(&dialog);
  editor->setPlaceholderText(tr("https://..."));
  editor->setFocus();
  layout->addWidget(editor);

  QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
  layout->addWidget(buttons);

  dialog.resize(460, 260);
  if (dialog.exec() != QDialog::Accepted)
    return;

  // Parse and normalise the input: split on newlines / commas / spaces /
  // tabs / semicolons, trim each token and drop anything that isn't a URL.
  QString raw = editor->toPlainText();
  raw.replace(',', '\n').replace(';', '\n')
     .replace('\t', '\n').replace(' ', '\n');
  QStringList parts = raw.split('\n', Qt::SkipEmptyParts);

  QStringList existing;
  for (int i = 0; i < rsshubInstancesList_->count(); ++i)
    existing << rsshubInstancesList_->item(i)->text().trimmed();

  int added = 0;
  foreach (QString token, parts) {
    token = token.trimmed();
    if (token.isEmpty())
      continue;
    if (!(token.startsWith("http://") || token.startsWith("https://")))
      continue;                      // ignore comments / non-URL garbage
    if (token.endsWith('/'))
      token.chop(1);                 // normalise trailing slash
    if (existing.contains(token))
      continue;                      // skip duplicates
    QListWidgetItem *item = new QListWidgetItem(token, rsshubInstancesList_);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    existing << token;
    added++;
  }

  if (added > 0)
    rsshubStatusLabel_->setText(tr("Added %1 instance(s).").arg(added));
  else
    rsshubStatusLabel_->setText(tr("Nothing to add. Enter valid instance URLs."));
}
//----------------------------------------------------------------------------
void OptionsDialog::slotRssHubRemoveInstance()
{
  QListWidgetItem *item = rsshubInstancesList_->currentItem();
  if (item)
    delete item;
}
//----------------------------------------------------------------------------
void OptionsDialog::slotRssHubCheckInstances()
{
  // Normalise the list currently shown in the dialog so trailing slashes
  // and casing don't cause false negatives when matching health results.
  QStringList instances;
  for (int i = 0; i < rsshubInstancesList_->count(); ++i) {
    QString base = RssHubInstances::normalizeBase(
        rsshubInstancesList_->item(i)->text());
    if (!base.isEmpty() && !instances.contains(base))
      instances << base;
  }
  if (instances.isEmpty()) {
    QMessageBox::information(this, tr("RSSHub Instances"),
                             tr("Instance list is empty."));
    return;
  }
  QApplication::setOverrideCursor(Qt::WaitCursor);
  QStringList healthy = RssHubInstances::checkAlive(instances);
  // Refresh the global cache in the background; the local "healthy" result
  // above is already what the dialog displays immediately.
  RssHubInstances::updateHealthyCacheAsync();
  QApplication::restoreOverrideCursor();

  // Re-normalise each displayed item before comparing against the healthy
  // set, so an entry the user typed with a trailing slash still matches.
  int checked = 0;
  for (int i = 0; i < rsshubInstancesList_->count(); ++i) {
    QListWidgetItem *item = rsshubInstancesList_->item(i);
    QString base = RssHubInstances::normalizeBase(item->text());
    bool valid = !base.isEmpty();
    bool ok = valid && healthy.contains(base);
    if (ok)
      checked++;
    item->setForeground(ok ? QColor(0, 140, 0) : QColor(200, 30, 30));
    item->setToolTip(ok ? tr("Available")
                        : (valid ? tr("Unavailable")
                                 : tr("Not a valid instance URL")));
  }
  rsshubStatusLabel_->setText(tr("%1 of %2 instances available")
                              .arg(checked).arg(instances.count()));
}
//----------------------------------------------------------------------------
void OptionsDialog::slotRssHubFetchRemote()
{
  QString url = rsshubRemoteUrlEdit_->text().trimmed();
  if (url.isEmpty()) {
    QMessageBox::information(this, tr("RSSHub Instances"),
                             tr("Enter a URL of the instance list "
                                "(one instance per line)."));
    return;
  }
  QApplication::setOverrideCursor(Qt::WaitCursor);
  QStringList list = RssHubInstances::fetchRemote(url);
  QApplication::restoreOverrideCursor();
  if (list.isEmpty()) {
    QMessageBox::warning(this, tr("RSSHub Instances"),
                         tr("Could not fetch any instances from this URL."));
    return;
  }
  for (int i = rsshubInstancesList_->count() - 1; i >= 0; --i)
    delete rsshubInstancesList_->item(i);
  foreach (const QString &base, list) {
    QListWidgetItem *item = new QListWidgetItem(base, rsshubInstancesList_);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
  }
  rsshubStatusLabel_->setText(tr("Fetched %1 instances.").arg(list.count()));
}
//----------------------------------------------------------------------------
void OptionsDialog::slotRssHubStatusMessage(const QString &message)
{
  QMessageBox::information(this, tr("Subscriptions"), message);
}
//----------------------------------------------------------------------------
void OptionsDialog::slotRssHubUnfreeze()
{
  QListWidgetItem *item = rsshubFrozenList_->currentItem();
  if (!item)
    return;
  QString base = item->text();
  RssHubInstances::unfreezeInstance(base);
  delete item;
  // Bring the instance back into the active list immediately so the user sees
  // the result without closing and reopening the dialog.
  const QString normalized = RssHubInstances::normalizeBase(base);
  if (normalized.isEmpty())
    return;
  for (int i = 0; i < rsshubInstancesList_->count(); ++i) {
    if (RssHubInstances::normalizeBase(
          rsshubInstancesList_->item(i)->text()) == normalized)
      return;
  }
  QListWidgetItem *newItem = new QListWidgetItem(base, rsshubInstancesList_);
  newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
}
//----------------------------------------------------------------------------
void OptionsDialog::slotRegisterFeedProtocol()
{
#if defined(Q_OS_WIN)
  // Register QuiteRSS as the handler for the "feed" URL protocol for the
  // current user (HKCU, no admin rights needed). This lets browser extensions
  // such as RSSHub-Radar launch QuiteRSS with feed:https://... URLs, which
  // MainApplication::receiveMessage() turns into a "New Feed" action.
  const QString exePath =
      QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
  const QString command = "\"" + exePath + "\" \"%1\"";

  QSettings reg("HKEY_CURRENT_USER\\Software\\Classes\\feed",
                QSettings::NativeFormat);
  reg.setValue(".", "URL:QuiteRSS Feed Subscription");
  reg.setValue("URL Protocol", "");
  reg.setValue("DefaultIcon/.", exePath);
  reg.setValue("shell/open/command/.", command);
  reg.sync();

  if (reg.status() == QSettings::NoError) {
    QMessageBox::information(
          this, tr("Feed protocol"),
          tr("QuiteRSS is now registered to handle feed:// links.\n\n"
             "In RSSHub-Radar (or your browser), set the default reader "
             "to the local/desktop application. Clicking a subscribe link "
             "will now open QuiteRSS."));
  } else {
    QMessageBox::warning(
          this, tr("Feed protocol"),
          tr("Could not write to the Windows registry. The registration "
             "for the feed: protocol could not be saved."));
  }
#else
  QMessageBox::information(
        this, tr("Feed protocol"),
        tr("Automatic registration is only available on Windows. On other "
           "platforms, configure your browser/extension to launch:\n\n"
           "%1 \"feed:%%s\"")
        .arg(QCoreApplication::applicationFilePath()));
#endif
}

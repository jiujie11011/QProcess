/* ============================================================
 * QProcess ReaderToolbar - 实现（骨架版）
 * 版本：v1.4
 * ============================================================ */
#include "readertoolbar.h"
#include <QHBoxLayout>
#include <QToolButton>
#include <QIcon>
#include <QAction>
#include <QMenu>
#include <QEvent>
#include <QDebug>
#if defined(QT6)
#include <QEnterEvent>
#endif

ReaderToolbar::ReaderToolbar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("readerToolbar");
    setFixedHeight(40);
    setupActions();
    setupMenu();
    setupLayout();
    updateIcons();
}

void ReaderToolbar::setArticleState(bool unread, bool starred)
{
    currentUnread_ = unread;
    currentStarred_ = starred;
    updateIcons();
}

void ReaderToolbar::setHoverExpand(bool expand)
{
    hoverExpand_ = expand;
    // TODO: 实现悬停展开/收起动画（opacity/width 动画）
    Q_UNUSED(expand);
}

void ReaderToolbar::setupActions()
{
    // 标记已读/未读
    actMarkRead_ = new QAction(this);
    actMarkRead_->setCheckable(true);
    connect(actMarkRead_, &QAction::triggered, this, [this](bool checked) {
        emit markReadRequested(!currentUnread_); // 切换状态
    });

    // 收藏/星标
    actStarred_ = new QAction(this);
    actStarred_->setCheckable(true);
    connect(actStarred_, &QAction::triggered, this, [this](bool checked) {
        emit toggleStarredRequested(!currentStarred_);
    });

    // 查看原文
    actOpenOriginal_ = new QAction(this);
    connect(actOpenOriginal_, &QAction::triggered, this, [this] {
        emit openOriginalRequested();
    });

    // 分享
    actShare_ = new QAction(this);
    connect(actShare_, &QAction::triggered, this, [this] {
        emit shareRequested();
    });

    // 更多（菜单触发器）
    actMore_ = new QAction(this);
}

void ReaderToolbar::setupMenu()
{
    moreMenu_ = new QMenu(this);
    moreMenu_->setObjectName("readerToolbarMoreMenu");

    QAction* actNewTab = moreMenu_->addAction("在新标签页打开");
    connect(actNewTab, &QAction::triggered, this, [this] { emit openInNewTabRequested(); });

    QAction* actCopyLink = moreMenu_->addAction("复制链接");
    connect(actCopyLink, &QAction::triggered, this, [this] { emit copyLinkRequested(); });

    QAction* actCopyTitle = moreMenu_->addAction("复制标题");
    connect(actCopyTitle, &QAction::triggered, this, [this] { emit copyTitleRequested(); });

    moreMenu_->addSeparator();

    QAction* actExportPdf = moreMenu_->addAction("导出为 PDF");
    connect(actExportPdf, &QAction::triggered, this, [this] { emit exportPdfRequested(); });

    QAction* actTts = moreMenu_->addAction("文本转语音");
    connect(actTts, &QAction::triggered, this, [this] { emit ttsRequested(); });

    QAction* actCustomize = moreMenu_->addAction("自定义工具栏");
    connect(actCustomize, &QAction::triggered, this, [this] { emit customizeToolbarRequested(); });

    actMore_->setMenu(moreMenu_);
}

void ReaderToolbar::setupLayout()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    auto makeBtn = [this](QAction* act, const QString& tooltip) {
        auto* btn = new QToolButton;
        btn->setDefaultAction(act);
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setIconSize(QSize(18, 18));
        btn->setFixedSize(32, 32);
        btn->setToolTip(tooltip);
        btn->setAutoRaise(true);
        return btn;
    };

    layout->addWidget(makeBtn(actMarkRead_, currentUnread_ ? "标记为已读" : "标记为未读"));
    layout->addWidget(makeBtn(actStarred_, currentStarred_ ? "取消收藏" : "收藏"));
    layout->addWidget(makeBtn(actOpenOriginal_, "查看原文"));
    layout->addWidget(makeBtn(actShare_, "分享"));
    layout->addStretch();
    layout->addWidget(makeBtn(actMore_, "更多"));
}

void ReaderToolbar::updateIcons()
{
    // TODO: 使用 SvgIconEngine 按当前主题色渲染 Lucide SVG
    // 当前用占位文本，编译通过后替换
    actMarkRead_->setText(currentUnread_ ? "◉" : "○");
    actMarkRead_->setToolTip(currentUnread_ ? "标记为已读" : "标记为未读");

    actStarred_->setText(currentStarred_ ? "★" : "☆");
    actStarred_->setToolTip(currentStarred_ ? "取消收藏" : "收藏");

    actOpenOriginal_->setText("🔗");
    actShare_->setText("📤");
    actMore_->setText("⋯");
}

#if defined(QT6)
void ReaderToolbar::enterEvent(QEnterEvent* event)
#else
void ReaderToolbar::enterEvent(QEvent* event)
#endif
{
    if (!hoverExpand_) {
        // TODO: 悬停时展开动画（显示更多按钮、或全部按钮淡入）
    }
    QWidget::enterEvent(event);
}

void ReaderToolbar::leaveEvent(QEvent* event)
{
    if (!hoverExpand_) {
        // TODO: 离开时收起动画
    }
    QWidget::leaveEvent(event);
}


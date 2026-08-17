/* ============================================================
 * QProcess NavRail - 实现（骨架版）
 * 版本：v1.4
 * ============================================================ */
#include "navrail.h"
#include <QVBoxLayout>
#include <QToolButton>
#include <QIcon>
#include <QDebug>

NavRail::NavRail(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("navRail");
    setFixedWidth(48);
    setMouseTracking(true);
    setAttribute(Qt::WA_StyledBackground, true);

    buttonGroup_ = new QButtonGroup(this);
    buttonGroup_->setExclusive(true);

    setupButtons();
    setupLayout();
    updateIcons();

    connect(buttonGroup_, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton* btn) {
        // 根据按钮映射发射 itemClicked
        if (btn == btnAll_) emit itemClicked(Item::AllArticles);
        else if (btn == btnUnread_) emit itemClicked(Item::Unread);
        else if (btn == btnStarred_) emit itemClicked(Item::Starred);
        else if (btn == btnTags_) emit itemClicked(Item::Tags);
        else if (btn == btnBrowser_) emit itemClicked(Item::BrowserTabs);
    });

    connect(btnSettings_, &QToolButton::clicked, this, &NavRail::settingsRequested);
    connect(btnSync_, &QToolButton::clicked, this, &NavRail::syncAccountRequested);
    connect(btnTheme_, &QToolButton::clicked, this, &NavRail::themeToggleRequested);
}

void NavRail::setupButtons()
{
    auto makeBtn = [this](const QString& tooltip, bool checkable = true) {
        auto* btn = new QToolButton;
        btn->setCheckable(checkable);
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setIconSize(QSize(20, 20));
        btn->setFixedSize(40, 40);
        btn->setToolTip(tooltip);
        btn->setAutoRaise(true);
        btn->setCursor(Qt::PointingHandCursor);
        buttonGroup_->addButton(btn);
        return btn;
    };

    btnAll_ = makeBtn("全部文章");
    btnUnread_ = makeBtn("未读");
    btnStarred_ = makeBtn("星标/收藏");
    btnTags_ = makeBtn("标签");
    btnBrowser_ = makeBtn("浏览器标签");
    btnSettings_ = makeBtn("设置", false);
    btnSync_ = makeBtn("同步账号", false);
    btnTheme_ = makeBtn("切换主题", false); // 底部
}

void NavRail::setupLayout()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 8);
    layout->setSpacing(4);

    // 顶部组
    layout->addWidget(btnAll_);
    layout->addWidget(btnUnread_);
    layout->addWidget(btnStarred_);
    layout->addWidget(btnTags_);
    layout->addWidget(btnBrowser_);

    layout->addStretch();

    // 底部组
    layout->addWidget(btnSync_);
    layout->addWidget(btnSettings_);
    layout->addWidget(btnTheme_);
}

void NavRail::setCurrentItem(Item item)
{
    QAbstractButton* btn = nullptr;
    switch (item) {
        case Item::AllArticles: btn = btnAll_; break;
        case Item::Unread: btn = btnUnread_; break;
        case Item::Starred: btn = btnStarred_; break;
        case Item::Tags: btn = btnTags_; break;
        case Item::BrowserTabs: btn = btnBrowser_; break;
        default: break;
    }
    if (btn) btn->setChecked(true);
}

void NavRail::setSyncStatus(bool connected, bool syncing)
{
    // TODO: 更新 btnSync_ 图标/颜色/tooltip
    // connected: 绿点/灰点；syncing: 旋转动画
    Q_UNUSED(connected);
    Q_UNUSED(syncing);
}

void NavRail::updateIcons()
{
    // TODO: 使用 SvgIconEngine 按当前主题渲染 Lucide SVG
    // 当前用占位文本，编译通过后替换
    btnAll_->setText("🏠");
    btnUnread_->setText("📥");
    btnStarred_->setText("⭐");
    btnTags_->setText("🏷");
    btnBrowser_->setText("🌐");
    btnSettings_->setText("⚙");
    btnSync_->setText("👤");
    btnTheme_->setText("◐");
}
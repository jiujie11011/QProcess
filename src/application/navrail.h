/* ============================================================
 * QProcess NavRail
 * 左侧 48px 图标导航栏（替代传统菜单栏入口职责）
 * 版本：v1.4（对应报告 §4.2, §4.3, §6.2, §13.2）
 * ============================================================ */
#ifndef NAVRAIL_H
#define NAVRAIL_H

#include <QFrame>
#include <QToolButton>
#include <QButtonGroup>
#include <QAction>

class NavRail : public QFrame
{
    Q_OBJECT
public:
    enum class Item {
        AllArticles,   // 🏠 全部文章
        Unread,        // 📥 未读
        Starred,       // ⭐ 星标/收藏
        Tags,          // 🏷 标签
        BrowserTabs,   // 🌐 浏览器标签
        Settings,      // ⚙ 设置
        SyncAccount    // 👤 同步账号
    };
    Q_ENUM(Item)

    explicit NavRail(QWidget* parent = nullptr);
    ~NavRail() override = default;

    // 设置当前选中项
    void setCurrentItem(Item item);

    // 设置同步账号状态（底部指示灯）
    void setSyncStatus(bool connected, bool syncing);

signals:
    void itemClicked(Item item);
    void themeToggleRequested();  // 底部主题快切
    void settingsRequested();
    void syncAccountRequested();

private:
    void setupButtons();
    void setupLayout();
    void updateIcons();

    QButtonGroup* buttonGroup_;
    QToolButton* btnAll_;
    QToolButton* btnUnread_;
    QToolButton* btnStarred_;
    QToolButton* btnTags_;
    QToolButton* btnBrowser_;
    QToolButton* btnSettings_;
    QToolButton* btnSync_;
    QToolButton* btnTheme_; // 底部主题切换
};

#endif // NAVRAIL_H
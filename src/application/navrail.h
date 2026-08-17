/* ============================================================
 * QProcess NavRail
 * Left 48px icon nav rail (replaces traditional menu bar entry duties)
 * Version: v1.4 (report section 4.2, 4.3, 6.2, 13.2)
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
        AllArticles,   // All articles
        Unread,        // Unread
        Starred,       // Starred/favorites
        Tags,          // Tags
        BrowserTabs,   // Browser tabs
        Settings,      // Settings
        SyncAccount    // Sync account
    };
    Q_ENUM(Item)

    explicit NavRail(QWidget* parent = nullptr);
    ~NavRail() override = default;

    // Set the current selected item
    void setCurrentItem(Item item);

    // Set sync account status (bottom indicator light)
    void setSyncStatus(bool connected, bool syncing);

signals:
    void itemClicked(Item item);
    void themeToggleRequested();  // bottom theme quick toggle
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
    QToolButton* btnTheme_; // bottom theme toggle
};

#endif // NAVRAIL_H
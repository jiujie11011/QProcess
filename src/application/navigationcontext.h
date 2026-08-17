/* ============================================================
 * QProcess NavigationContext
 * Two-level navigation state: left feed selected -> middle list filter + top title
 *                             middle article selected -> right reader + back button
 * Version: v1.4 (report section 13.2, 4.2, 4.3)
 * ============================================================ */
#ifndef NAVIGATIONCONTEXT_H
#define NAVIGATIONCONTEXT_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QDateTime>
#include <QStringList>
#include <QHash>

class NavigationContext : public QObject
{
    Q_OBJECT
public:
    enum class Level {
        FeedList,   // level 1: feed list (left tree)
        ArticleList,// level 2: article list (middle)
        Reading     // level 3: reading detail (right/fullscreen)
    };
    Q_ENUM(Level)

    enum class FilterMode {
        All,        // all articles
        Unread,     // unread
        Starred,    // starred
        Tag,        // tag
        Search      // search results
    };
    Q_ENUM(FilterMode)

    struct FeedInfo {
        QString id;
        QString title;
        QString icon;      // Lucide icon name
        int unreadCount = 0;
    };

    struct ArticleInfo {
        QString id;
        QString feedId;
        QString title;
        QString summary;
        QString author;
        QDateTime pubDate;
        bool unread = true;
        bool starred = false;
        QStringList tags;
    };

    explicit NavigationContext(QObject* parent = nullptr);
    ~NavigationContext() override = default;

    // Current navigation level
    Level currentLevel() const { return level_; }

    // Level 1: select feed
    void selectFeed(const QString& feedId);
    void selectFilterMode(FilterMode mode, const QString& param = QString()); // for Tag mode param=tagId

    // Level 2: select article
    void selectArticle(const QString& articleId);

    // Level 3: reading detail
    void enterReading(const QString& articleId);
    void leaveReading(); // back to article list

    // Breadcrumb/nav stack (for command palette / title bar)
    QString breadcrumb() const;

    // Currently selected feed
    QString currentFeedId() const { return currentFeedId_; }
    FeedInfo currentFeed() const;

    // Current filter mode
    FilterMode currentFilterMode() const { return filterMode_; }
    QString currentFilterParam() const { return filterParam_; }

    // Article list data source (injected by external Model; only IDs stored here)
    void setArticleIds(const QVector<QString>& ids);
    QVector<QString> articleIds() const { return articleIds_; }

    // Unread/starred count updates
    void updateUnreadCount(const QString& feedId, int count);
    void updateStarredCount(const QString& feedId, int count);

signals:
    // Navigation changes
    void levelChanged(Level level);
    void feedSelected(const QString& feedId);
    void filterChanged(FilterMode mode, const QString& param);
    void articleSelected(const QString& articleId);
    void readingEntered(const QString& articleId);
    void readingLeft();

    // Breadcrumb update
    void breadcrumbChanged(const QString& breadcrumb);

    // Count changes (for NavRail/left tree badges)
    void unreadCountChanged(const QString& feedId, int count);
    void starredCountChanged(const QString& feedId, int count);

private:
    Level level_ = Level::FeedList;
    FilterMode filterMode_ = FilterMode::All;
    QString filterParam_;
    QString currentFeedId_;
    QString currentArticleId_;
    QVector<QString> articleIds_;
    QHash<QString, FeedInfo> feedInfos_;
    QHash<QString, ArticleInfo> articleInfos_;
};

#endif // NAVIGATIONCONTEXT_H
/* ============================================================
 * QProcess NavigationContext
 * 两级导航状态管理：左侧选中订阅源 -> 中间列表过滤 + 顶部标题
 *                            中间选中文章 -> 右侧阅读区 + 返回按钮
 * 版本：v1.4（对应报告 §13.2, §4.2, §4.3）
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
        FeedList,   // 一级：订阅源列表（左侧树）
        ArticleList,// 二级：文章列表（中间）
        Reading     // 三级：阅读详情（右侧/全屏）
    };
    Q_ENUM(Level)

    enum class FilterMode {
        All,        // 全部文章
        Unread,     // 未读
        Starred,    // 星标
        Tag,        // 标签
        Search      // 搜索结果
    };
    Q_ENUM(FilterMode)

    struct FeedInfo {
        QString id;
        QString title;
        QString icon;      // Lucide 图标名
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

    // 当前导航层级
    Level currentLevel() const { return level_; }

    // 一级：选中订阅源
    void selectFeed(const QString& feedId);
    void selectFilterMode(FilterMode mode, const QString& param = QString()); // Tag时param=tagId

    // 二级：选中文章
    void selectArticle(const QString& articleId);

    // 三级：阅读详情
    void enterReading(const QString& articleId);
    void leaveReading(); // 返回文章列表

    // 面包屑/导航栈（用于命令面板/标题栏显示）
    QString breadcrumb() const;

    // 当前选中的 Feed
    QString currentFeedId() const { return currentFeedId_; }
    FeedInfo currentFeed() const;

    // 当前过滤模式
    FilterMode currentFilterMode() const { return filterMode_; }
    QString currentFilterParam() const { return filterParam_; }

    // 文章列表数据源（由外部 Model 注入，这里只存 ID 列表）
    void setArticleIds(const QVector<QString>& ids);
    QVector<QString> articleIds() const { return articleIds_; }

    // 未读/星标计数更新
    void updateUnreadCount(const QString& feedId, int count);
    void updateStarredCount(const QString& feedId, int count);

signals:
    // 导航变化
    void levelChanged(Level level);
    void feedSelected(const QString& feedId);
    void filterChanged(FilterMode mode, const QString& param);
    void articleSelected(const QString& articleId);
    void readingEntered(const QString& articleId);
    void readingLeft();

    // 面包屑更新
    void breadcrumbChanged(const QString& breadcrumb);

    // 计数变化（供 NavRail/左侧树更新 badge）
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
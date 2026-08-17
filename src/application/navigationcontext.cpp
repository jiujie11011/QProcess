/* ============================================================
 * QProcess NavigationContext - 实现（骨架版）
 * 版本：v1.4
 * ============================================================ */
#include "navigationcontext.h"
#include <QDebug>

NavigationContext::NavigationContext(QObject* parent)
    : QObject(parent)
{
}

void NavigationContext::selectFeed(const QString& feedId)
{
    if (currentFeedId_ == feedId && level_ == Level::ArticleList) return;

    currentFeedId_ = feedId;
    level_ = Level::ArticleList;
    currentArticleId_.clear();

    // 重置为全文模式，外部会根据 feedId 重新查询文章列表
    filterMode_ = FilterMode::All;
    filterParam_.clear();

    emit levelChanged(level_);
    emit feedSelected(feedId);
    emit breadcrumbChanged(breadcrumb());
}

void NavigationContext::selectFilterMode(FilterMode mode, const QString& param)
{
    filterMode_ = mode;
    filterParam_ = param;
    currentArticleId_.clear();

    emit filterChanged(mode, param);
    emit breadcrumbChanged(breadcrumb());
}

void NavigationContext::selectArticle(const QString& articleId)
{
    if (currentArticleId_ == articleId) return;

    currentArticleId_ = articleId;
    level_ = Level::Reading;

    emit articleSelected(articleId);
    emit readingEntered(articleId);
    emit breadcrumbChanged(breadcrumb());
}

void NavigationContext::enterReading(const QString& articleId)
{
    selectArticle(articleId);
}

void NavigationContext::leaveReading()
{
    if (level_ != Level::Reading) return;

    level_ = Level::ArticleList;
    currentArticleId_.clear();

    emit readingLeft();
    emit levelChanged(level_);
    emit breadcrumbChanged(breadcrumb());
}

QString NavigationContext::breadcrumb() const
{
    QStringList parts;

    if (!currentFeedId_.isEmpty()) {
        auto it = feedInfos_.find(currentFeedId_);
        if (it != feedInfos_.end()) {
            parts << it->title;
        } else {
            parts << "订阅源";
        }
    }

    if (filterMode_ != FilterMode::All) {
        switch (filterMode_) {
            case FilterMode::Unread: parts << "未读"; break;
            case FilterMode::Starred: parts << "星标"; break;
            case FilterMode::Tag: parts << QString("标签:%1").arg(filterParam_); break;
            case FilterMode::Search: parts << QString("搜索:%1").arg(filterParam_); break;
            default: break;
        }
    } else {
        parts << "文章列表";
    }

    if (!currentArticleId_.isEmpty()) {
        auto it = articleInfos_.find(currentArticleId_);
        if (it != articleInfos_.end()) {
            parts << it->title.left(30);
        } else {
            parts << "阅读详情";
        }
    }

    return parts.join(" / ");
}

NavigationContext::FeedInfo NavigationContext::currentFeed() const
{
    auto it = feedInfos_.find(currentFeedId_);
    return it != feedInfos_.end() ? *it : FeedInfo{};
}

void NavigationContext::setArticleIds(const QVector<QString>& ids)
{
    articleIds_ = ids;
}

void NavigationContext::updateUnreadCount(const QString& feedId, int count)
{
    if (feedInfos_.contains(feedId)) {
        feedInfos_[feedId].unreadCount = count;
        emit unreadCountChanged(feedId, count);
    }
}

void NavigationContext::updateStarredCount(const QString& feedId, int count)
{
    if (feedInfos_.contains(feedId)) {
        feedInfos_[feedId].unreadCount = count; // 复用字段或另加 starredCount
        emit starredCountChanged(feedId, count);
    }
}

#include "moc_navigationcontext.cpp"
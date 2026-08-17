/* ============================================================
 * QProcess NewsCardDelegate
 * News list card delegate: V2/V3 two visual levels, micro-interactions
 * Version: v1.4 (report section 6.4, 11.1-11.3)
 * ============================================================ */
#ifndef NEWSCARDDELEGATE_H
#define NEWSCARDDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QRect>
#include <QSize>
#include <QDateTime>
#include <QStringList>

class NewsCardDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    enum class VisualLevel {
        V1_List,    // compact list (single line, title + source)
        V2_Card,    // card (multi-line, title + summary + meta, default)
        V3_Featured // featured card (pinned/featured, big image + full summary)
    };
    Q_ENUM(VisualLevel)

    struct ArticleData {
        QString id;
        QString title;
        QString summary;
        QString feedId;
        QString feedTitle;
        QString feedIcon;      // Lucide icon name
        QDateTime pubDate;
        bool unread = true;
        bool starred = false;
        QStringList tags;
        QString imageUrl;      // thumbnail (used by V3)
        int readTime = 0;      // minutes
    };

    explicit NewsCardDelegate(QObject* parent = nullptr);
    ~NewsCardDelegate() override = default;

    // Set visual level
    void setVisualLevel(VisualLevel level) { visualLevel_ = level; }
    VisualLevel visualLevel() const { return visualLevel_; }

    // Set data (injected by external Model via setItemData)
    static void setArticleData(QAbstractItemModel* model, const QModelIndex& index,
                               const ArticleData& data);

    static ArticleData articleData(const QModelIndex& index);

    // Card size calculation
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    // Painting
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    // Editor (not needed)
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override { return nullptr; }

signals:
    // Card button clicks (forwarded from mouse events in paint)
    void starClicked(const QString& articleId);
    void tagClicked(const QString& tag);
    void feedClicked(const QString& feedId);

protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

private:
    VisualLevel visualLevel_ = VisualLevel::V2_Card;

    // Layout constants (scaled by tokens)
    int cardMargin_ = 12;
    int cardSpacing_ = 8;
    int cardRadius_ = 8;
    int imageHeight_ = 140; // used by V3

    // Drawing helpers
    void drawCardBackground(QPainter* painter, const QRect& rect,
                            const QStyleOptionViewItem& option) const;
    void drawV1(QPainter* painter, const QRect& rect,
                const ArticleData& data, const QStyleOptionViewItem& option) const;
    void drawV2(QPainter* painter, const QRect& rect,
                const ArticleData& data, const QStyleOptionViewItem& option) const;
    void drawV3(QPainter* painter, const QRect& rect,
                const ArticleData& data, const QStyleOptionViewItem& option) const;

    // Hit area detection
    enum class HitArea { None, Star, Tag, Feed, Title, Summary };
    HitArea hitTest(const QPoint& pos, const QRect& cardRect,
                    const ArticleData& data) const;
};

// Used for QVariant serialization in setArticleData/articleData.
// Qt5 QVariant::fromValue/value<T>/canConvert<T> depend on Q_DECLARE_METATYPE;
// missing it causes static_assert compile failure (unrelated to Qt6, keep it).
Q_DECLARE_METATYPE(NewsCardDelegate::ArticleData)

#endif // NEWSCARDDELEGATE_H
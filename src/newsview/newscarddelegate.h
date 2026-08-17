/* ============================================================
 * QProcess NewsCardDelegate
 * 文章列表卡片化代理：V2/V3 两级视觉层级、微交互
 * 版本：v1.4（对应报告 §6.4, §11.1–11.3）
 * ============================================================ */
#ifndef NEWSCARDDELEGATE_H
#define NEWSCARDDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QRect>
#include <QSize>

class NewsCardDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    enum class VisualLevel {
        V1_List,    // 紧凑列表（单行，仅标题+来源）
        V2_Card,    // 卡片（多行，标题+摘要+元信息，默认）
        V3_Featured // 特大卡片（置顶/精选，大图+完整摘要）
    };
    Q_ENUM(VisualLevel)

    struct ArticleData {
        QString id;
        QString title;
        QString summary;
        QString feedTitle;
        QString feedIcon;      // Lucide 图标名
        QDateTime pubDate;
        bool unread = true;
        bool starred = false;
        QStringList tags;
        QString imageUrl;      // 缩略图（V3 用）
        int readTime = 0;      // 分钟
    };

    explicit NewsCardDelegate(QObject* parent = nullptr);
    ~NewsCardDelegate() override = default;

    // 设置视觉层级
    void setVisualLevel(VisualLevel level) { visualLevel_ = level; }
    VisualLevel visualLevel() const { return visualLevel_; }

    // 设置数据（由外部 Model 通过 setItemData 注入）
    static void setArticleData(QAbstractItemModel* model, const QModelIndex& index,
                               const ArticleData& data);

    static ArticleData articleData(const QModelIndex& index);

    // 卡片尺寸计算
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    // 绘制
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    // 编辑器（不需要）
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override { return nullptr; }

signals:
    // 卡片内按钮点击（由 paint 中的鼠标事件转发）
    void starClicked(const QString& articleId);
    void tagClicked(const QString& tag);
    void feedClicked(const QString& feedId);

protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) const override;

private:
    VisualLevel visualLevel_ = VisualLevel::V2_Card;

    // 布局常量（按 tokens 缩放）
    int cardMargin_ = 12;
    int cardSpacing_ = 8;
    int cardRadius_ = 8;
    int imageHeight_ = 140; // V3 用

    // 绘制辅助
    void drawCardBackground(QPainter* painter, const QRect& rect,
                            const QStyleOptionViewItem& option) const;
    void drawV1(QPainter* painter, const QRect& rect,
                const ArticleData& data, const QStyleOptionViewItem& option) const;
    void drawV2(QPainter* painter, const QRect& rect,
                const ArticleData& data, const QStyleOptionViewItem& option) const;
    void drawV3(QPainter* painter, const QRect& rect,
                const ArticleData& data, const QStyleOptionViewItem& option) const;

    // 点击区域检测
    enum class HitArea { None, Star, Tag, Feed, Title, Summary };
    HitArea hitTest(const QPoint& pos, const QRect& cardRect,
                    const ArticleData& data) const;
};

#endif // NEWSCARDDELEGATE_H
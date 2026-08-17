/* ============================================================
 * QProcess NewsCardDelegate - 实现（骨架版）
 * 版本：v1.4
 * ============================================================ */
#include "newscarddelegate.h"
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QApplication>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QVariant>
#include <QDebug>

NewsCardDelegate::NewsCardDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void NewsCardDelegate::setArticleData(QAbstractItemModel* model, const QModelIndex& index,
                                      const ArticleData& data)
{
    if (!model || !index.isValid()) return;
    // 使用 Qt::UserRole 存储自定义数据
    model->setData(index, QVariant::fromValue(data), Qt::UserRole);
}

NewsCardDelegate::ArticleData NewsCardDelegate::articleData(const QModelIndex& index)
{
    if (!index.isValid()) return ArticleData{};
    QVariant v = index.data(Qt::UserRole);
    if (v.canConvert<ArticleData>()) {
        return v.value<ArticleData>();
    }
    return ArticleData{};
}

QSize NewsCardDelegate::sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    ArticleData data = articleData(index);
    if (data.id.isEmpty()) return QStyledItemDelegate::sizeHint(option, index);

    int width = option.rect.width();
    int height = 0;

    switch (visualLevel_) {
        case VisualLevel::V1_List:
            height = 40; // 紧凑单行
            break;
        case VisualLevel::V2_Card:
            height = 120; // 标准卡片
            break;
        case VisualLevel::V3_Featured:
            height = 280; // 特大卡片（含图片）
            break;
    }
    return QSize(width, height);
}

void NewsCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    ArticleData data = articleData(index);
    if (data.id.isEmpty()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QRect rect = option.rect.adjusted(cardMargin_, cardMargin_, -cardMargin_, -cardMargin_);

    // 保存状态
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    // 绘制卡片背景
    drawCardBackground(painter, rect, option);

    // 根据视觉层级绘制内容
    switch (visualLevel_) {
        case VisualLevel::V1_List:
            drawV1(painter, rect, data, option);
            break;
        case VisualLevel::V2_Card:
            drawV2(painter, rect, data, option);
            break;
        case VisualLevel::V3_Featured:
            drawV3(painter, rect, data, option);
            break;
    }

    painter->restore();
}

void NewsCardDelegate::drawCardBackground(QPainter* painter, const QRect& rect,
                                          const QStyleOptionViewItem& option) const
{
    QColor bgColor;
    if (option.state & QStyle::State_Selected) {
        bgColor = QColor("#4C8DFF"); // accent
        bgColor.setAlpha(30); // 12%
    } else if (option.state & QStyle::State_MouseOver) {
        bgColor = QColor("#2A2A2E"); // bgHover
    } else {
        bgColor = QColor("#202024"); // bgSurface
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(rect, cardRadius_, cardRadius_);

    // 边框
    if (option.state & QStyle::State_Selected) {
        painter->setPen(QColor("#4C8DFF"));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rect.adjusted(0, 0, -1, -1), cardRadius_, cardRadius_);
    }
}

void NewsCardDelegate::drawV1(QPainter* painter, const QRect& rect,
                              const ArticleData& data,
                              const QStyleOptionViewItem& option) const
{
    // 紧凑单行：[未读点] 标题  来源  时间
    int x = rect.left() + 8;
    int y = rect.top();
    int h = rect.height();

    // 未读指示点
    if (data.unread) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#4C8DFF")); // statusUnread
        painter->drawEllipse(QPoint(x + 4, y + h/2), 4, 4);
        x += 14;
    } else {
        x += 8;
    }

    // 标题
    QFont titleFont = QApplication::font();
    titleFont.setPixelSize(14);
    titleFont.setWeight(QFont::Medium);
    painter->setFont(titleFont);
    painter->setPen(option.state & QStyle::State_Selected ? Qt::white : QColor("#E8E8EA"));

    QString title = data.title;
    QFontMetrics fm(titleFont);
    int titleWidth = rect.right() - x - 120; // 预留来源+时间空间
    title = fm.elidedText(title, Qt::ElideRight, titleWidth);
    painter->drawText(x, y, titleWidth, h, Qt::AlignVCenter | Qt::AlignLeft, title);

    // 来源 + 时间（右对齐）
    QFont metaFont = QApplication::font();
    metaFont.setPixelSize(12);
    painter->setFont(metaFont);
    painter->setPen(QColor("#A0A0AA")); // textSecondary

    QString meta = data.feedTitle + "  •  " + data.pubDate.toString("MM-dd hh:mm");
    painter->drawText(rect.right() - 110, y, 100, h, Qt::AlignVCenter | Qt::AlignRight, meta);

    // 星标
    if (data.starred) {
        painter->setPen(QColor("#F5B84D")); // statusStarred
        painter->drawText(rect.right() - 10, y, 20, h, Qt::AlignVCenter | Qt::AlignRight, "★");
    }
}

void NewsCardDelegate::drawV2(QPainter* painter, const QRect& rect,
                              const ArticleData& data,
                              const QStyleOptionViewItem& option) const
{
    // 标准卡片：
    // [未读点] 标题
    // 摘要（2行截断）
    // 底部：来源 • 时间 • 标签 • 星标 • 阅读时长

    int x = rect.left() + 12;
    int y = rect.top() + 10;
    int w = rect.width() - 24;

    // 未读点 + 标题
    if (data.unread) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#4C8DFF"));
        painter->drawEllipse(QPoint(x + 4, y + 8), 4, 4);
        x += 14;
    } else {
        x += 8;
    }

    QFont titleFont = QApplication::font();
    titleFont.setPixelSize(15);
    titleFont.setWeight(QFont::DemiBold);
    painter->setFont(titleFont);
    painter->setPen(option.state & QStyle::State_Selected ? Qt::white : QColor("#E8E8EA"));

    QFontMetrics fm(titleFont);
    QString title = fm.elidedText(data.title, Qt::ElideRight, w);
    painter->drawText(x, y, w, 24, Qt::AlignLeft, title);
    y += 26;

    // 摘要（2行）
    if (!data.summary.isEmpty()) {
        QFont summaryFont = QApplication::font();
        summaryFont.setPixelSize(13);
        painter->setFont(summaryFont);
        painter->setPen(QColor("#A0A0AA")); // textSecondary

        QFontMetrics sfm(summaryFont);
        QString summary = data.summary;
        // 简单截断到 2 行
        QStringList lines = summary.split('\n');
        QString firstLine = lines.first();
        int availW = w;
        firstLine = sfm.elidedText(firstLine, Qt::ElideRight, availW);
        painter->drawText(x, y, w, 20, Qt::AlignLeft, firstLine);
        y += 20;

        if (lines.size() > 1) {
            QString secondLine = lines[1];
            secondLine = sfm.elidedText(secondLine, Qt::ElideRight, availW);
            painter->drawText(x, y, w, 20, Qt::AlignLeft, secondLine);
            y += 20;
        } else {
            y += 16;
        }
    }

    // 底部元信息行
    y = rect.bottom() - 24;
    QFont metaFont = QApplication::font();
    metaFont.setPixelSize(11);
    painter->setFont(metaFont);
    painter->setPen(QColor("#7C7C84")); // textTertiary

    // 左侧：来源图标 + 来源名
    QString feedText = data.feedTitle;
    QFontMetrics mfm(metaFont);
    int feedW = mfm.horizontalAdvance(feedText);
    painter->drawText(x, y, feedW, 20, Qt::AlignVCenter | Qt::AlignLeft, feedText);

    // 右侧：时间 • 标签 • 星标 • 阅读时长
    QStringList metaParts;
    metaParts << data.pubDate.toString("MM-dd hh:mm");
    if (!data.tags.isEmpty()) metaParts << data.tags.first();
    if (data.readTime > 0) metaParts << QString("%1 min").arg(data.readTime);
    if (data.starred) metaParts << "★";

    QString metaText = metaParts.join("  •  ");
    int metaW = mfm.horizontalAdvance(metaText);
    painter->drawText(rect.right() - 12 - metaW, y, metaW, 20,
                      Qt::AlignVCenter | Qt::AlignRight, metaText);
}

void NewsCardDelegate::drawV3(QPainter* painter, const QRect& rect,
                              const ArticleData& data,
                              const QStyleOptionViewItem& option) const
{
    // 特大卡片：顶部大图 + 标题 + 完整摘要 + 底部元信息
    // 结构：
    // ┌─────────────────────────────────────┐
    // │  缩略图 (140px 高，全宽)              │
    // ├─────────────────────────────────────┤
    // │  [未读点] 标题                        │
    // │  完整摘要（多行）                     │
    // │  来源 • 时间 • 标签 • 星标 • 阅读时长   │
    // └─────────────────────────────────────┘

    int x = rect.left() + 12;
    int y = rect.top() + 12;
    int w = rect.width() - 24;

    // 缩略图区域
    if (!data.imageUrl.isEmpty()) {
        int imgH = imageHeight_;
        QRect imgRect(x, y, w, imgH);
        // TODO: 实际加载图片并绘制，这里画占位
        painter->setPen(QColor("#3F3F46"));
        painter->setBrush(QColor("#26262B"));
        painter->drawRoundedRect(imgRect, 6, 6);
        painter->drawText(imgRect, Qt::AlignCenter, "🖼 图片加载中...");
        y += imgH + 12;
    }

    // 标题
    QFont titleFont = QApplication::font();
    titleFont.setPixelSize(18);
    titleFont.setWeight(QFont::Bold);
    painter->setFont(titleFont);
    painter->setPen(option.state & QStyle::State_Selected ? Qt::white : QColor("#E8E8EA"));

    QFontMetrics fm(titleFont);
    QString title = fm.elidedText(data.title, Qt::ElideRight, w);
    painter->drawText(x, y, w, 28, Qt::AlignLeft, title);
    y += 32;

    // 完整摘要
    if (!data.summary.isEmpty()) {
        QFont summaryFont = QApplication::font();
        summaryFont.setPixelSize(14);
        painter->setFont(summaryFont);
        painter->setPen(QColor("#A0A0AA"));

        QFontMetrics sfm(summaryFont);
        QString summary = data.summary;
        // 绘制多行，最多 5 行
        QStringList lines = summary.split('\n');
        for (int i = 0; i < qMin(lines.size(), 5); ++i) {
            QString line = sfm.elidedText(lines[i], Qt::ElideRight, w);
            painter->drawText(x, y, w, 22, Qt::AlignLeft, line);
            y += 22;
        }
        y += 8;
    }

    // 底部元信息
    QFont metaFont = QApplication::font();
    metaFont.setPixelSize(12);
    painter->setFont(metaFont);
    painter->setPen(QColor("#7C7C84"));

    QStringList metaParts;
    metaParts << data.feedTitle;
    metaParts << data.pubDate.toString("yyyy-MM-dd hh:mm");
    if (!data.tags.isEmpty()) metaParts << data.tags.join(", ");
    if (data.readTime > 0) metaParts << QString("%1 min read").arg(data.readTime);
    if (data.starred) metaParts << "★";

    QString metaText = metaParts.join("  •  ");
    painter->drawText(x, y, w, 20, Qt::AlignLeft, metaText);
}

bool NewsCardDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(event);
        ArticleData data = articleData(index);
        if (data.id.isEmpty()) return false;

        HitArea area = hitTest(me->pos(), option.rect, data);
        switch (area) {
            case HitArea::Star:
                emit starClicked(data.id);
                return true;
            case HitArea::Tag:
                if (!data.tags.isEmpty()) emit tagClicked(data.tags.first());
                return true;
            case HitArea::Feed:
                emit feedClicked(data.feedId);
                return true;
            default:
                break;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

NewsCardDelegate::HitArea NewsCardDelegate::hitTest(const QPoint& pos, const QRect& cardRect,
                                                     const ArticleData& data) const
{
    // 简化：根据视觉层级和位置判断
    // 实际应根据 paint 时的布局精确计算
    QRect rect = cardRect.adjusted(cardMargin_, cardMargin_, -cardMargin_, -cardMargin_);

    // 右侧区域：星标/标签/来源
    int rightZone = rect.right() - 120;
    if (pos.x() > rightZone) {
        if (data.starred) return HitArea::Star;
        if (!data.tags.isEmpty()) return HitArea::Tag;
        return HitArea::Feed;
    }

    // 标题区域
    if (pos.y() < rect.top() + 40) return HitArea::Title;

    return HitArea::None;
}


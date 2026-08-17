/* ============================================================
 * Quill - unit tests for NewsCardDelegate
 *
 * 重点回归：feedId 字段曾出现"结构体字段被当方法调用"
 * （data.feedId() -> no match for call to (QString)）的笔误，
 * 本套件通过 setArticleData/articleData 往返验证字段级一致性。
 * ============================================================ */
#include <QtTest>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>

#include "tst_newscarddelegate.h"
#include "newscarddelegate.h"

void TestNewsCardDelegate::roundTripPreservesAllFields()
{
    QStandardItemModel model(1, 1);
    const QModelIndex idx = model.index(0, 0);

    NewsCardDelegate::ArticleData in;
    in.id = QStringLiteral("art-42");
    in.title = QStringLiteral("Hello World");
    in.summary = QStringLiteral("summary text");
    in.feedId = QStringLiteral("feed-7");
    in.feedTitle = QStringLiteral("Tech News");
    in.feedIcon = QStringLiteral("house");
    in.pubDate = QDateTime(QDate(2026, 8, 17), QTime(10, 30));
    in.unread = true;
    in.starred = true;
    in.tags = QStringList() << QStringLiteral("qt") << QStringLiteral("rss");
    in.imageUrl = QStringLiteral("https://example.com/a.png");
    in.readTime = 5;

    NewsCardDelegate::setArticleData(&model, idx, in);
    const NewsCardDelegate::ArticleData out =
        NewsCardDelegate::articleData(idx);

    QCOMPARE(out.id, in.id);
    QCOMPARE(out.title, in.title);
    QCOMPARE(out.summary, in.summary);
    QCOMPARE(out.feedId, in.feedId);
    QCOMPARE(out.feedTitle, in.feedTitle);
    QCOMPARE(out.feedIcon, in.feedIcon);
    QCOMPARE(out.pubDate, in.pubDate);
    QCOMPARE(out.unread, in.unread);
    QCOMPARE(out.starred, in.starred);
    QCOMPARE(out.tags, in.tags);
    QCOMPARE(out.imageUrl, in.imageUrl);
    QCOMPARE(out.readTime, in.readTime);
}

void TestNewsCardDelegate::feedIdRoundTrip()
{
    QStandardItemModel model(1, 1);
    const QModelIndex idx = model.index(0, 0);

    NewsCardDelegate::ArticleData in;
    in.feedId = QStringLiteral("feed-abc-123");

    NewsCardDelegate::setArticleData(&model, idx, in);
    QCOMPARE(NewsCardDelegate::articleData(idx).feedId,
             QStringLiteral("feed-abc-123"));
}

void TestNewsCardDelegate::emptyDataDefaults()
{
    QStandardItemModel model(1, 1);
    const QModelIndex idx = model.index(0, 0);

    NewsCardDelegate::ArticleData in;
    NewsCardDelegate::setArticleData(&model, idx, in);

    const NewsCardDelegate::ArticleData out =
        NewsCardDelegate::articleData(idx);
    QCOMPARE(out.id, QString());
    QVERIFY(out.unread);
    QVERIFY(!out.starred);
    QCOMPARE(out.tags.size(), 0);
    QCOMPARE(out.readTime, 0);
}

void TestNewsCardDelegate::visualLevelAccessors()
{
    NewsCardDelegate del;
    QCOMPARE(int(del.visualLevel()),
             int(NewsCardDelegate::VisualLevel::V2_Card));

    del.setVisualLevel(NewsCardDelegate::VisualLevel::V3_Featured);
    QCOMPARE(int(del.visualLevel()),
             int(NewsCardDelegate::VisualLevel::V3_Featured));

    del.setVisualLevel(NewsCardDelegate::VisualLevel::V1_List);
    QCOMPARE(int(del.visualLevel()),
             int(NewsCardDelegate::VisualLevel::V1_List));
}

void TestNewsCardDelegate::sizeHintNeverNegative()
{
    NewsCardDelegate del;
    QStandardItemModel model(1, 1);
    const QModelIndex idx = model.index(0, 0);

    QStyleOptionViewItem opt;
    const QSize s = del.sizeHint(opt, idx);
    QVERIFY(s.width() > 0);
    QVERIFY(s.height() > 0);
}

/* ============================================================
 * QProcess CommandPalette
 * Ctrl+K 全局命令面板：搜文章/订阅源/命令三合一
 * 版本：v1.4（对应报告 §4.3, §8.2）
 * ============================================================ */
#ifndef COMMANDPALETTE_H
#define COMMANDPALETTE_H

#include <QFrame>
#include <QLineEdit>
#include <QListView>
#include <QStringListModel>

class CommandPalette : public QFrame
{
    Q_OBJECT
public:
    enum class ResultType {
        Article,      // 文章标题（FTS 搜索）
        Feed,         // 订阅源
        Command       // 动作命令
    };

    struct Result {
        ResultType type;
        QString id;         // articleId / feedId / commandId
        QString title;      // 显示标题
        QString subtitle;   // 副标题（来源/分类/快捷键）
        QIcon icon;
    };

    explicit CommandPalette(QWidget* parent = nullptr);
    ~CommandPalette() override = default;

    // 显示面板（居中弹出）
    void showPalette();

    // 隐藏面板
    void hidePalette();

    // 设置数据源（由外部注入）
    void setArticleResults(const QList<Result>& results);
    void setFeedResults(const QList<Result>& results);
    void setCommandResults(const QList<Result>& results);

signals:
    void articleSelected(const QString& articleId);
    void feedSelected(const QString& feedId);
    void commandExecuted(const QString& commandId);
    void closed();

private:
    void setupUI();
    void filterResults(const QString& text);
    void selectCurrent();

    bool eventFilter(QObject* watched, QEvent* event) override;

    QLineEdit* input_;
    QListView* list_;
    QStringListModel* model_;
    QList<Result> allResults_;
};

#endif // COMMANDPALETTE_H
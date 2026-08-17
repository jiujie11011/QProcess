/* ============================================================
 * QProcess CommandPalette
 * Ctrl+K global command palette: search articles/feeds/commands
 * Version: v1.4 (report section 4.3, 8.2)
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
        Article,      // article title (FTS search)
        Feed,         // feed
        Command       // action command
    };

    struct Result {
        ResultType type;
        QString id;         // articleId / feedId / commandId
        QString title;      // display title
        QString subtitle;   // subtitle (source/category/shortcut)
        QIcon icon;
    };

    explicit CommandPalette(QWidget* parent = nullptr);
    ~CommandPalette() override = default;

    // Show the palette (centered popup)
    void showPalette();

    // Hide the palette
    void hidePalette();

    // Set data sources (injected externally)
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
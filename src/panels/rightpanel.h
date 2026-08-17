/* ============================================================
 * QProcess RightPanel
 * 右侧可折叠面板：AI 摘要 / 文章详情 / Diff 预览 / 终端
 * 版本：v1.4（对应报告 §4.2, §4.3, §6.3）
 * ============================================================ */
#ifndef RIGHTPANEL_H
#define RIGHTPANEL_H

#include <QFrame>
#include <QStackedWidget>
#include <QToolButton>
#include <QString>
#include <QDateTime>
#include <QStringList>

class QLabel;
class QScrollArea;
class QButtonGroup;

class RightPanel : public QFrame
{
    Q_OBJECT
public:
    enum class Mode {
        Empty,      // 空白
        Summary,    // AI 摘要
        Details,    // 文章详情/元数据
        Diff,       // Diff 预览（对比版本）
        Terminal    // 内嵌终端（未来扩展）
    };
    Q_ENUM(Mode)

    explicit RightPanel(QWidget* parent = nullptr);
    ~RightPanel() override = default;

    // 切换模式
    void setMode(Mode mode);
    Mode currentMode() const { return currentMode_; }

    // 设置内容（由外部注入）
    void setSummary(const QString& html);
    void setDetails(const QString& feedTitle, const QString& articleTitle,
                    const QString& author, const QDateTime& pubDate,
                    const QStringList& tags, int wordCount, int readTime);
    void setDiff(const QString& oldHtml, const QString& newHtml);

    // 展开/折叠
    void setExpanded(bool expanded);
    bool isExpanded() const { return expanded_; }

    // 最小/最大宽度（配合 SplitterHandle 约束）
    void setWidthConstraints(int minPx, int maxPx);

signals:
    void modeChanged(Mode mode);
    void expandedChanged(bool expanded);
    void widthChanged(int width);
    void copyRequested(const QString& text);
    void openInBrowserRequested(const QString& url);

private:
    void setupUI();
    void setupSummaryPage();
    void setupDetailsPage();
    void setupDiffPage();
    void updatePage();

    Mode currentMode_ = Mode::Empty;
    bool expanded_ = true;
    int minWidth_ = 200;
    int maxWidth_ = 700;

    QStackedWidget* stack_;
    QLabel* emptyPage_;
    QWidget* summaryPage_;
    QScrollArea* detailsPage_;
    QWidget* diffPage_;
    QLabel* terminalPage_;

    // Toolbar（顶部标签栏）
    QWidget* toolbar_;
    QButtonGroup* modeButtons_;
};

#endif // RIGHTPANEL_H
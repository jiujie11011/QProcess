/* ============================================================
 * QProcess ReaderToolbar
 * 阅读区顶部工具栏：已读/收藏/原文/分享/更多菜单
 * 版本：v1.4（对应报告 §13.3, §13.4）
 * ============================================================ */
#ifndef READERTOOLBAR_H
#define READERTOOLBAR_H

#include <QWidget>
#include <QAction>
#include <QMenu>

class ReaderToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit ReaderToolbar(QWidget* parent = nullptr);
    ~ReaderToolbar() override = default;

    // 设置当前文章状态（由外部调用，如文章切换时）
    void setArticleState(bool unread, bool starred);

    // 悬停显示策略：默认只显示"更多"，悬停时展开
    void setHoverExpand(bool expand);

signals:
    void markReadRequested(bool read);
    void toggleStarredRequested(bool starred);
    void openOriginalRequested();
    void shareRequested();
    void openInNewTabRequested();
    void copyLinkRequested();
    void copyTitleRequested();
    void exportPdfRequested();
    void ttsRequested();
    void customizeToolbarRequested();

private:
    void setupActions();
    void setupMenu();
    void setupLayout();
    void updateIcons();

    // 动作
    QAction* actMarkRead_;
    QAction* actStarred_;
    QAction* actOpenOriginal_;
    QAction* actShare_;
    QAction* actMore_;
    QMenu* moreMenu_;

    // 内部状态
    bool currentUnread_ = true;
    bool currentStarred_ = false;
    bool hoverExpand_ = false;

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
};

#endif // READERTOOLBAR_H
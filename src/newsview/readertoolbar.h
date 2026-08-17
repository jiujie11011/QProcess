/* ============================================================
 * QProcess ReaderToolbar
 * Reader top toolbar: read/starred/original/share/more menu
 * Version: v1.4 (report section 13.3, 13.4)
 * ============================================================ */
#ifndef READERTOOLBAR_H
#define READERTOOLBAR_H

#include <QWidget>
#include <QAction>
#include <QMenu>
#include <QEvent>
#if defined(QT6)
#include <QEnterEvent>
#endif

class ReaderToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit ReaderToolbar(QWidget* parent = nullptr);
    ~ReaderToolbar() override = default;

    // Set current article state (called externally, e.g. on article switch)
    void setArticleState(bool unread, bool starred);

    // Hover reveal policy: by default only "More" shows, expands on hover
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

    // actions
    QAction* actMarkRead_;
    QAction* actStarred_;
    QAction* actOpenOriginal_;
    QAction* actShare_;
    QAction* actMore_;
    QMenu* moreMenu_;

    // internal state
    bool currentUnread_ = true;
    bool currentStarred_ = false;
    bool hoverExpand_ = false;

protected:
#if defined(QT6)
    void enterEvent(QEnterEvent* event) override;
#else
    void enterEvent(QEvent* event) override;
#endif
    void leaveEvent(QEvent* event) override;
};

#endif // READERTOOLBAR_H
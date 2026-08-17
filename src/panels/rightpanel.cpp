/* ============================================================
 * QProcess RightPanel - 实现（骨架版）
 * 版本：v1.4
 * ============================================================ */
#include "rightpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QToolButton>
#include <QButtonGroup>
#include <QScrollArea>
#include <QSplitter>
#include <QDateTime>
#include <QDebug>

RightPanel::RightPanel(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("rightPanel");
    setFrameShape(QFrame::StyledPanel);
    setLineWidth(1);
    setMinimumWidth(minWidth_);
    setMaximumWidth(maxWidth_);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    setupUI();
}

void RightPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部工具栏：模式切换
    toolbar_ = new QWidget;
    toolbar_->setObjectName("rightPanelToolbar");
    toolbar_->setFixedHeight(36);
    auto* toolLayout = new QHBoxLayout(toolbar_);
    toolLayout->setContentsMargins(8, 0, 8, 0);
    toolLayout->setSpacing(4);

    modeButtons_ = new QButtonGroup(this);
    modeButtons_->setExclusive(true);

    auto makeModeBtn = [this](Mode mode, const QString& text, const QString& tooltip) {
        auto* btn = new QToolButton;
        btn->setCheckable(true);
        btn->setText(text);
        btn->setToolTip(tooltip);
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setFixedHeight(28);
        btn->setCursor(Qt::PointingHandCursor);
        modeButtons_->addButton(btn, static_cast<int>(mode));
        toolLayout->addWidget(btn);
        return btn;
    };

    makeModeBtn(Mode::Summary, "摘要", "AI 摘要");
    makeModeBtn(Mode::Details, "详情", "文章详情");
    makeModeBtn(Mode::Diff, "对比", "版本对比");
    makeModeBtn(Mode::Terminal, "终端", "内嵌终端");

    toolLayout->addStretch();

    // 折叠/展开按钮
    QToolButton* btnCollapse = new QToolButton;
    btnCollapse->setText("◀");
    btnCollapse->setToolTip("折叠面板");
    btnCollapse->setFixedSize(28, 28);
    btnCollapse->setCursor(Qt::PointingHandCursor);
    connect(btnCollapse, &QToolButton::clicked, this, [this] {
        setExpanded(!expanded_);
    });
    toolLayout->addWidget(btnCollapse);

    mainLayout->addWidget(toolbar_);

    // 内容堆栈
    stack_ = new QStackedWidget;
    stack_->setObjectName("rightPanelStack");

    // 空白页
    emptyPage_ = new QLabel("选择文章查看详情");
    emptyPage_->setAlignment(Qt::AlignCenter);
    emptyPage_->setObjectName("rightPanelEmpty");
    stack_->addWidget(emptyPage_);

    // 摘要页
    setupSummaryPage();
    stack_->addWidget(summaryPage_);

    // 详情页
    setupDetailsPage();
    stack_->addWidget(detailsPage_);

    // Diff 页
    setupDiffPage();
    stack_->addWidget(diffPage_);

    // 终端页（占位）
    terminalPage_ = new QLabel("终端功能待实现");
    terminalPage_->setAlignment(Qt::AlignCenter);
    stack_->addWidget(terminalPage_);

    mainLayout->addWidget(stack_, 1);

    connect(modeButtons_, QOverload<int>::of(&QButtonGroup::idClicked),
            this, [this](int id) { setMode(static_cast<Mode>(id)); });
}

void RightPanel::setupSummaryPage()
{
    summaryPage_ = new QWidget;
    auto* layout = new QVBoxLayout(summaryPage_);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QLabel* title = new QLabel("AI 摘要");
    title->setObjectName("rightPanelSectionTitle");
    layout->addWidget(title);

    QTextBrowser* browser = new QTextBrowser;
    browser->setObjectName("rightPanelSummaryBrowser");
    browser->setOpenExternalLinks(true);
    browser->setReadOnly(true);
    layout->addWidget(browser, 1);

    // 操作按钮
    auto* actions = new QHBoxLayout();
    QToolButton* btnCopy = new QToolButton;
    btnCopy->setText("复制");
    btnCopy->setCursor(Qt::PointingHandCursor);
    connect(btnCopy, &QToolButton::clicked, this, [browser] {
        emit copyRequested(browser->toPlainText());
    });
    actions->addStretch();
    actions->addWidget(btnCopy);
    layout->addLayout(actions);
}

void RightPanel::setupDetailsPage()
{
    detailsPage_ = new QScrollArea;
    detailsPage_->setWidgetResizable(true);
    detailsPage_->setFrameShape(QFrame::NoFrame);
    detailsPage_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);

    // 标题
    QLabel* title = new QLabel("文章详情");
    title->setObjectName("rightPanelSectionTitle");
    layout->addWidget(title);

    // 字段网格
    auto addField = [layout](const QString& label, const QString& value) {
        auto* row = new QWidget;
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);
        QLabel* l = new QLabel(label);
        l->setObjectName("detailLabel");
        l->setFixedWidth(80);
        QLabel* v = new QLabel(value);
        v->setObjectName("detailValue");
        v->setWordWrap(true);
        v->setTextInteractionFlags(Qt::TextSelectableByMouse);
        rowLayout->addWidget(l);
        rowLayout->addWidget(v, 1);
        layout->addWidget(row);
    };

    // 占位，运行时由 setDetails 填充
    addField("订阅源:", "—");
    addField("标题:", "—");
    addField("作者:", "—");
    addField("发布时间:", "—");
    addField("标签:", "—");
    addField("字数:", "—");
    addField("预估阅读:", "—");

    layout->addStretch();
    detailsPage_->setWidget(content);
}

void RightPanel::setupDiffPage()
{
    diffPage_ = new QWidget;
    auto* layout = new QVBoxLayout(diffPage_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QLabel* title = new QLabel("版本对比");
    title->setObjectName("rightPanelSectionTitle");
    title->setFixedHeight(36);
    layout->addWidget(title);

    // 左右分栏
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);

    QTextBrowser* oldBrowser = new QTextBrowser;
    oldBrowser->setObjectName("diffOldBrowser");
    oldBrowser->setReadOnly(true);
    oldBrowser->setLineWrapMode(QTextBrowser::WidgetWidth);

    QTextBrowser* newBrowser = new QTextBrowser;
    newBrowser->setObjectName("diffNewBrowser");
    newBrowser->setReadOnly(true);
    newBrowser->setLineWrapMode(QTextBrowser::WidgetWidth);

    splitter->addWidget(oldBrowser);
    splitter->addWidget(newBrowser);
    splitter->setSizes({500, 500});

    layout->addWidget(splitter, 1);
}

void RightPanel::setMode(Mode mode)
{
    if (currentMode_ == mode) return;
    currentMode_ = mode;
    updatePage();
    emit modeChanged(mode);
}

void RightPanel::setSummary(const QString& html)
{
    QTextBrowser* browser = summaryPage_->findChild<QTextBrowser*>("rightPanelSummaryBrowser");
    if (browser) browser->setHtml(html);
    if (currentMode_ == Mode::Summary) updatePage();
}

void RightPanel::setDetails(const QString& feedTitle, const QString& articleTitle,
                            const QString& author, const QDateTime& pubDate,
                            const QStringList& tags, int wordCount, int readTime)
{
    QWidget* content = detailsPage_->widget();
    if (!content) return;

    auto findLabel = [content](const QString& objName) -> QLabel* {
        return content->findChild<QLabel*>(objName);
    };

    // 这里需要更精确的查找，简化处理
    QList<QLabel*> labels = content->findChildren<QLabel*>("detailValue");
    if (labels.size() >= 7) {
        labels[0]->setText(feedTitle);
        labels[1]->setText(articleTitle);
        labels[2]->setText(author);
        labels[3]->setText(pubDate.toString("yyyy-MM-dd hh:mm"));
        labels[4]->setText(tags.join(", "));
        labels[5]->setText(QString::number(wordCount));
        labels[6]->setText(QString("%1 分钟").arg(readTime));
    }

    if (currentMode_ == Mode::Details) updatePage();
}

void RightPanel::setDiff(const QString& oldHtml, const QString& newHtml)
{
    QTextBrowser* oldBrowser = diffPage_->findChild<QTextBrowser*>("diffOldBrowser");
    QTextBrowser* newBrowser = diffPage_->findChild<QTextBrowser*>("diffNewBrowser");
    if (oldBrowser) oldBrowser->setHtml(oldHtml);
    if (newBrowser) newBrowser->setHtml(newHtml);
    if (currentMode_ == Mode::Diff) updatePage();
}

void RightPanel::setExpanded(bool expanded)
{
    if (expanded_ == expanded) return;
    expanded_ = expanded;
    setVisible(expanded);
    emit expandedChanged(expanded);
}

void RightPanel::setWidthConstraints(int minPx, int maxPx)
{
    minWidth_ = minPx;
    maxWidth_ = maxPx;
    setMinimumWidth(minWidth_);
    setMaximumWidth(maxWidth_);
}

void RightPanel::updatePage()
{
    int idx = static_cast<int>(currentMode_);
    if (idx >= 0 && idx < stack_->count()) {
        stack_->setCurrentIndex(idx);
    }
}

#include "moc_rightpanel.cpp"
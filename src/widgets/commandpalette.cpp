/* ============================================================
 * QProcess CommandPalette - 实现（骨架版）
 * 版本：v1.4
 * ============================================================ */
#include "commandpalette.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListView>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QDebug>

CommandPalette::CommandPalette(QWidget* parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    setObjectName("commandPalette");
    setFixedWidth(560);
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    setupUI();

    // 安装事件过滤器捕获 Esc 关闭
    installEventFilter(this);
}

void CommandPalette::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 输入框
    input_ = new QLineEdit;
    input_->setPlaceholderText("搜索文章、订阅源、命令... (Ctrl+K)");
    input_->setClearButtonEnabled(true);
    input_->setFixedHeight(44);
    input_->setObjectName("commandPaletteInput");
    connect(input_, &QLineEdit::textChanged, this, &CommandPalette::filterResults);

    // 列表
    list_ = new QListView;
    list_->setObjectName("commandPaletteList");
    list_->setUniformItemSizes(true);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    list_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list_->setMouseTracking(true);

    model_ = new QStringListModel(this);
    list_->setModel(model_);

    connect(list_, &QListView::clicked, this, [this](const QModelIndex& idx) {
        if (idx.isValid() && idx.row() < allResults_.size()) {
            const Result& r = allResults_[idx.row()];
            switch (r.type) {
                case ResultType::Article: emit articleSelected(r.id); break;
                case ResultType::Feed: emit feedSelected(r.id); break;
                case ResultType::Command: emit commandExecuted(r.id); break;
            }
            hidePalette();
        }
    });

    layout->addWidget(input_);
    layout->addWidget(list_, 1);
}

void CommandPalette::showPalette()
{
    // 居中显示在屏幕上方 15% 处
    if (QWidget* pw = parentWidget()) {
        QRect screenGeo = QApplication::primaryScreen()->geometry();
        int x = screenGeo.center().x() - width() / 2;
        int y = screenGeo.top() + screenGeo.height() * 0.15;
        move(x, y);
    }
    raise();
    show();
    input_->clear();
    input_->setFocus();
    QApplication::setActiveWindow(this);
}

void CommandPalette::hidePalette()
{
    hide();
    emit closed();
}

void CommandPalette::setArticleResults(const QList<Result>& results)
{
    // 合并到 allResults_ 并刷新
    for (const auto& r : results) allResults_.append(r);
    filterResults(input_->text());
}

void CommandPalette::setFeedResults(const QList<Result>& results)
{
    for (const auto& r : results) allResults_.append(r);
    filterResults(input_->text());
}

void CommandPalette::setCommandResults(const QList<Result>& results)
{
    for (const auto& r : results) allResults_.append(r);
    filterResults(input_->text());
}

void CommandPalette::filterResults(const QString& text)
{
    // TODO: 实现模糊匹配（可复用现有 pinyin 分词资产）
    // 当前简单过滤
    QStringList display;
    for (const auto& r : allResults_) {
        if (text.isEmpty() || r.title.contains(text, Qt::CaseInsensitive) ||
            r.subtitle.contains(text, Qt::CaseInsensitive)) {
            display << r.title + "  —  " + r.subtitle;
        }
    }
    model_->setStringList(display);
}

void CommandPalette::selectCurrent()
{
    // TODO: 键盘 ↑↓ 选择，Enter 执行
}

bool CommandPalette::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == this && event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) {
            hidePalette();
            return true;
        }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            selectCurrent();
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}

#include "moc_commandpalette.cpp"
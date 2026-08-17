/* ============================================================
 * QProcess PlayerBar - 实现（骨架版）
 * 版本：v1.4
 * ============================================================ */
#include "playerbar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QToolButton>
#include <QStyle>
#include <QTimer>
#include <QDebug>

PlayerBar::PlayerBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("playerBar");
    setFixedHeight(64);
    setVisible(false); // 按需出现

    setupUI();
    setupPlayer();
}

void PlayerBar::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(4);

    // 顶部：进度条
    progressBar_ = new QSlider(Qt::Horizontal);
    progressBar_->setObjectName("playerProgress");
    progressBar_->setRange(0, 0);
    progressBar_->setMouseTracking(true);
    connect(progressBar_, &QSlider::sliderPressed, this, [this] { userSeeking_ = true; });
    connect(progressBar_, &QSlider::sliderReleased, this, [this] {
        userSeeking_ = false;
        setPosition(progressBar_->value());
    });
    connect(progressBar_, &QSlider::sliderMoved, this, [this](int value) {
        // 拖拽时不发射 positionChanged，避免与 setPosition 冲突
    });

    // 底部：轨道信息 + 控制 + 扩展
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(12);

    // 轨道信息
    trackInfo_ = new QWidget;
    auto* trackLayout = new QHBoxLayout(trackInfo_);
    trackLayout->setContentsMargins(0, 0, 0, 0);
    trackLayout->setSpacing(8);

    QLabel* titleLabel = new QLabel("标题");
    titleLabel->setObjectName("playerTitle");
    titleLabel->setElideMode(Qt::ElideRight);
    QLabel* artistLabel = new QLabel("作者/来源");
    artistLabel->setObjectName("playerArtist");
    trackLayout->addWidget(titleLabel);
    trackLayout->addWidget(artistLabel);
    trackLayout->addStretch();

    // 控制按钮
    controls_ = new QWidget;
    auto* ctrlLayout = new QHBoxLayout(controls_);
    ctrlLayout->setContentsMargins(0, 0, 0, 0);
    ctrlLayout->setSpacing(8);

    auto makeBtn = [this](const QString& tooltip, const char* iconName) {
        auto* btn = new QToolButton;
        btn->setToolTip(tooltip);
        btn->setIconSize(QSize(24, 24));
        btn->setFixedSize(36, 36);
        btn->setAutoRaise(true);
        btn->setCursor(Qt::PointingHandCursor);
        // TODO: 使用 SvgIconEngine 加载 Lucide 图标
        btn->setText(iconName); // 占位
        return btn;
    };

    QToolButton* btnPrev = makeBtn("上一首", "⏮");
    QToolButton* btnPlay = makeBtn("播放/暂停", "▶");
    QToolButton* btnNext = makeBtn("下一首", "⏭");
    QToolButton* btnStop = makeBtn("停止", "⏹");

    connect(btnPlay, &QToolButton::clicked, this, [this] {
        if (player_->playbackState() == QMediaPlayer::PlayingState)
            pause();
        else
            resume();
    });
    connect(btnStop, &QToolButton::clicked, this, &PlayerBar::stop);
    connect(btnPrev, &QToolButton::clicked, this, [] { /* TODO: 播放列表上一首 */ });
    connect(btnNext, &QToolButton::clicked, this, [] { /* TODO: 播放列表下一首 */ });

    ctrlLayout->addWidget(btnPrev);
    ctrlLayout->addWidget(btnPlay);
    ctrlLayout->addWidget(btnNext);
    ctrlLayout->addWidget(btnStop);

    // 音量
    QToolButton* btnMute = makeBtn("静音", "🔊");
    QSlider* volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setFixedWidth(80);
    volumeSlider->setValue(80);
    connect(volumeSlider, &QSlider::valueChanged, this, [this](int v) {
        setVolume(v / 100.0f);
    });
    connect(btnMute, &QToolButton::clicked, this, [this, volumeSlider] {
        if (audioOutput_->volume() > 0) {
            audioOutput_->setVolume(0);
            volumeSlider->setValue(0);
        } else {
            audioOutput_->setVolume(0.8f);
            volumeSlider->setValue(80);
        }
    });

    ctrlLayout->addWidget(btnMute);
    ctrlLayout->addWidget(volumeSlider);

    // 扩展：倍速/循环/列表
    extra_ = new QWidget;
    auto* extraLayout = new QHBoxLayout(extra_);
    extraLayout->setContentsMargins(0, 0, 0, 0);
    extraLayout->setSpacing(8);

    QToolButton* btnSpeed = makeBtn("播放速度", "1x");
    QToolButton* btnLoop = makeBtn("循环模式", "🔁");
    QToolButton* btnList = makeBtn("播放列表", "📋");
    extraLayout->addWidget(btnSpeed);
    extraLayout->addWidget(btnLoop);
    extraLayout->addWidget(btnList);

    // 组装底部
    bottomLayout->addWidget(trackInfo_, 1);
    bottomLayout->addWidget(controls_);
    bottomLayout->addWidget(extra_);

    mainLayout->addWidget(progressBar_);
    mainLayout->addLayout(bottomLayout);
}

void PlayerBar::setupPlayer()
{
    player_ = new QMediaPlayer(this);
    audioOutput_ = new QAudioOutput(this);
    player_->setAudioOutput(audioOutput_);

    connect(player_, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        emit playbackStateChanged(state);
        updateControls();
        if (state == QMediaPlayer::StoppedState && !userSeeking_) {
            emit mediaEnded();
        }
    });

    connect(player_, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
        if (!userSeeking_) {
            progressBar_->setValue(pos);
        }
        emit positionChanged(pos);
    });

    connect(player_, &QMediaPlayer::durationChanged, this, [this](qint64 dur) {
        progressBar_->setRange(0, dur);
        emit durationChanged(dur);
    });

    connect(player_, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString& errorString) {
        Q_UNUSED(error);
        emit errorOccurred(errorString);
        stop();
    });
}

void PlayerBar::play(const QUrl& url, MediaType type)
{
    currentUrl_ = url;
    currentType_ = type;
    player_->setSource(url);
    player_->play();
    setVisible(true);
    updateControls();
}

void PlayerBar::pause()
{
    player_->pause();
}

void PlayerBar::resume()
{
    player_->play();
}

void PlayerBar::stop()
{
    player_->stop();
    setVisible(false);
    currentUrl_.clear();
}

void PlayerBar::setPosition(qint64 ms)
{
    player_->setPosition(ms);
}

void PlayerBar::setVolume(float volume)
{
    audioOutput_->setVolume(volume);
    emit volumeChanged(volume);
}

void PlayerBar::updateControls()
{
    // TODO: 更新播放/暂停按钮图标、标题/作者标签
}

#include "moc_playerbar.cpp"
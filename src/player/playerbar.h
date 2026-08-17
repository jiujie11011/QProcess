/* ============================================================
 * QProcess PlayerBar
 * 底部按需出现的音频/视频播放条
 * 版本：v1.4（对应报告 §4.2, §4.3, §11.3）
 * ============================================================ */
#ifndef PLAYERBAR_H
#define PLAYERBAR_H

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QUrl>

#if !defined(QT6)
#include <QMediaContent>
#endif

class QSlider;

class PlayerBar : public QWidget
{
    Q_OBJECT
public:
    enum class MediaType { Audio, Video };
    Q_ENUM(MediaType)

    explicit PlayerBar(QWidget* parent = nullptr);
    ~PlayerBar() override = default;

    // 播放指定 URL
    void play(const QUrl& url, MediaType type = MediaType::Audio);

    // 暂停/恢复
    void pause();
    void resume();

    // 停止并隐藏
    void stop();

    // 跳转进度
    void setPosition(qint64 ms);

    // 音量
    void setVolume(float volume); // 0.0-1.0

    // 是否正在播放
    bool isPlaying() const {
#if defined(QT6)
        return player_->playbackState() == QMediaPlayer::PlayingState;
#else
        return player_->state() == QMediaPlayer::PlayingState;
#endif
    }

    // 当前媒体信息
    QUrl currentUrl() const { return currentUrl_; }
    MediaType currentMediaType() const { return currentType_; }

signals:
#if defined(QT6)
    void playbackStateChanged(QMediaPlayer::PlaybackState state);
#else
    void playbackStateChanged(QMediaPlayer::State state);
#endif
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void volumeChanged(float volume);
    void mediaEnded(); // 自然播放结束
    void errorOccurred(const QString& error);

private:
    void setupUI();
    void setupPlayer();
    void updateControls();

    QMediaPlayer* player_;
    QAudioOutput* audioOutput_;
    QUrl currentUrl_;
    MediaType currentType_ = MediaType::Audio;

    // UI 组件
    QWidget* trackInfo_;      // 标题/作者
    QSlider* progressBar_;    // 进度条（可拖拽）
    QWidget* controls_;       // 播放/暂停/停止/静音/音量
    QWidget* extra_;          // 倍速/播放列表/全屏等

    // 状态
    bool userSeeking_ = false;
};

#endif // PLAYERBAR_H
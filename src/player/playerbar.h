/* ============================================================
 * QProcess PlayerBar
 * On-demand audio/video player bar at the bottom
 * Version: v1.4 (report section 4.2, 4.3, 11.3)
 * ============================================================ */
#ifndef PLAYERBAR_H
#define PLAYERBAR_H

#include <QWidget>
#include <QMediaPlayer>
#include <QTimer>
#include <QUrl>
#include "qt6compat.h"

class QSlider;

class PlayerBar : public QWidget
{
    Q_OBJECT
public:
    enum class MediaType { Audio, Video };
    Q_ENUM(MediaType)

    explicit PlayerBar(QWidget* parent = nullptr);
    ~PlayerBar() override = default;

    // Play the given URL
    void play(const QUrl& url, MediaType type = MediaType::Audio);

    // Pause/resume
    void pause();
    void resume();

    // Stop and hide
    void stop();

    // Seek to position
    void setPosition(qint64 ms);

    // Volume
    void setVolume(float volume); // 0.0-1.0

    // Whether playing
    bool isPlaying() const {
        return QMEDIAPLAYER_PLAYBACK_STATE(player_) == QMediaPlayer::PlayingState;
    }

    // Current media info
    QUrl currentUrl() const { return currentUrl_; }
    MediaType currentMediaType() const { return currentType_; }

signals:
    // Uses Qt5/Qt6 common QMediaPlayer::State (Qt5) / PlaybackState (Qt6)
    // PlayingState has the same value in Qt5/Qt6, so int is safe
    void playbackStateChanged(int state);
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void volumeChanged(float volume);
    void mediaEnded(); // natural playback finished
    void errorOccurred(const QString& error);

private:
    void setupUI();
    void setupPlayer();
    void updateControls();

    QMediaPlayer* player_;
#if defined(QT6)
    QAudioOutput* audioOutput_;
#else
    // Qt5: no QAudioOutput object; volume controlled via QMediaPlayer::setVolume
#endif
    QUrl currentUrl_;
    MediaType currentType_ = MediaType::Audio;

    // UI widgets
    QWidget* trackInfo_;      // title/author
    QSlider* progressBar_;    // seek bar (draggable)
    QWidget* controls_;       // play/pause/stop/mute/volume
    QWidget* extra_;          // speed/playlist/fullscreen etc.

    // State
    bool userSeeking_ = false;
};

#endif // PLAYERBAR_H
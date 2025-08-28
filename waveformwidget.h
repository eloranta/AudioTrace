#pragma once
#include <QWidget>
#include <QVector>
#include <QMutex>
#include <QTimer>
#include <QAudioFormat>

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QAudioSource>
#include <QAudioDevice>
#include <QMediaDevices>
#else
#include <QAudioInput>
#include <QAudioDeviceInfo>
#endif

class WaveformWidget : public QWidget
{
public:
    explicit WaveformWidget(QWidget *parent = nullptr);
    ~WaveformWidget();

protected:
    void paintEvent(QPaintEvent *e) override;
    QSize minimumSizeHint() const override { return { 600, 200 }; }

private:
    void initAudio();
    void appendSamples(const QByteArray &bytes);

    QVector<qint16> m_samples;   // mono samples (most-recent buffer)
    QMutex          m_mutex;
    int             m_maxSamples; // how many samples to keep for drawing

    QAudioSource   *m_audio = nullptr;
    QAudioDevice    m_deviceInfo;
    QIODevice      *m_capture = nullptr;
    QAudioFormat    m_format;

    QTimer          m_timer; // triggers repaint ~30 FPS
};

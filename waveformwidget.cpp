#include "waveformwidget.h"
#include <QPainter>
#include <QtMath>

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent),
    m_maxSamples(44100 / 2) // ~0.5 s at 44.1 kHz
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);

    initAudio();

    // Redraw at ~30 FPS
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, this, QOverload<>::of(&WaveformWidget::update));
    m_timer.start();
}

WaveformWidget::~WaveformWidget() {
    if (m_audio) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
        m_audio->stop();
#else
        m_audio->stop();
#endif
    }
}

void WaveformWidget::initAudio() {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    m_deviceInfo = QMediaDevices::defaultAudioInput();

    QAudioFormat fmt;
    fmt.setSampleRate(48000);
    fmt.setChannelCount(2);
    fmt.setSampleFormat(QAudioFormat::Float); // Signed 16-bit

    if (!m_deviceInfo.isFormatSupported(fmt)) {
        // Fallback to device preferred format (we will only handle Int16 below)
        fmt = m_deviceInfo.preferredFormat();
    }
    m_format = fmt;

    m_audio = new QAudioSource(m_deviceInfo, m_format, this);
    m_capture = m_audio->start();

    // Pull-mode: read from the returned QIODevice
    connect(m_capture, &QIODevice::readyRead, this, [this]() {
        const QByteArray data = m_capture->readAll();
        appendSamples(data);
    });
#else
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();

    QAudioFormat fmt;
    fmt.setSampleRate(44100);
    fmt.setChannelCount(1);
    fmt.setSampleSize(16);
    fmt.setCodec("audio/pcm");
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleType(QAudioFormat::SignedInt);

    if (!info.isFormatSupported(fmt)) {
        fmt = info.nearestFormat(fmt);
    }
    m_format = fmt;

    m_audio = new QAudioInput(info, m_format, this);
    m_capture = m_audio->start();

    connect(m_capture, &QIODevice::readyRead, this, [this]() {
        const QByteArray data = m_capture->readAll();
        appendSamples(data);
    });
#endif
}

void WaveformWidget::appendSamples(const QByteArray &bytes) {
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    // if (m_format.sampleFormat() != QAudioFormat::Int16) {
    //     return; // keep code simple: only draw 16-bit
    // }
    const int channels = qMax(1, m_format.channelCount());
    const float *src = reinterpret_cast<const float*>(bytes.constData());
    const int frames = bytes.size() / (sizeof(float) * m_format.channelCount());
#else
    if (m_format.sampleSize() != 16) {
        return; // keep code simple: only draw 16-bit
    }
    const int channels = qMax(1, m_format.channelCount());
    const int16_t *src = reinterpret_cast<const int16_t*>(bytes.constData());
    const int frames = bytes.size() / (int)(sizeof(int16_t) * channels);
#endif

    QMutexLocker lock(&m_mutex);
    m_samples.reserve(qMin(m_maxSamples * 2, m_samples.size() + frames));

    // Downmix to mono (or just pass-through if already mono)
    for (int i = 0; i < frames; ++i) {
        float sum = 0.f;
        for (int c = 0; c < m_format.channelCount(); ++c)
            sum += *src++;
        float mono = sum / m_format.channelCount(); // downmix to mono
        m_samples.push_back(static_cast<qint16>(mono * 32767.0f));
    }

    // Keep only the most recent m_maxSamples
    if (m_samples.size() > m_maxSamples) {
        const int removeCount = m_samples.size() - m_maxSamples;
        m_samples.erase(m_samples.begin(), m_samples.begin() + removeCount);
    }
}

void WaveformWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), palette().window());

    // Snapshot the samples to avoid holding the lock during painting
    QVector<qint16> local;
    {
        QMutexLocker lock(&m_mutex);
        local = m_samples;
    }
    const int w = width();
    const int h = height();
    const int midY = h / 2;

    // Midline
    p.setPen(QPen(palette().mid().color(), 1));
    p.drawLine(0, midY, w, midY);

    if (local.isEmpty() || w <= 2 || h <= 2)
        return;

    // Reduce point count so we don't draw millions of points
    const int N = local.size();
    const int step = qMax(1, N / w); // ~one sample per pixel
    QPolygonF poly;
    poly.reserve(N / step + 1);

    const double amp = (h / 2.0) - 2.0;
    for (int i = 0; i < N; i += step) {
        const double x = (double)i / (N - 1) * (w - 1);
        const double y = midY - (local[i] / 32768.0) * amp;
        poly << QPointF(x, y);
    }

    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(palette().highlight().color(), 1));
    p.drawPolyline(poly);

    // Outline
    p.setPen(QPen(palette().mid().color(), 1));
    p.drawRect(rect().adjusted(0, 0, -1, -1));
}

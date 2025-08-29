#include "waveformwidget.h"
#include <QPainter>
#include <QtMath>

static QAudioDevice pickLineIn() {
    const auto inputs = QMediaDevices::audioInputs();
    for (const QAudioDevice &d : inputs) {
        // Check common naming; adjust to your exact device string if needed
        const QString name = d.description();
        if (name.contains("Line In", Qt::CaseInsensitive) ||
            name.contains("Line-In", Qt::CaseInsensitive) ||
            name.contains("Line",    Qt::CaseInsensitive)) {
            qInfo() << "Selected input device:" << name;
            return d;
        }
    }
    qWarning() << "No 'Line In' found; using default input instead.";
    return QMediaDevices::defaultAudioInput();
}

WaveformWidget::WaveformWidget(QWidget *parent) : QWidget(parent), m_maxSamples(44100 / 2) // ~0.5 s at 44.1 kHz
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);

    initAudio();

    // Redraw at ~30 FPS
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, this, QOverload<>::of(&WaveformWidget::update));
    m_timer.start();
}

WaveformWidget::~WaveformWidget()
{
    if (m_audio)
    {
        m_audio->stop();
    }
}

void WaveformWidget::initAudio()
{
    qInfo() << "=== Available INPUT devices ===";
    for (const QAudioDevice &d : QMediaDevices::audioInputs())
        qInfo() << " -" << d.description() << "id=" << d.id();

    m_deviceInfo = QMediaDevices::defaultAudioInput();

    QAudioDevice inDev = pickLineIn();

    QAudioFormat fmt;
    fmt.setSampleRate(48000);
    fmt.setChannelCount(2);
    fmt.setSampleFormat(QAudioFormat::Float);

    if (!m_deviceInfo.isFormatSupported(fmt))
    {
        // Fallback to device preferred format
        fmt = m_deviceInfo.preferredFormat();
    }
    m_format = fmt;

    m_audio = new QAudioSource(inDev, m_format, this);
    m_capture = m_audio->start();

    // Pull-mode: read from the returned QIODevice
    connect(m_capture, &QIODevice::readyRead, this, [this]()
    {
        const QByteArray data = m_capture->readAll();
        appendSamples(data);
    });
}

void WaveformWidget::appendSamples(const QByteArray &bytes)
{
    const float *src = reinterpret_cast<const float*>(bytes.constData());
    const int frames = bytes.size() / (sizeof(float) * m_format.channelCount());

    QMutexLocker lock(&m_mutex);
    m_samples.reserve(qMin(m_maxSamples * 2, m_samples.size() + frames));

    // Downmix to mono (or just pass-through if already mono)
    for (int i = 0; i < frames; ++i)
    {
        float sum = 0.f;
        for (int c = 0; c < m_format.channelCount(); ++c)
            sum += *src++;
        float mono = sum / m_format.channelCount(); // downmix to mono
        m_samples.push_back(static_cast<qint16>(mono * 32767.0f));
    }

    // Keep only the most recent m_maxSamples
    if (m_samples.size() > m_maxSamples)
    {
        const int removeCount = m_samples.size() - m_maxSamples;
        m_samples.erase(m_samples.begin(), m_samples.begin() + removeCount);
    }
}

void WaveformWidget::paintEvent(QPaintEvent *)
{
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
    for (int i = 0; i < N; i += step)
    {
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

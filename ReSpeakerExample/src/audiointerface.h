#ifndef AUDIOINTERFACE_H
#define AUDIOINTERFACE_H

#include <QObject>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QBuffer>

#include "wavfile.h"
#include "micarray.h"
#include "spectrumanalyser.h"

class QAudioInput;
class QAudioOutput;
class FrequencySpectrum;

class AudioInterface : public QObject
{
    Q_OBJECT
public:
    explicit AudioInterface(QObject *parent = 0);
    ~AudioInterface();

    const QList<QAudioDeviceInfo> &availableAudioInputDevicesList() const
    { return availableAudioInputDevices; }

    const QList<QAudioDeviceInfo> &availableAudioOutputDevicesList() const
    { return availableAudioOutputDevices; }

    QAudio::Mode mode() const { return audioMode; }
    QAudio::State state() const { return audioState; }


    /**
     * \return Current audio format
     * \note May be QAudioFormat() if engine is not initialized
     */
    const QAudioFormat& format() const { return audioFormat; }

    /**
     * Stop any ongoing recording or playback, and reset to ground state.
     */
    void reset();


    QAudio::Mode        audioMode;
    QAudio::State       audioState;


    const QList<QAudioDeviceInfo>  availableAudioInputDevices;
    QAudioDeviceInfo     audioInputDevice;
    QAudioInput*         audioInput;
    QIODevice*           audioInputIODevice;
    qint64               recordPosition;

    const QList<QAudioDeviceInfo>  availableAudioOutputDevices;
    QAudioDeviceInfo     audioOutputDevice;
    QAudioOutput*        audioOutput;
    qint64               playPosition;
    QBuffer              audioOutputIODevice;

    WavFile*            wavFile;
    // We need a second file handle via which to read data into m_buffer
    // for analysis
    WavFile*            analysisFile;


    QAudioFormat        audioFormat;

    QByteArray          audioBuffer;
    qint64              audioBufferLength;
    qint64              audioBufferPosition;
    qint64              audioDataLength;

    bool                generateTone;
    SweptTone           tone;

    int                 levelBufferLength;
    qreal               audioRmsLevel;
    qreal               audioPeakLevel;



    int                 spectrumBufferLength;
    QByteArray          spectrumBuffer;
    SpectrumAnalyser    spectrumAnalyser;
    qint64              spectrumPosition;

    int                 audioCount;


    /**
     * Length of the internal engine buffer.
     * \return Buffer length in bytes.
     */
    qint64 bufferLength() const;

    /**
     * Amount of data held in the buffer.
     * \return Data length in bytes.
     */
    qint64 dataLength() const { return audioDataLength; }




    bool initialize() ;
    void resetAudioDevices();
    bool selectFormat() ;

    void stopRecording();
    void stopPlayback();


    void setState(QAudio::State state);
    void setState(QAudio::Mode mode, QAudio::State state);
    void setFormat(const QAudioFormat &format);
    void setRecordPosition(qint64 position, bool forceEmit = false);
    void setPlayPosition(qint64 position, bool forceEmit = false);
    void calculateLevel(qint64 position, qint64 length);
    void calculateSpectrum(qint64 position);
    void setLevel(qreal rmsLevel, qreal peakLevel, int numSamples);

signals:
    void stateChanged(QAudio::Mode mode, QAudio::State state);

    /**
     * Informational message for non-modal display
     */
    void infoMessage(const QString &message, int durationMs);

    /**
     * Error message for modal display
     */
    void errorMessage(const QString &heading, const QString &detail);

    /**
     * Format of audio data has changed
     */
    void formatChanged(const QAudioFormat &format);

    /**
     * Length of buffer has changed.
     * \param duration Duration in microseconds
     */
    void bufferLengthChanged(qint64 duration);

    /**
     * Amount of data in buffer has changed.
     * \param Length of data in bytes
     */
    void dataLengthChanged(qint64 duration);

    /**
     * Position of the audio input device has changed.
     * \param position Position in bytes
     */
    void recordPositionChanged(qint64 position);

    /**
     * Position of the audio output device has changed.
     * \param position Position in bytes
     */
    void playPositionChanged(qint64 position);

    /**
     * Level changed
     * \param rmsLevel RMS level in range 0.0 - 1.0
     * \param peakLevel Peak level in range 0.0 - 1.0
     * \param numSamples Number of audio samples analyzed
     */
    void levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples);

    /**
     * Spectrum has changed.
     * \param position Position of start of window in bytes
     * \param length   Length of window in bytes
     * \param spectrum Resulting frequency spectrum
     */
    void spectrumChanged(qint64 position, qint64 length, const FrequencySpectrum &spectrum);

    /**
     * Buffer containing audio data has changed.
     * \param position Position of start of buffer in bytes
     * \param buffer   Buffer
     */
    void bufferChanged(qint64 position, qint64 length, const QByteArray &buffer);

public slots:
    void startRecording();
    void startPlayback();
    void suspend();
    void setAudioInputDevice(const QAudioDeviceInfo &device);
    void setAudioOutputDevice(const QAudioDeviceInfo &device);

private slots:
    void audioNotify();
    void audioStateChanged(QAudio::State state);
    void audioDataReady();
    void spectrumChanged(const FrequencySpectrum &spectrum);

};

#endif // AUDIOINTERFACE_H

#include "audiointerface.h"

#include <QAudioInput>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>


#include <math.h>

#include "utils.h"
#include "tonegenerator.h"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

const qint64 BufferDurationUs       = 10 * 1000000;
const int    NotifyIntervalMs       = 100;

// Size of the level calculation window in microseconds
const int    LevelWindowUs          = 0.1 * 1000000;


AudioInterface::AudioInterface(QObject *parent)
    : QObject(parent)
    , audioMode(QAudio::AudioInput)
    , audioState(QAudio::StoppedState)
    , generateTone(false)
    , wavFile(0)
    , analysisFile(0)
    , availableAudioInputDevices
        (QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    , audioInputDevice(QAudioDeviceInfo::defaultInputDevice())
    , availableAudioOutputDevices
        (QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
    , audioOutputDevice(QAudioDeviceInfo::defaultOutputDevice())
    , audioBufferLength(0)
    , audioInput(0)
    , audioInputIODevice(0)
    , recordPosition(0)
    , audioOutput(0)
    , playPosition(0)
    , audioBufferPosition(0)
    , audioDataLength(0)
    , levelBufferLength(0)
    , audioRmsLevel(0.0)
    , audioPeakLevel(0.0)
    , spectrumBufferLength(0)
    , spectrumAnalyser(0)
    , spectrumPosition(0)
    , audioCount(0)

{
    qRegisterMetaType<FrequencySpectrum>("FrequencySpectrum");
    qRegisterMetaType<WindowFunction>("WindowFunction");
    CHECKED_CONNECT(&spectrumAnalyser,
                    SIGNAL(spectrumChanged(FrequencySpectrum)),
                    this,
                    SLOT(spectrumChanged(FrequencySpectrum)));

    // initialize();

#ifdef DUMP_DATA
    createOutputDir();
#endif

#ifdef DUMP_SPECTRUM
    m_spectrumAnalyser.setOutputPath(outputPath());
#endif
}

AudioInterface::~AudioInterface() {

}

qint64 AudioInterface::bufferLength() const
{
    return wavFile ? wavFile->size() : audioBufferLength;
}

void AudioInterface::resetAudioDevices()
{
    delete audioInput;
    audioInput = 0;
    audioInputIODevice = 0;
    setRecordPosition(0);
    delete audioOutput;
    audioOutput = 0;
    setPlayPosition(0);
    spectrumPosition = 0;
    setLevel(0.0, 0.0, 0);
}

void AudioInterface::reset()
{
    stopRecording();
    stopPlayback();
    setState(QAudio::AudioInput, QAudio::StoppedState);
    setFormat(QAudioFormat());
    generateTone = false;
    delete wavFile;
    wavFile = 0;
    delete analysisFile;
    analysisFile = 0;
    audioBuffer.clear();
    audioBufferPosition = 0;
    audioBufferLength = 0;
    audioDataLength = 0;
    emit dataLengthChanged(0);
    resetAudioDevices();
}


bool AudioInterface::initialize()
{
    bool result = false;

    QAudioFormat format = audioFormat;

    if (selectFormat()) {
        if (audioFormat != format) {
            resetAudioDevices();
            if (wavFile) {
                emit bufferLengthChanged(bufferLength());
                emit dataLengthChanged(dataLength());
                emit bufferChanged(0, 0, audioBuffer);
                setRecordPosition(bufferLength());
                result = true;
            } else {
                audioBufferLength = audioLength(audioFormat, BufferDurationUs);
                audioBuffer.resize(audioBufferLength);
                audioBuffer.fill(0);
                emit bufferLengthChanged(bufferLength());
                if (generateTone) {
                    if (0 == tone.endFreq) {
                        const qreal nyquist = nyquistFrequency(audioFormat);
                        tone.endFreq = qMin(qreal(SpectrumHighFreq), nyquist);
                    }
                    // Call function defined in utils.h, at global scope
                    ::generateTone(tone, audioFormat, audioBuffer);
                    audioDataLength = audioBufferLength;
                    emit dataLengthChanged(dataLength());
                    emit bufferChanged(0, audioDataLength, audioBuffer);
                    setRecordPosition(audioBufferLength);
                    result = true;
                } else {
                    emit bufferChanged(0, 0, audioBuffer);
                    audioInput = new QAudioInput(audioInputDevice, audioFormat, this);
                    audioInput->setNotifyInterval(NotifyIntervalMs);
                    result = true;
                }
            }
            audioOutput = new QAudioOutput(audioOutputDevice, audioFormat, this);
            audioOutput->setNotifyInterval(NotifyIntervalMs);
        }
    } else {
        if (wavFile)
            emit errorMessage(tr("Audio format not supported"),
                              formatToString(audioFormat));
        else if (generateTone)
            emit errorMessage(tr("No suitable format found"), "");
        else
            emit errorMessage(tr("No common input / output format found"), "");
    }

    ENGINE_DEBUG << "AudioInterface::initialize" << "audioBufferLength" << audioBufferLength;
    ENGINE_DEBUG << "AudioInterface::initialize" << "audioDataLength" << audioDataLength;
    ENGINE_DEBUG << "AudioInterface::initialize" << "audioFormat" << audioFormat;

    return result;
}

// Formats

bool AudioInterface::selectFormat()
{
    bool foundSupportedFormat = false;

    if (wavFile || QAudioFormat() != audioFormat) {
        QAudioFormat format = audioFormat;
        if (wavFile)
            // Header is read from the WAV file; just need to check whether
            // it is supported by the audio output device
            format = wavFile->fileFormat();
        if (audioOutputDevice.isFormatSupported(format)) {
            setFormat(format);
            foundSupportedFormat = true;
        }
    } else {

        QList<int> sampleRatesList;
    #ifdef Q_OS_WIN
        // The Windows audio backend does not correctly report format support
        // (see QTBUG-9100).  Furthermore, although the audio subsystem captures
        // at 11025Hz, the resulting audio is corrupted.
        sampleRatesList += 8000;
    #endif

        if (!generateTone)
            sampleRatesList += audioInputDevice.supportedSampleRates();

        sampleRatesList += audioOutputDevice.supportedSampleRates();
        sampleRatesList = sampleRatesList.toSet().toList(); // remove duplicates
        qSort(sampleRatesList);
        ENGINE_DEBUG << "AudioInterface::initialize frequenciesList" << sampleRatesList;

        QList<int> channelsList;
        channelsList += audioInputDevice.supportedChannelCounts();
        channelsList += audioOutputDevice.supportedChannelCounts();
        channelsList = channelsList.toSet().toList();
        qSort(channelsList);
        ENGINE_DEBUG << "AudioInterface::initialize channelsList" << channelsList;

        QAudioFormat format;
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setCodec("audio/pcm");
        format.setSampleSize(16);
        format.setSampleType(QAudioFormat::SignedInt);
        int sampleRate, channels;
        foreach (sampleRate, sampleRatesList) {
            if (foundSupportedFormat)
                break;
            format.setSampleRate(sampleRate);
            foreach (channels, channelsList) {
                format.setChannelCount(channels);
                const bool inputSupport = generateTone ||
                                          audioInputDevice.isFormatSupported(format);
                const bool outputSupport = audioOutputDevice.isFormatSupported(format);
                ENGINE_DEBUG << "AudioInterface::initialize checking " << format
                             << "input" << inputSupport
                             << "output" << outputSupport;
                if (inputSupport && outputSupport) {
                    foundSupportedFormat = true;
                    break;
                }
            }
        }

        if (!foundSupportedFormat)
            format = QAudioFormat();

        setFormat(format);
    }

    return foundSupportedFormat;
}

void AudioInterface::startRecording()
{
    if (audioInput) {
        if (QAudio::AudioInput == audioMode &&
            QAudio::SuspendedState == audioState) {
            audioInput->resume();
        } else {
            spectrumAnalyser.cancelCalculation();
            spectrumChanged(0, 0, FrequencySpectrum());

            audioBuffer.fill(0);
            setRecordPosition(0, true);
            stopPlayback();
            audioMode = QAudio::AudioInput;
            CHECKED_CONNECT(audioInput, SIGNAL(stateChanged(QAudio::State)),
                            this, SLOT(audioStateChanged(QAudio::State)));
            CHECKED_CONNECT(audioInput, SIGNAL(notify()),
                            this, SLOT(audioNotify()));
            audioCount = 0;
            audioDataLength = 0;
            emit dataLengthChanged(0);
            audioInputIODevice = audioInput->start();
            CHECKED_CONNECT(audioInputIODevice, SIGNAL(readyRead()),
                            this, SLOT(audioDataReady()));
        }
    }
}

void AudioInterface::startPlayback()
{
    if (audioOutput) {
        if (QAudio::AudioOutput == audioMode &&
            QAudio::SuspendedState == audioState) {
#ifdef Q_OS_WIN
            // The Windows backend seems to internally go back into ActiveState
            // while still returning SuspendedState, so to ensure that it doesn't
            // ignore the resume() call, we first re-suspend
            audioOutput->suspend();
#endif
            audioOutput->resume();
        } else {
            spectrumAnalyser.cancelCalculation();
            spectrumChanged(0, 0, FrequencySpectrum());
            setPlayPosition(0, true);
            stopRecording();
            audioMode = QAudio::AudioOutput;
            CHECKED_CONNECT(audioOutput, SIGNAL(stateChanged(QAudio::State)),
                            this, SLOT(audioStateChanged(QAudio::State)));
            CHECKED_CONNECT(audioOutput, SIGNAL(notify()),
                            this, SLOT(audioNotify()));
            audioCount = 0;
            if (wavFile) {
                wavFile->seek(0);
                audioBufferPosition = 0;
                audioDataLength = 0;
                audioOutput->start(wavFile);
            } else {
                audioOutputIODevice.close();
                audioOutputIODevice.setBuffer(&audioBuffer);
                audioOutputIODevice.open(QIODevice::ReadOnly);
                audioOutput->start(&audioOutputIODevice);
            }
        }
    }
}

void AudioInterface::suspend()
{
    if (QAudio::ActiveState == audioState ||
        QAudio::IdleState == audioState) {
        switch (audioMode) {
        case QAudio::AudioInput:
            audioInput->suspend();
            break;
        case QAudio::AudioOutput:
            audioOutput->suspend();
            break;
        }
    }
}

void AudioInterface::setAudioInputDevice(const QAudioDeviceInfo &device)
{
    if (device.deviceName() != audioInputDevice.deviceName()) {
        audioInputDevice = device;
        initialize();
    }
}

void AudioInterface::setAudioOutputDevice(const QAudioDeviceInfo &device)
{
    if (device.deviceName() != audioOutputDevice.deviceName()) {
        audioOutputDevice = device;
        initialize();
    }
}


void AudioInterface::stopRecording()
{
    if (audioInput) {
        audioInput->stop();
        QCoreApplication::instance()->processEvents();
        audioInput->disconnect();
    }
    audioInputIODevice = 0;

#ifdef DUMP_AUDIO
    dumpData();
#endif
}

void AudioInterface::stopPlayback()
{
    if (audioOutput) {
        audioOutput->stop();
        QCoreApplication::instance()->processEvents();
        audioOutput->disconnect();
        setPlayPosition(0);
    }
}

void AudioInterface::setState(QAudio::State state)
{
    const bool changed = (audioState != state);
    audioState = state;
    if (changed)
        emit stateChanged(audioMode, audioState);
}

void AudioInterface::setState(QAudio::Mode mode, QAudio::State state)
{
    const bool changed = (audioMode != mode || audioState != state);
    audioMode = mode;
    audioState = state;
    if (changed)
        emit stateChanged(audioMode, audioState);
}

void AudioInterface::setRecordPosition(qint64 position, bool forceEmit)
{
    const bool changed = (recordPosition != position);
    recordPosition = position;
    if (changed || forceEmit)
        emit recordPositionChanged(recordPosition);
}

void AudioInterface::setPlayPosition(qint64 position, bool forceEmit)
{
    const bool changed = (playPosition != position);
    playPosition = position;
    if (changed || forceEmit)
        emit playPositionChanged(playPosition);
}

void AudioInterface::calculateLevel(qint64 position, qint64 length)
{
#ifdef DISABLE_LEVEL
    Q_UNUSED(position)
    Q_UNUSED(length)
#else
    Q_ASSERT(position + length <= audioBufferPosition + audioDataLength);

    qreal peakLevel = 0.0;

    qreal sum = 0.0;
    const char *ptr = audioBuffer.constData() + position - audioBufferPosition;
    const char *const end = ptr + length;
    while (ptr < end) {
        const qint16 value = *reinterpret_cast<const qint16*>(ptr);
        const qreal fracValue = pcmToReal(value);
        peakLevel = qMax(peakLevel, fracValue);
        sum += fracValue * fracValue;
        ptr += 2;
    }
    const int numSamples = length / 2;
    qreal rmsLevel = sqrt(sum / numSamples);

    rmsLevel = qMax(qreal(0.0), rmsLevel);
    rmsLevel = qMin(qreal(1.0), rmsLevel);
    setLevel(rmsLevel, peakLevel, numSamples);

    ENGINE_DEBUG << "AudioInterface::calculateLevel" << "pos" << position << "len" << length
                 << "rms" << rmsLevel << "peak" << peakLevel;
#endif
}

void AudioInterface::calculateSpectrum(qint64 position)
{
#ifdef DISABLE_SPECTRUM
    Q_UNUSED(position)
#else
    Q_ASSERT(position + spectrumBufferLength <= audioBufferPosition + audioDataLength);
    Q_ASSERT(0 == spectrumBufferLength % 2); // constraint of FFT algorithm

    // QThread::currentThread is marked 'for internal use only', but
    // we're only using it for debug output here, so it's probably OK :)
    ENGINE_DEBUG << "AudioInterface::calculateSpectrum" << QThread::currentThread()
                 << "count" << audioCount << "pos" << position << "len" << spectrumBufferLength
                 << "spectrumAnalyser.isReady" << spectrumAnalyser.isReady();

    if (spectrumAnalyser.isReady()) {
        spectrumBuffer = QByteArray::fromRawData(audioBuffer.constData() + position - audioBufferPosition,
                                                   spectrumBufferLength);
        spectrumPosition = position;
        spectrumAnalyser.calculate(spectrumBuffer, audioFormat);
    }
#endif
}

void AudioInterface::setFormat(const QAudioFormat &format)
{
    const bool changed = (format != audioFormat);
    audioFormat = format;
    levelBufferLength = audioLength(audioFormat, LevelWindowUs);
    spectrumBufferLength = SpectrumLengthSamples *
                            (audioFormat.sampleSize() / 8) * audioFormat.channelCount();
    if (changed)
        emit formatChanged(audioFormat);
}

void AudioInterface::setLevel(qreal rmsLevel, qreal peakLevel, int numSamples)
{
    audioRmsLevel = rmsLevel;
    audioPeakLevel = peakLevel;
    emit levelChanged(audioRmsLevel, audioPeakLevel, numSamples);
}


//-----------------------------------------------------------------------------
// Private slots
//-----------------------------------------------------------------------------

void AudioInterface::audioNotify()
{
    switch (audioMode) {
    case QAudio::AudioInput: {
            const qint64 recordPosition = qMin(audioBufferLength, audioLength(audioFormat,audioInput->processedUSecs()));
            setRecordPosition(recordPosition);
            const qint64 levelPosition = audioDataLength - levelBufferLength;
            if (levelPosition >= 0)
                calculateLevel(levelPosition, levelBufferLength);
            if (audioDataLength >= spectrumBufferLength) {
                const qint64 spectrumPosition = audioDataLength - spectrumBufferLength;
                calculateSpectrum(spectrumPosition);
            }
            emit bufferChanged(0, audioDataLength, audioBuffer);
        }
        break;
    case QAudio::AudioOutput: {
            const qint64 playPosition = audioLength(audioFormat, audioOutput->processedUSecs());
            setPlayPosition(qMin(bufferLength(), playPosition));
            const qint64 levelPosition = playPosition - levelBufferLength;
            const qint64 spectrumPosition = playPosition - spectrumBufferLength;
            if (wavFile) {
                if (levelPosition > audioBufferPosition ||
                    spectrumPosition > audioBufferPosition ||
                    qMax(levelBufferLength, spectrumBufferLength) > audioDataLength) {
                    audioBufferPosition = 0;
                    audioDataLength = 0;
                    // Data needs to be read into m_buffer in order to be analysed
                    const qint64 readPos = qMax(qint64(0), qMin(levelPosition, spectrumPosition));
                    const qint64 readEnd = qMin(analysisFile->size(), qMax(levelPosition + levelBufferLength, spectrumPosition + spectrumBufferLength));
                    const qint64 readLen = readEnd - readPos + audioLength(audioFormat, WaveformWindowDuration);
                    qDebug() << "AudioInterface::audioNotify [1]"
                             << "analysisFileSize" << analysisFile->size()
                             << "readPos" << readPos
                             << "readLen" << readLen;
                    if (analysisFile->seek(readPos + analysisFile->headerLength())) {
                        audioBuffer.resize(readLen);
                        audioBufferPosition = readPos;
                        audioDataLength = analysisFile->read(audioBuffer.data(), readLen);
                        qDebug() << "AudioInterface::audioNotify [2]" << "bufferPosition" << audioBufferPosition << "dataLength" << audioDataLength;
                    } else {
                        qDebug() << "AudioInterface::audioNotify [2]" << "file seek error";
                    }
                    emit bufferChanged(audioBufferPosition, audioDataLength, audioBuffer);
                }
            } else {
                if (playPosition >= audioDataLength)
                    stopPlayback();
            }
            if (levelPosition >= 0 && levelPosition + levelBufferLength < audioBufferPosition + audioDataLength)
                calculateLevel(levelPosition, levelBufferLength);
            if (spectrumPosition >= 0 && spectrumPosition + spectrumBufferLength < audioBufferPosition + audioDataLength)
                calculateSpectrum(spectrumPosition);
        }
        break;
    }
}

void AudioInterface::audioStateChanged(QAudio::State state)
{
    ENGINE_DEBUG << "AudioInterface::audioStateChanged from" << audioState
                 << "to" << state;

    if (QAudio::IdleState == state && wavFile && wavFile->pos() == wavFile->size()) {
        stopPlayback();
    } else {
        if (QAudio::StoppedState == state) {
            // Check error
            QAudio::Error error = QAudio::NoError;
            switch (audioMode) {
            case QAudio::AudioInput:
                error = audioInput->error();
                break;
            case QAudio::AudioOutput:
                error = audioOutput->error();
                break;
            }
            if (QAudio::NoError != error) {
                reset();
                return;
            }
        }
        setState(state);
    }
}

void AudioInterface::audioDataReady()
{
    Q_ASSERT(0 == audioBufferPosition);
    const qint64 bytesReady = audioInput->bytesReady();
    const qint64 bytesSpace = audioBuffer.size() - audioDataLength;
    const qint64 bytesToRead = qMin(bytesReady, bytesSpace);

    const qint64 bytesRead = audioInputIODevice->read(
                                       audioBuffer.data() + audioDataLength,
                                       bytesToRead);

    if (bytesRead) {
        audioDataLength += bytesRead;
        emit dataLengthChanged(dataLength());
    }

    if (audioBuffer.size() == audioDataLength)
        stopRecording();
}

void AudioInterface::spectrumChanged(const FrequencySpectrum &spectrum)
{
    ENGINE_DEBUG << "AudioInterface::spectrumChanged" << "pos" << spectrumPosition;
    emit spectrumChanged(spectrumPosition, spectrumBufferLength, spectrum);
}


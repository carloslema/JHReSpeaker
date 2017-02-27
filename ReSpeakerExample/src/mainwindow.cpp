#include <QApplication>
#include <QMainWindow>
#include <QColorDialog>
#include <QAudioInput>
#include <QAudioOutput>
#include <QMessageBox>
#include <QTimer>
#include <QPixmap>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "micarray.h"
#include "audiointerface.h"
#include "../../src/respeakermicarray.h"
#include "levelmeter.h"
#include "waveform.h"
#include "progressbar.h"
#include "spectrograph.h"

// Constants
const int NullTimerId = -1;
const int AutoReportInterval = 5; // ms


// micArray is the global handle to the far field microphone array
ReSpeakerMicArray *micArray ;
QColor ledAllColor ;




MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    audioInterface(new AudioInterface(this)),
    waveform(NULL),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    createUI() ;
    connectUI() ;
    hid_init();
    micArray = new ReSpeakerMicArray() ;
    std::cout << "Mic Array: " << micArray->handle << std::endl ;
    initAudioDeviceSelector() ;
    std::cout<<"Lights all white\n" ;
    ledAllColor = Qt::white ;
    QString qss = QString("background-color: %1").arg(ledAllColor.name());
    ui->ledAllColorButton->setStyleSheet(qss);
    micArray->setLEDMode(1,ledAllColor.red(),ledAllColor.green(),ledAllColor.blue()) ;
    usleep(1000000);
    micArray->setLEDAllOff() ;
    usleep(100000) ;
    micArray->setLEDAutoVoiceLocated();
    autoReportTimer = new QTimer(this) ;
    // Get the list of audio devices and print them out
    connect(autoReportTimer, SIGNAL(timeout()), this, SLOT(autoReportTimerExpired()));
    autoReportTimer->start(AutoReportInterval);

    unsigned char buf[4];

    micArray->readRegister( 0x10, buf, 1);
    printf("mic gain is %d\n", (char)buf[0]);

    buf[0] = 35;
    micArray->writeRegister(0x10, buf, 1);

    buf[0] = 0; // spk proc bypass
    micArray->writeRegister(0x13, buf, 1);

    buf[0] = 0; // no AGC
    micArray->writeRegister(0x2A, buf, 1);

    fflush(stdout) ;



}

MainWindow::~MainWindow()
{
    delete ui;
}

//
// LED Light Control
//

int MainWindow::setLEDAllRGB ( unsigned char redColor, unsigned char greenColor, unsigned char blueColor ) {
    micArray->setLEDAllRGB(redColor,greenColor,blueColor);
}

void MainWindow::on_ledAllOffRB_clicked()
{
    micArray->setLEDAllOff() ;
}

void MainWindow::on_ledAudioVoiceLocatedRB_clicked()
{
    micArray->setLEDAllOff();
    usleep(100000) ;
    micArray->setLEDAutoVoiceLocated();
}

void MainWindow::on_ledWaitingRB_clicked()
{
    micArray->setLEDWaiting() ;
}


void MainWindow::on_ledAllColorButton_clicked()
{
    QColor color = QColorDialog::getColor(ledAllColor, this );
    if( color.isValid() )
    {
        ledAllColor = color ;
        QString qss = QString("background-color: %1").arg(ledAllColor.name());
        ui->ledAllColorButton->setStyleSheet(qss);
        micArray->setLEDAllRGB(ledAllColor.red(),ledAllColor.green(),ledAllColor.blue()) ;
    }
}

void MainWindow::on_ledAllOneColorRB_clicked()
{
    QString qss = QString("background-color: %1").arg(ledAllColor.name());
    ui->ledAllColorButton->setStyleSheet(qss);
    micArray->setLEDAllRGB(ledAllColor.red(),ledAllColor.green(),ledAllColor.blue()) ;

}

void MainWindow::on_ledListeningRB_clicked()
{
    micArray->setLEDListening(45,0) ;

}

void MainWindow::on_ledSpeakingRB_clicked()
{
    micArray->setLEDSpeaking(32,32,32) ;

}

void MainWindow::on_ledDisplayDataRB_clicked()
{
    micArray->setLEDData() ;
}

void MainWindow::on_ledVolumeSlider_actionTriggered(int action)
{
    micArray->setLEDVolume(ui->ledVolumeSlider->value());
    ui->ledVolumeRB->setChecked(true) ;
}

void MainWindow::on_ledVolumeRB_clicked()
{
    micArray->setLEDVolume(ui->ledVolumeSlider->value()) ;

}


void MainWindow::on_horizontalSlider_actionTriggered(int action)
{

}

//
// Audio Device
//

void MainWindow::initAudioDeviceSelector ( void ) {
    QAudioDeviceInfo device;
    foreach (device, audioInterface->availableAudioInputDevices)
        ui->inputDeviceCB->addItem(device.deviceName(),
                                   QVariant::fromValue(device));
    int indexy = ui->inputDeviceCB->findText(QAudioDeviceInfo::defaultInputDevice().deviceName()) ;
    ui->inputDeviceCB->setCurrentIndex(indexy);
    foreach (device, audioInterface->availableAudioOutputDevices )
        ui->outputDeviceCB->addItem(device.deviceName(),
                                    QVariant::fromValue(device)) ;
    QString outputDeviceName = QAudioDeviceInfo::defaultOutputDevice().deviceName();
    indexy = ui->outputDeviceCB->findText(outputDeviceName);
    ui->outputDeviceCB->setCurrentIndex(indexy);

}

void MainWindow::on_inputDeviceCB_activated(const QString &arg1)
{

}

void MainWindow::on_inputDeviceCB_currentIndexChanged(int index)
{
    audioInterface->audioInputDevice = ui->inputDeviceCB->itemData(index).value<QAudioDeviceInfo>();
    ENGINE_DEBUG << "AudioInterface::inputDeviceCB_currentIndexChanged" << ui->inputDeviceCB->itemText(index);
    audioInterface->initialize() ;
}


// slots
void MainWindow::infoMessage(const QString &message, int timeoutMs)
{
    ui->infoMessageLabel->setText(message);

    if (NullTimerId != infoMessageTimerId) {
        killTimer(infoMessageTimerId);
        infoMessageTimerId = NullTimerId;
    }

    if (NullMessageTimeout != timeoutMs)
       infoMessageTimerId = startTimer(timeoutMs);
}

void MainWindow::errorMessage(const QString &heading, const QString &detail)
{
    QMessageBox::warning(this, heading, detail, QMessageBox::Close);
}

void MainWindow::stateChanged(QAudio::Mode mode, QAudio::State state)
{
    Q_UNUSED(mode);
    updateButtonStates();

    if (QAudio::ActiveState != state && QAudio::SuspendedState != state) {
        levelMeter->reset();
        spectrograph->reset();
    }
}

void MainWindow::formatChanged(const QAudioFormat &format)
{
   infoMessage(formatToString(format), NullMessageTimeout);



#ifndef DISABLE_WAVEFORM
    if (QAudioFormat() != format) {
        waveform->initialize(format, WaveformTileLength,
                               WaveformWindowDuration);
    }
#endif


}

void MainWindow::spectrumChanged(qint64 position, qint64 length,
                                 const FrequencySpectrum &spectrum)
{
#if 0
    m_progressBar->windowChanged(position, length);
#endif
    spectrograph->spectrumChanged(spectrum);

}

void MainWindow::audioPositionChanged(qint64 position)
{

#ifndef DISABLE_WAVEFORM
   waveform->audioPositionChanged(position);
#else
    Q_UNUSED(position)
#endif

}

void MainWindow::bufferLengthChanged(qint64 length)
{
#if 0
    m_progressBar->bufferLengthChanged(length);
#endif
}

void MainWindow::updateButtonStates()
{
    const bool recordEnabled = ((QAudio::AudioOutput == audioInterface->mode() ||
                                (QAudio::ActiveState != audioInterface->state() &&
                                 QAudio::IdleState != audioInterface->state())) &&
                                RecordMode == m_mode);
    ui->recordButton->setEnabled(recordEnabled);

    const bool pauseEnabled = (QAudio::ActiveState == audioInterface->state() ||
                               QAudio::IdleState == audioInterface->state());
    ui->pauseButton->setEnabled(pauseEnabled);

    const bool playEnabled = (/*ui->audioInterface->audioDataLength() &&*/
                              (QAudio::AudioOutput != audioInterface->mode() ||
                               (QAudio::ActiveState != audioInterface->state() &&
                                QAudio::IdleState != audioInterface->state())));
    ui->playButton->setEnabled(playEnabled);
}

void MainWindow::autoReportTimerExpired() {
    unsigned char vadActivity ;
    unsigned short angle ;
    int returned ;
    returned = micArray->readAutoReport(&angle,&vadActivity);
    if (returned) {
        printf("Return: %d Angle: %d VAD: %d\n",returned, angle, vadActivity) ;
        fflush(stdout);

        switch (vadActivity) {
          case 0:
            ui->vadIndicator->setPixmap(amberPixMap);
            break ;
          case 1:
            // Don't know what this is used for
            ui->vadIndicator->setPixmap(redPixMap) ;
            break ;
          case 2:
            // Voice Activity Detected ;
            ui->vadIndicator->setPixmap(greenPixMap) ;
            break ;
        default:
            ui->vadIndicator->setPixmap(redPixMap) ;
            break ;
        }


    }
    // printf("Voice Angle:\n") ;
    // micArray->voiceAngle();
}



void MainWindow::createUI() {
    recordIcon = QIcon(":/images/record.png") ;
    pauseIcon = style()->standardIcon(QStyle::SP_MediaPause);
    ui->pauseButton->setIcon(pauseIcon) ;
    playIcon = style()->standardIcon(QStyle::SP_MediaPlay);
    ui->playButton->setIcon(playIcon);
    setMode(RecordMode);
    waveform = (Waveform*)ui->waveformWidget ;
    levelMeter = (LevelMeter*)ui->levelMeterWidget;
    spectrograph = (Spectrograph*)ui->spectrographWidget;
    spectrograph->setParams(SpectrumNumBands, SpectrumLowFreq, SpectrumHighFreq);

    greenPixMap.load(":/images/32px-Button_Icon_Green.svg.png");
    amberPixMap.load(":/images/32px-Button_Icon_Orange.svg.png");
    redPixMap.load(":/images/32px-Button_Icon_Red.svg.png");


    // addWidget(waveform) ;

}

void MainWindow::connectUI()
{

#if 0
    CHECKED_CONNECT(m_settingsButton, SIGNAL(clicked()),
            this, SLOT(showSettingsDialog()));
#endif

    CHECKED_CONNECT(audioInterface, SIGNAL(stateChanged(QAudio::Mode,QAudio::State)),
            this, SLOT(stateChanged(QAudio::Mode,QAudio::State)));

    CHECKED_CONNECT(audioInterface, SIGNAL(formatChanged(const QAudioFormat &)),
            this, SLOT(formatChanged(const QAudioFormat &)));

#if 0
    m_progressBar->bufferLengthChanged(m_engine->bufferLength());
#endif

    CHECKED_CONNECT(audioInterface, SIGNAL(bufferLengthChanged(qint64)),
            this, SLOT(bufferLengthChanged(qint64)));

    CHECKED_CONNECT(audioInterface, SIGNAL(dataLengthChanged(qint64)),
            this, SLOT(updateButtonStates()));

#if 0

    CHECKED_CONNECT(audioInterface, SIGNAL(recordPositionChanged(qint64)),
            m_progressBar, SLOT(recordPositionChanged(qint64)));

    CHECKED_CONNECT(audioInterface, SIGNAL(playPositionChanged(qint64)),
            m_progressBar, SLOT(playPositionChanged(qint64)));
#endif

    CHECKED_CONNECT(audioInterface, SIGNAL(recordPositionChanged(qint64)),
            this, SLOT(audioPositionChanged(qint64)));

    CHECKED_CONNECT(audioInterface, SIGNAL(playPositionChanged(qint64)),
            this, SLOT(audioPositionChanged(qint64)));

    CHECKED_CONNECT(audioInterface, SIGNAL(levelChanged(qreal, qreal, int)),
            levelMeter, SLOT(levelChanged(qreal, qreal, int)));

    CHECKED_CONNECT(audioInterface, SIGNAL(spectrumChanged(qint64, qint64, const FrequencySpectrum &)),
            this, SLOT(spectrumChanged(qint64, qint64, const FrequencySpectrum &)));

    CHECKED_CONNECT(audioInterface, SIGNAL(infoMessage(QString, int)),
            this, SLOT(infoMessage(QString, int)));

    CHECKED_CONNECT(audioInterface, SIGNAL(errorMessage(QString, QString)),
            this, SLOT(errorMessage(QString, QString)));

    CHECKED_CONNECT(spectrograph, SIGNAL(infoMessage(QString, int)),
            this, SLOT(infoMessage(QString, int)));

#ifndef DISABLE_WAVEFORM
    CHECKED_CONNECT(audioInterface, SIGNAL(bufferChanged(qint64, qint64, const QByteArray &)),
            waveform, SLOT(bufferChanged(qint64, qint64, const QByteArray &)));
#endif

}


void MainWindow::setMode(Mode mode)
{
    m_mode = mode;
    // updateModeMenu();
}

void MainWindow::on_recordButton_clicked()
{
    if (audioInterface->initialize())
        updateButtonStates();
    audioInterface->startRecording();
    updateButtonStates() ;

}

void MainWindow::on_pauseButton_clicked()
{
    audioInterface->suspend();
}

void MainWindow::on_playButton_clicked()
{
    audioInterface->startPlayback();
}

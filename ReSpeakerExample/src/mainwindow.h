#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudio>
#include <QAudioFormat>

class AudioInterface ;
class FrequencySpectrum;
class LevelMeter;
class ProgressBar;
class Spectrograph;
class Waveform;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    AudioInterface *audioInterface ;
    int  infoMessageTimerId;

#ifndef DISABLE_WAVEFORM
    Waveform*               waveform;
#endif
    ProgressBar*            progressBar;
    Spectrograph*           spectrograph;
    LevelMeter*             levelMeter;

    QPixmap greenPixMap ;
    QPixmap amberPixMap ;
    QPixmap redPixMap ;


    enum Mode {
        NoMode,
        RecordMode,
        GenerateToneMode,
        LoadFileMode
    };

    void setMode(Mode mode);

private:
    Mode                    m_mode;

public slots:
    void stateChanged(QAudio::Mode mode, QAudio::State state);
    void formatChanged(const QAudioFormat &format);
    void spectrumChanged(qint64 position, qint64 length,
                         const FrequencySpectrum &spectrum);
    void infoMessage(const QString &message, int timeoutMs);
    void errorMessage(const QString &heading, const QString &detail);
    void audioPositionChanged(qint64 position);
    void bufferLengthChanged(qint64 length);
    void connectUI();


private slots:

    void on_ledAllOffRB_clicked();

    int setLEDAllRGB ( unsigned char redColor, unsigned char greenColor, unsigned char blueColor ) ;


    void on_ledAudioVoiceLocatedRB_clicked();

    void on_ledWaitingRB_clicked();

    void on_horizontalSlider_actionTriggered(int action);

    void on_ledAllColorButton_clicked();

    void on_ledAllOneColorRB_clicked();

    void on_inputDeviceCB_activated(const QString &arg1);

    void on_inputDeviceCB_currentIndexChanged(int index);

    void on_ledListeningRB_clicked();

    void on_ledSpeakingRB_clicked();

    void on_ledDisplayDataRB_clicked();

    void on_ledVolumeSlider_actionTriggered(int action);

    void on_ledVolumeRB_clicked();

    void on_recordButton_clicked();

    void on_pauseButton_clicked();

    void on_playButton_clicked();

    void updateButtonStates();

    void autoReportTimerExpired() ;

private:
    Ui::MainWindow *ui;
    void createUI ( void ) ;
    void initAudioDeviceSelector ( void );

    QIcon recordIcon ;
    QIcon pauseIcon ;
    QIcon playIcon ;
    QTimer *autoReportTimer;

};

#endif // MAINWINDOW_H

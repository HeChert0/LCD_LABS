#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <QObject>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QMediaRecorder>
#include <QVideoSink>
#include <QImage>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QWindow>
#include <QGuiApplication>
#include <QKeySequence>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class CameraManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool cameraAvailable READ cameraAvailable NOTIFY cameraAvailableChanged)
    Q_PROPERTY(bool cameraActive READ cameraActive NOTIFY cameraActiveChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(bool stealthMode READ stealthMode NOTIFY stealthModeChanged)
    Q_PROPERTY(QString cameraName READ cameraName NOTIFY cameraNameChanged)
    Q_PROPERTY(QString cameraDescription READ cameraDescription NOTIFY cameraDescriptionChanged)
    Q_PROPERTY(QString lastPhotoPath READ lastPhotoPath NOTIFY lastPhotoPathChanged)
    Q_PROPERTY(QString lastVideoPath READ lastVideoPath NOTIFY lastVideoPathChanged)
    Q_PROPERTY(int photoCount READ photoCount NOTIFY photoCountChanged)
    Q_PROPERTY(int videoCount READ videoCount NOTIFY videoCountChanged)
    Q_PROPERTY(QString recordingTime READ recordingTime NOTIFY recordingTimeChanged)
    Q_PROPERTY(QObject* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)

public:
    explicit CameraManager(QObject *parent = nullptr);
    ~CameraManager();

    bool cameraAvailable() const { return m_cameraAvailable; }
    bool cameraActive() const { return m_cameraActive; }
    bool recording() const { return m_recording; }
    bool stealthMode() const { return m_stealthMode; }
    QString cameraName() const { return m_cameraName; }
    QString cameraDescription() const { return m_cameraDescription; }
    QString lastPhotoPath() const { return m_lastPhotoPath; }
    QString lastVideoPath() const { return m_lastVideoPath; }
    int photoCount() const { return m_photoCount; }
    int videoCount() const { return m_videoCount; }
    QString recordingTime() const;
    QObject* videoSink() const { return m_videoSink; }
    void setVideoSink(QObject* sink);

    Q_INVOKABLE void startCamera();
    Q_INVOKABLE void stopCamera();
    Q_INVOKABLE void takePhoto();
    Q_INVOKABLE void takeStealthPhoto();
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void startStealthRecording();
    Q_INVOKABLE void stopStealthRecording();
    Q_INVOKABLE void enterStealthMode();
    Q_INVOKABLE void exitStealthMode();
    Q_INVOKABLE void toggleStealthMode();
    Q_INVOKABLE QString getCameraInfo();
    Q_INVOKABLE void setupHotkeys();
    Q_INVOKABLE void showWindow();
    Q_INVOKABLE void hideWindow();
    Q_INVOKABLE QStringList getCameraList();

signals:
    void cameraAvailableChanged();
    void cameraActiveChanged();
    void recordingChanged();
    void stealthModeChanged();
    void cameraNameChanged();
    void cameraDescriptionChanged();
    void lastPhotoPathChanged();
    void lastVideoPathChanged();
    void photoCountChanged();
    void videoCountChanged();
    void recordingTimeChanged();
    void videoSinkChanged();
    void photoTaken(const QString& path);
    void videoSaved(const QString& path);
    void errorOccurred(const QString& error);
    void stealthPhotoTaken();
    void stealthRecordingStarted();
    void stealthRecordingStopped();
    void cameraDetected();
    void hotkeyPressed(const QString& action);

private slots:
    void onImageCaptured(int id, const QImage& image);
    void onImageSaved(int id, const QString& fileName);
    void onRecorderStateChanged(QMediaRecorder::RecorderState state);
    void onRecorderErrorOccurred(QMediaRecorder::Error error, const QString& errorString);
    void updateRecordingTime();
    void checkForCameraActivity();

    void handleCameraActiveChanged(bool active);

private:
    void initializeCamera();
    void createOutputDirectories();
    QString generatePhotoPath();
    QString generateVideoPath();
    void installGlobalHotkeys();
    void uninstallGlobalHotkeys();

    QCamera* m_camera;
    QMediaCaptureSession* m_captureSession;
    QImageCapture* m_imageCapture;
    QMediaRecorder* m_mediaRecorder;
    QVideoSink* m_videoSink;

    bool m_cameraAvailable;
    bool m_cameraActive;
    bool m_recording;
    bool m_stealthMode;
    bool m_stealthRecording;

    QString m_cameraName;
    QString m_cameraDescription;
    QString m_lastPhotoPath;
    QString m_lastVideoPath;

    int m_photoCount;
    int m_videoCount;

    QDateTime m_recordingStartTime;
    QTimer* m_recordingTimer;
    QTimer* m_activityCheckTimer;

    QString m_photoDir;
    QString m_videoDir;

#ifdef Q_OS_WIN
    static HHOOK s_keyboardHook;
    static CameraManager* s_instance;
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif
};

#endif // CAMERAMANAGER_H

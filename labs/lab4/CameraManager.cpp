#include "CameraManager.h"
#include <QCameraDevice>
#include <QMediaDevices>
#include <QStandardPaths>
#include <QUrl>
#include <QDebug>
#include <QWindow>
#include <QGuiApplication>
#include <QMediaFormat>

#ifdef Q_OS_WIN
HHOOK CameraManager::s_keyboardHook = nullptr;
CameraManager* CameraManager::s_instance = nullptr;
#endif

CameraManager::CameraManager(QObject *parent)
    : QObject(parent)
    , m_camera(nullptr)
    , m_captureSession(nullptr)
    , m_imageCapture(nullptr)
    , m_mediaRecorder(nullptr)
    , m_videoSink(nullptr)
    , m_cameraAvailable(false)
    , m_cameraActive(false)
    , m_recording(false)
    , m_stealthMode(false)
    , m_stealthRecording(false)
    , m_photoCount(0)
    , m_videoCount(0)
{
#ifdef Q_OS_WIN
    s_instance = this;
#endif

    createOutputDirectories();
    initializeCamera();

    m_recordingTimer = new QTimer(this);
    m_recordingTimer->setInterval(1000);
    connect(m_recordingTimer, &QTimer::timeout, this, &CameraManager::updateRecordingTime);

    m_activityCheckTimer = new QTimer(this);
    m_activityCheckTimer->setInterval(500);
    connect(m_activityCheckTimer, &QTimer::timeout, this, &CameraManager::checkForCameraActivity);
    m_activityCheckTimer->start();

    setupHotkeys();
}

CameraManager::~CameraManager()
{
    uninstallGlobalHotkeys();
    if (m_camera) {
        m_camera->stop();
        delete m_camera;
    }
    delete m_captureSession;
    delete m_imageCapture;
    delete m_mediaRecorder;
}

void CameraManager::initializeCamera()
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        qWarning() << "No cameras found";
        m_cameraAvailable = false;
        emit cameraAvailableChanged();
        return;
    }

    QCameraDevice selectedCamera = QMediaDevices::defaultVideoInput();
    if (selectedCamera.isNull() && !cameras.isEmpty()) {
        selectedCamera = cameras.first();
    }

    m_cameraName = selectedCamera.description();
    m_cameraDescription = QString("ID: %1\nPosition: %2")
                              .arg(QString::fromUtf8(selectedCamera.id()))
                              .arg(selectedCamera.position() == QCameraDevice::FrontFace ? "Front" :
                                       selectedCamera.position() == QCameraDevice::BackFace ? "Back" : "Unknown");

    m_camera = new QCamera(selectedCamera, this);
    m_captureSession = new QMediaCaptureSession(this);
    m_imageCapture = new QImageCapture(this);
    m_mediaRecorder = new QMediaRecorder(this);

    connect(m_camera, SIGNAL(activeChanged(bool)),
            this, SLOT(handleCameraActiveChanged(bool)));

    m_captureSession->setCamera(m_camera);
    m_captureSession->setImageCapture(m_imageCapture);
    m_captureSession->setRecorder(m_mediaRecorder);

    m_imageCapture->setQuality(QImageCapture::VeryHighQuality);
    m_imageCapture->setFileFormat(QImageCapture::JPEG);

    QMediaFormat format;
    format.setVideoCodec(QMediaFormat::VideoCodec::H264);
    format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    format.setFileFormat(QMediaFormat::MPEG4);
    m_mediaRecorder->setMediaFormat(format);
    m_mediaRecorder->setQuality(QMediaRecorder::HighQuality);
    m_mediaRecorder->setVideoFrameRate(30);

    connect(m_imageCapture, &QImageCapture::imageCaptured,
            this, &CameraManager::onImageCaptured);
    connect(m_imageCapture, &QImageCapture::imageSaved,
            this, &CameraManager::onImageSaved);
    connect(m_mediaRecorder, &QMediaRecorder::recorderStateChanged,
            this, &CameraManager::onRecorderStateChanged);
    connect(m_mediaRecorder, &QMediaRecorder::errorOccurred,
            this, &CameraManager::onRecorderErrorOccurred);

    m_cameraAvailable = true;
    emit cameraAvailableChanged();
    emit cameraNameChanged();
    emit cameraDescriptionChanged();
}

void CameraManager::createOutputDirectories()
{
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_photoDir = documentsPath + "/CameraCaptures/Photos";
    m_videoDir = documentsPath + "/CameraCaptures/Videos";

    QDir().mkpath(m_photoDir);
    QDir().mkpath(m_videoDir);
}

QString CameraManager::generatePhotoPath()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString("%1/photo_%2.jpg").arg(m_photoDir).arg(timestamp);
}

QString CameraManager::generateVideoPath()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QString("%1/video_%2.mp4").arg(m_videoDir).arg(timestamp);
}

void CameraManager::setVideoSink(QObject* sink)
{
    m_videoSink = qobject_cast<QVideoSink*>(sink);
    if (m_videoSink && m_captureSession) {
        m_captureSession->setVideoOutput(m_videoSink);
    }
    emit videoSinkChanged();
}

void CameraManager::startCamera()
{
    if (!m_camera || m_cameraActive) return;

    m_camera->start();
    m_cameraActive = true;
    emit cameraActiveChanged();
    emit cameraDetected();
}

void CameraManager::stopCamera()
{
    if (!m_camera || !m_cameraActive) return;

    if (m_recording) {
        stopRecording();
    }

    m_camera->stop();
    m_cameraActive = false;
    emit cameraActiveChanged();
}

void CameraManager::takePhoto()
{
    if (!m_camera || !m_cameraActive || !m_imageCapture) return;

    QString path = generatePhotoPath();
    m_imageCapture->captureToFile(path);
    m_lastPhotoPath = path;
    emit lastPhotoPathChanged();
}

void CameraManager::takeStealthPhoto()
{
    bool wasHidden = m_stealthMode;

    if (!wasHidden) {
        hideWindow();
    }

    if (!m_cameraActive) {
        startCamera();
        QTimer::singleShot(500, [this, wasHidden]() {
            takePhoto();
            emit stealthPhotoTaken();

            QTimer::singleShot(1000, [this, wasHidden]() {
                stopCamera();
                if (!wasHidden) {
                    // showWindow();
                }
            });
        });
    } else {
        takePhoto();
        emit stealthPhotoTaken();
    }
}

void CameraManager::startRecording()
{
    if (!m_camera || !m_cameraActive || !m_mediaRecorder || m_recording) return;

    QString path = generateVideoPath();
    m_mediaRecorder->setOutputLocation(QUrl::fromLocalFile(path));
    m_mediaRecorder->record();
    m_lastVideoPath = path;
    m_recordingStartTime = QDateTime::currentDateTime();
    m_recording = true;
    m_recordingTimer->start();

    emit lastVideoPathChanged();
    emit recordingChanged();
}

void CameraManager::stopRecording()
{
    if (!m_mediaRecorder || !m_recording) return;

    m_mediaRecorder->stop();
    m_recording = false;
    m_stealthRecording = false;
    m_recordingTimer->stop();

    emit recordingChanged();
    emit recordingTimeChanged();
}

void CameraManager::startStealthRecording()
{
    if (m_recording) return;

    hideWindow();
    m_stealthRecording = true;

    if (!m_cameraActive) {
        startCamera();
        QTimer::singleShot(500, [this]() {
            startRecording();
            emit stealthRecordingStarted();
        });
    } else {
        startRecording();
        emit stealthRecordingStarted();
    }
}

void CameraManager::stopStealthRecording()
{
    if (!m_stealthRecording) return;

    stopRecording();
    stopCamera();
    emit stealthRecordingStopped();
}

void CameraManager::enterStealthMode()
{
    m_stealthMode = true;
    hideWindow();
    emit stealthModeChanged();
}

void CameraManager::exitStealthMode()
{
    m_stealthMode = false;
    showWindow();
    emit stealthModeChanged();
}

void CameraManager::toggleStealthMode()
{
    if (m_stealthMode) {
        exitStealthMode();
    } else {
        enterStealthMode();
    }
}

QString CameraManager::getCameraInfo()
{
    if (!m_cameraAvailable) {
        return "No camera available";
    }

    return QString("Camera: %1\n%2\nPhotos taken: %3\nVideos recorded: %4")
        .arg(m_cameraName)
        .arg(m_cameraDescription)
        .arg(m_photoCount)
        .arg(m_videoCount);
}

void CameraManager::showWindow()
{
    if (auto window = qobject_cast<QWindow*>(QGuiApplication::topLevelWindows().first())) {
        window->show();
        window->raise();
        window->requestActivate();
#ifdef Q_OS_WIN
        ShowWindow(reinterpret_cast<HWND>(window->winId()), SW_RESTORE);
#endif
    }
}

void CameraManager::hideWindow()
{
    if (auto window = qobject_cast<QWindow*>(QGuiApplication::topLevelWindows().first())) {
        window->hide();
#ifdef Q_OS_WIN
        ShowWindow(reinterpret_cast<HWND>(window->winId()), SW_HIDE);
#endif
    }
}

QStringList CameraManager::getCameraList()
{
    QStringList list;
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice& camera : cameras) {
        list.append(camera.description());
    }
    return list;
}

QString CameraManager::recordingTime() const
{
    if (!m_recording) return "00:00";

    qint64 seconds = m_recordingStartTime.secsTo(QDateTime::currentDateTime());
    return QString("%1:%2")
        .arg(seconds / 60, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'));
}

void CameraManager::onImageCaptured(int id, const QImage& image)
{
    Q_UNUSED(id)
    Q_UNUSED(image)
    m_photoCount++;
    emit photoCountChanged();
}

void CameraManager::onImageSaved(int id, const QString& fileName)
{
    Q_UNUSED(id)
    emit photoTaken(fileName);
}

void CameraManager::onRecorderStateChanged(QMediaRecorder::RecorderState state)
{
    if (state == QMediaRecorder::StoppedState && !m_lastVideoPath.isEmpty()) {
        m_videoCount++;
        emit videoCountChanged();
        emit videoSaved(m_lastVideoPath);
    }
}

void CameraManager::onRecorderErrorOccurred(QMediaRecorder::Error error, const QString& errorString)
{
    Q_UNUSED(error)
    emit errorOccurred("Recording error: " + errorString);
}

void CameraManager::updateRecordingTime()
{
    emit recordingTimeChanged();
}

void CameraManager::checkForCameraActivity()
{
    if (m_cameraActive) {
        emit cameraDetected();
    }
}

void CameraManager::setupHotkeys()
{
    installGlobalHotkeys();
}

#ifdef Q_OS_WIN
LRESULT CALLBACK CameraManager::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && wParam == WM_KEYDOWN && s_instance) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;

        bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;

        if (ctrlPressed && shiftPressed) {
            switch (kbStruct->vkCode) {
            case 'P':
                QMetaObject::invokeMethod(s_instance, "takeStealthPhoto", Qt::QueuedConnection);
                QMetaObject::invokeMethod(s_instance, "hotkeyPressed", Qt::QueuedConnection,
                                          Q_ARG(QString, "Stealth Photo"));
                break;

            case 'R':
                QMetaObject::invokeMethod(s_instance, "startStealthRecording", Qt::QueuedConnection);
                QMetaObject::invokeMethod(s_instance, "hotkeyPressed", Qt::QueuedConnection,
                                          Q_ARG(QString, "Start Stealth Recording"));
                break;

            case 'S':
                QMetaObject::invokeMethod(s_instance, "stopStealthRecording", Qt::QueuedConnection);
                QMetaObject::invokeMethod(s_instance, "hotkeyPressed", Qt::QueuedConnection,
                                          Q_ARG(QString, "Stop Stealth Recording"));
                break;

            case 'H':
                QMetaObject::invokeMethod(s_instance, "toggleStealthMode", Qt::QueuedConnection);
                QMetaObject::invokeMethod(s_instance, "hotkeyPressed", Qt::QueuedConnection,
                                          Q_ARG(QString, "Toggle Window"));
                break;
            }
        }
    }

    return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
}
#endif

void CameraManager::installGlobalHotkeys()
{
#ifdef Q_OS_WIN
    if (!s_keyboardHook) {
        s_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc,
                                          GetModuleHandle(NULL), 0);
    }
#endif
}

void CameraManager::uninstallGlobalHotkeys()
{
#ifdef Q_OS_WIN
    if (s_keyboardHook) {
        UnhookWindowsHookEx(s_keyboardHook);
        s_keyboardHook = nullptr;
    }
#endif
}

void CameraManager::handleCameraActiveChanged(bool active)
{
    m_cameraActive = active;
    emit cameraActiveChanged();

    if (active) {
        emit cameraDetected();
    }
}

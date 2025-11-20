#include "UsbManager.h"
#include <QGuiApplication>
#include <QWindow>
#include <QDebug>
#include <QStorageInfo>

#ifdef Q_OS_WIN
#include <initguid.h>
// Определяем GUID для HID-устройств
DEFINE_GUID(GUID_DEVCLASS_HID, 0x745a17a0, 0x74d3, 0x11d0, 0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda);
#endif

UsbManager::UsbManager(QObject *parent) : QObject(parent), m_notifyHandle(nullptr)
{
    m_rescanTimer = new QTimer(this);
    m_rescanTimer->setInterval(500);
    m_rescanTimer->setSingleShot(true);
    connect(m_rescanTimer, &QTimer::timeout, this, &UsbManager::rescanDevices);

    QWindow* window = new QWindow();
    window->create();
    m_windowId = window->winId();
    qApp->installNativeEventFilter(this);
    DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
    ZeroMemory(&notificationFilter, sizeof(notificationFilter));
    notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    m_notifyHandle = RegisterDeviceNotification(reinterpret_cast<HWND>(m_windowId), &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
    if (!m_notifyHandle) {
        emit logMessage("Error: Could not register for device notifications.");
    }
}

UsbManager::~UsbManager()
{
    if (m_notifyHandle) {
        UnregisterDeviceNotification(m_notifyHandle);
    }
    qApp->removeNativeEventFilter(this);
}

void UsbManager::initialScan()
{
    rescanDevices();
}

bool UsbManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_DEVICECHANGE) {
            if (msg->wParam == DBT_DEVICEARRIVAL || msg->wParam == DBT_DEVICEREMOVECOMPLETE) {
                m_rescanTimer->start();
            }
        }
    }
    return false;
}

void UsbManager::rescanDevices()
{
    emit logMessage("Rescanning all USB bus devices...");
    m_devices.clear();

    // --- Шаг 1: Получаем ВСЕ устройства на шине USB ---
    HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, L"USB", NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        emit logMessage("Error: Could not get device info list for USB bus.");
        return;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
        wchar_t devName[256] = {0};
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)devName, sizeof(devName), NULL) ||
            SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)devName, sizeof(devName), NULL))
        {
            QString displayName = QString::fromWCharArray(devName);

            // Фильтруем только самый низкоуровневый "мусор", остальное оставляем
            if (displayName.contains("Host Controller", Qt::CaseInsensitive) || displayName.contains("Root Hub", Qt::CaseInsensitive)) {
                continue;
            }

            QVariantMap details;
            details["id"] = displayName; // Временный ID
            details["displayName"] = displayName;
            details["isEjectable"] = false; // По умолчанию все неизвлекаемое
            addDevice(details);
        }
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);


    // --- Шаг 2: "Обогащаем" флешки информацией о дисках ---
    const auto drives = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &drive : drives) {
        QString root = drive.rootPath();
        if (root.isEmpty()) continue;
        std::wstring drivePathW = root.toStdWString();
        drivePathW[drivePathW.length() - 1] = L'\\';
        UINT driveType = GetDriveTypeW(drivePathW.c_str());

        if (driveType == DRIVE_REMOVABLE) {
            QString driveLetter = root.left(2);
            QString driveName = drive.name();

            // Ищем в нашем списке устройство, которое является этой флешкой.
            // Обычно флешка определяется как "Запоминающее устройство" или "Mass Storage".
            for (int i = 0; i < m_devices.size(); ++i) {
                QVariantMap deviceMap = m_devices[i].toMap();
                QString currentName = deviceMap["displayName"].toString();

                if (currentName.contains("Storage", Qt::CaseInsensitive) || currentName.contains("Запоминающее", Qt::CaseInsensitive))
                {
                    // Нашли! Обновляем информацию.
                    deviceMap["isEjectable"] = true;
                    deviceMap["id"] = root; // Правильный ID для извлечения
                    deviceMap["displayName"] = QString("%1 (%2)").arg(driveName.isEmpty() ? "Removable Drive" : driveName).arg(driveLetter);
                    m_devices[i] = deviceMap; // Заменяем старую запись на новую
                    break;
                }
            }
        }
    }

    emit devicesChanged();
}

void UsbManager::addDevice(const QVariantMap &deviceDetails)
{
    for (const QVariant& device : m_devices) {
        if (device.toMap().value("id").toString() == deviceDetails.value("id").toString()) {
            return;
        }
    }
    m_devices.append(deviceDetails);
    emit logMessage(QString("Device found: %1").arg(deviceDetails["displayName"].toString()));
}

void UsbManager::ejectDevice(const QString &deviceId)
{
    QString driveLetter = deviceId.left(2);
    if (driveLetter.length() != 2 || !driveLetter.endsWith(':')) {
        emit logMessage("Eject error: Invalid device ID for ejection. Expected a drive path like 'E:/'.");
        return;
    }

    QString volumePath = "\\\\.\\" + driveLetter;
    DWORD dwBytesReturned;
    HANDLE hVolume = CreateFileW((LPCWSTR)volumePath.utf16(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hVolume == INVALID_HANDLE_VALUE) {
        emit logMessage("Eject failed: Could not get handle for " + driveLetter);
        return;
    }

    if (!DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
        emit logMessage("Eject failed: Could not lock volume " + driveLetter);
        CloseHandle(hVolume);
        return;
    }

    if (!DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
        emit logMessage("Eject failed: Could not dismount volume " + driveLetter);
        CloseHandle(hVolume);
        return;
    }

    if (!DeviceIoControl(hVolume, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
        emit logMessage("Eject refused: Device " + driveLetter + " may be in use.");
        DeviceIoControl(hVolume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
        CloseHandle(hVolume);
        return;
    }

    emit logMessage("Successfully ejected " + driveLetter);
    CloseHandle(hVolume);

    m_rescanTimer->start();
}

QVariantList UsbManager::devices() const
{
    return m_devices;
}

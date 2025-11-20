#include "UsbManager.h"
#include <QGuiApplication>
#include <QWindow>
#include <QDebug>
#include <QStorageInfo>
#include <QSet>

#ifdef Q_OS_WIN
#include <initguid.h>
DEFINE_GUID(GUID_DEVCLASS_HID, 0x745a17a0, 0x74d3, 0x11d0, 0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda);
DEFINE_GUID(GUID_DEVCLASS_MOUSE, 0x4d36e96f, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);
DEFINE_GUID(GUID_DEVCLASS_KEYBOARD, 0x4d36e96b, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);
#include <cfgmgr32.h>
#pragma comment(lib, "cfgmgr32.lib")
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

    m_notifyHandle = RegisterDeviceNotification(
        reinterpret_cast<HWND>(m_windowId),
        &notificationFilter,
        DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES
        );

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
            if (msg->wParam == DBT_DEVICEARRIVAL) {
                emit logMessage("USB device connected");
                m_rescanTimer->start();
            } else if (msg->wParam == DBT_DEVICEREMOVECOMPLETE) {
                emit logMessage("USB device disconnected");
                m_rescanTimer->start();
            }
        }
    }
    return false;
}

bool UsbManager::isExternalUSBDevice(HDEVINFO hDevInfo, SP_DEVINFO_DATA* devInfoData)
{
    // Получаем информацию о родительском устройстве
    DWORD dwSize = 0;
    wchar_t szDeviceInstanceId[MAX_DEVICE_ID_LEN];

    if (!SetupDiGetDeviceInstanceIdW(hDevInfo, devInfoData, szDeviceInstanceId,
                                     MAX_DEVICE_ID_LEN, &dwSize)) {
        return false;
    }

    QString deviceId = QString::fromWCharArray(szDeviceInstanceId).toUpper();

    // Проверяем, что это USB устройство по VID/PID
    if (!deviceId.contains("USB\\VID_") && !deviceId.contains("HID\\VID_")) {
        return false;
    }

    // Исключаем встроенные устройства по известным идентификаторам
    QStringList internalDevices = {
        "VID_0000", "VID_FFFF",  // Системные
        "VID_8087",  // Intel встроенные
        "VID_0A5C", "VID_0CF3",  // Встроенный Bluetooth
    };

    for (const QString& internal : internalDevices) {
        if (deviceId.contains(internal)) {
            return false;
        }
    }

    return true;
}

void UsbManager::rescanDevices()
{
    emit logMessage("Scanning USB devices...");
    m_devices.clear();

    // --- ЧАСТЬ А: Поиск флешек (съемных дисков) ---
    const auto drives = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &drive : drives) {
        QString root = drive.rootPath();
        if (root.isEmpty() || root.length() < 2) continue;

        // Проверяем, что диск готов
        if (!drive.isReady() || !drive.isValid()) {
            continue;
        }

        std::wstring drivePathW = root.toStdWString();
        if (!drivePathW.empty() && drivePathW.back() != L'\\') {
            drivePathW += L'\\';
        }

        UINT driveType = GetDriveTypeW(drivePathW.c_str());

        // Проверяем, что это съемный диск и у него есть размер
        if (driveType == DRIVE_REMOVABLE && root.at(0).isLetter() && drive.bytesTotal() > 0) {
            QVariantMap details;
            details["id"] = root;

            QString volumeName = drive.name();
            if (volumeName.isEmpty()) {
                volumeName = "Съемный диск";
            }

            // Информация о размере и занятом месте
            qint64 totalSize = drive.bytesTotal();
            qint64 usedSize = totalSize - drive.bytesAvailable();

            QString sizeStr;
            if (totalSize > 0) {
                double totalGB = totalSize / (1024.0 * 1024.0 * 1024.0);
                double usedGB = usedSize / (1024.0 * 1024.0 * 1024.0);
                sizeStr = QString("%1/%2 GB").arg(usedGB, 0, 'f', 1).arg(totalGB, 0, 'f', 1);
            }

            QString displayName = QString("%1 (%2)").arg(volumeName).arg(root.left(2));
            if (!sizeStr.isEmpty()) {
                displayName += " - " + sizeStr;
            }

            details["displayName"] = displayName;
            details["isEjectable"] = true;
            details["type"] = "USB Flash Drive";
            details["driveLetter"] = root.left(2);

            addDevice(details);
            emit logMessage(QString("Found USB Flash Drive: %1").arg(displayName));
        }
    }

    // --- ЧАСТЬ Б: Поиск внешних мышей ---
    QSet<QString> foundDevices; // Для предотвращения дубликатов

    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_MOUSE, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE) {
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
            if (!isExternalUSBDevice(hDevInfo, &devInfoData)) {
                continue;
            }

            wchar_t devName[256];
            if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME,
                                                   NULL, (PBYTE)devName, sizeof(devName), NULL)) {
                if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DEVICEDESC,
                                                       NULL, (PBYTE)devName, sizeof(devName), NULL)) {
                    continue;
                }
            }

            QString deviceName = QString::fromWCharArray(devName);

            // Пропускаем, если уже добавили
            if (foundDevices.contains(deviceName)) {
                continue;
            }
            foundDevices.insert(deviceName);

            // Получаем Instance ID для операции отключения
            wchar_t instanceId[MAX_DEVICE_ID_LEN];
            if (SetupDiGetDeviceInstanceIdW(hDevInfo, &devInfoData, instanceId,
                                            MAX_DEVICE_ID_LEN, NULL)) {

                QVariantMap details;
                details["id"] = QString::fromWCharArray(instanceId);
                details["displayName"] = "USB Mouse: " + deviceName;
                details["isEjectable"] = true;  // Делаем мышь отключаемой
                details["type"] = "USB Mouse";
                details["deviceInstanceId"] = QString::fromWCharArray(instanceId);

                addDevice(details);
                emit logMessage(QString("Found USB Mouse: %1").arg(deviceName));
            }
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    // --- ЧАСТЬ В: Поиск внешних клавиатур ---
    hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_KEYBOARD, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE) {
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
            if (!isExternalUSBDevice(hDevInfo, &devInfoData)) {
                continue;
            }

            wchar_t devName[256];
            if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME,
                                                   NULL, (PBYTE)devName, sizeof(devName), NULL)) {
                if (!SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DEVICEDESC,
                                                       NULL, (PBYTE)devName, sizeof(devName), NULL)) {
                    continue;
                }
            }

            QString deviceName = QString::fromWCharArray(devName);

            if (foundDevices.contains(deviceName)) {
                continue;
            }
            foundDevices.insert(deviceName);

            wchar_t instanceId[MAX_DEVICE_ID_LEN];
            if (SetupDiGetDeviceInstanceIdW(hDevInfo, &devInfoData, instanceId,
                                            MAX_DEVICE_ID_LEN, NULL)) {

                QVariantMap details;
                details["id"] = QString::fromWCharArray(instanceId);
                details["displayName"] = "USB Keyboard: " + deviceName;
                details["isEjectable"] = true;
                details["type"] = "USB Keyboard";
                details["deviceInstanceId"] = QString::fromWCharArray(instanceId);

                addDevice(details);
                emit logMessage(QString("Found USB Keyboard: %1").arg(deviceName));
            }
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    emit devicesChanged();

    if (m_devices.isEmpty()) {
        emit logMessage("No external USB devices found");
    }
}

void UsbManager::addDevice(const QVariantMap &deviceDetails)
{
    for (const QVariant& device : m_devices) {
        if (device.toMap().value("id").toString() == deviceDetails.value("id").toString()) {
            return;
        }
    }
    m_devices.append(deviceDetails);
}

void UsbManager::ejectDevice(const QString &deviceId)
{
    // Находим устройство в списке
    QVariantMap deviceInfo;
    for (const QVariant& device : m_devices) {
        if (device.toMap().value("id").toString() == deviceId) {
            deviceInfo = device.toMap();
            break;
        }
    }

    QString deviceType = deviceInfo.value("type").toString();

    if (deviceType == "USB Flash Drive") {
        // Извлечение флешки
        ejectFlashDrive(deviceId);
    } else if (deviceType == "USB Mouse" || deviceType == "USB Keyboard") {
        // Отключение HID устройства
        QString instanceId = deviceInfo.value("deviceInstanceId").toString();
        disableHIDDevice(instanceId);
    }
}

void UsbManager::ejectFlashDrive(const QString &deviceId)
{
    QString driveLetter = deviceId.left(2);
    if (driveLetter.length() < 2 || !driveLetter.at(0).isLetter()) {
        emit logMessage("Error: Invalid device ID for ejection");
        return;
    }

    emit logMessage(QString("Attempting to eject %1...").arg(driveLetter));

    QString volumePath = "\\\\.\\" + driveLetter;
    DWORD dwBytesReturned;

    HANDLE hVolume = CreateFileW(
        (LPCWSTR)volumePath.utf16(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    if (hVolume == INVALID_HANDLE_VALUE) {
        emit logMessage(QString("Eject failed: Could not open volume %1").arg(driveLetter));
        return;
    }

    // Блокируем том
    if (!DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
        emit logMessage(QString("Eject failed: Volume %1 is in use. Close all programs using this drive.")
                            .arg(driveLetter));
        CloseHandle(hVolume);
        return;
    }

    // Демонтируем том
    if (!DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
        emit logMessage(QString("Eject failed: Could not dismount volume %1").arg(driveLetter));
        DeviceIoControl(hVolume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
        CloseHandle(hVolume);
        return;
    }

    // Извлекаем устройство
    PREVENT_MEDIA_REMOVAL pmr = {FALSE};
    DeviceIoControl(hVolume, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(pmr),
                    NULL, 0, &dwBytesReturned, NULL);

    if (!DeviceIoControl(hVolume, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
        emit logMessage(QString("Eject refused: Device %1 cannot be ejected.")
                            .arg(driveLetter));
        DeviceIoControl(hVolume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
        CloseHandle(hVolume);
        return;
    }

    emit logMessage(QString("✓ Successfully ejected %1. You can safely remove the device.").arg(driveLetter));
    CloseHandle(hVolume);

    // Сразу обновляем список
    QTimer::singleShot(100, this, &UsbManager::rescanDevices);
}

void UsbManager::disableHIDDevice(const QString &instanceId)
{
    emit logMessage(QString("Attempting to disable device..."));

    // Получаем информацию об устройстве
    DEVINST devInst;
    CONFIGRET cr = CM_Locate_DevNodeW(&devInst, (DEVINSTID_W)instanceId.toStdWString().c_str(),
                                      CM_LOCATE_DEVNODE_NORMAL);

    if (cr != CR_SUCCESS) {
        emit logMessage("Failed to locate device node");
        return;
    }

    // Пытаемся отключить устройство
    cr = CM_Request_Device_EjectW(devInst, nullptr, nullptr, 0, 0);

    if (cr == CR_SUCCESS) {
        emit logMessage("✓ Device disabled successfully. You can unplug it.");
        QTimer::singleShot(100, this, &UsbManager::rescanDevices);
    } else {
        // Альтернативный метод - изменение состояния устройства
        cr = CM_Disable_DevNode(devInst, 0);
        if (cr == CR_SUCCESS) {
            emit logMessage("✓ Device disabled successfully.");
            // Включаем обратно через 2 секунды
            QTimer::singleShot(2000, [this, devInst]() {
                CM_Enable_DevNode(devInst, 0);
                emit logMessage("Device re-enabled.");
                rescanDevices();
            });
        } else {
            emit logMessage("Failed to disable device. It may require administrator privileges.");
        }
    }
}

QVariantList UsbManager::devices() const
{
    return m_devices;
}

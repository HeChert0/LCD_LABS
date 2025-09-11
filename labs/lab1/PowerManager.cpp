#include "PowerManager.h"
#include <Windows.h>
#include <powrprof.h>
#include <SetupAPI.h>
#include <devguid.h>
#include <batclass.h>

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Powrprof.lib")

#ifdef QT_NO_DEBUG
#undef QT_NO_DEBUG
#endif

static const GUID GUID_DEVCLASS_BATTERY = { 0x72631e54, 0x78a4, 0x11d0, { 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a } };

PowerManager::PowerManager(QObject *parent) : QObject(parent)
{
    m_powerStatus = new SYSTEM_POWER_STATUS();

    queryBatteryType();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PowerManager::updatePowerInfo);
    m_timer->start(1000);

    updatePowerInfo();
}

void PowerManager::updatePowerInfo()
{
    if (GetSystemPowerStatus(m_powerStatus)) {
        emit powerInfoChanged();
    }
}

QString PowerManager::powerSourceType() const
{
    switch (m_powerStatus->ACLineStatus) {
    case 1: return "От сети";
    case 0: return "От батареи";
    default: return "Неизвестно";
    }
}

int PowerManager::batteryLevel() const
{
    return (m_powerStatus->BatteryLifePercent == 255) ? -1 : m_powerStatus->BatteryLifePercent;
}

QString PowerManager::powerSavingMode() const
{
    if (m_powerStatus->SystemStatusFlag == 1) {
        return "Включен";
    }
    if (m_powerStatus->ACLineStatus == 1) {
        return "Выключен (питание от сети)";
    }
    return "Выключен";
}

QString PowerManager::batteryFullLifeTime() const
{
    if (m_powerStatus->BatteryFullLifeTime == (DWORD)-1) {
        return "Неизвестно";
    }
    int totalSeconds = m_powerStatus->BatteryFullLifeTime;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    return QString("%1 ч %2 мин").arg(hours).arg(minutes);
}

QString PowerManager::batteryLifeTime() const
{
    if (m_powerStatus->BatteryLifeTime == (DWORD)-1) {
        return "Неизвестно";
    }
    int remainingSeconds = m_powerStatus->BatteryLifeTime;
    int hours = remainingSeconds / 3600;
    int minutes = (remainingSeconds % 3600) / 60;
    return QString("%1 ч %2 мин").arg(hours).arg(minutes);
}


QString PowerManager::batteryType() const
{
    return m_batteryType;
}

void PowerManager::sleep()
{
    SetSuspendState(false, true, false);
}

void PowerManager::hibernate()
{
    SetSuspendState(true, true, true);
}

void PowerManager::queryBatteryType()
{
    HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE); //список
    if (hdev == INVALID_HANDLE_VALUE) {
        m_batteryType = "Доступ к устройствам не удался";
        return;
    }
    SP_DEVICE_INTERFACE_DATA did = {0};
    did.cbSize = sizeof(did);

    if (SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVCLASS_BATTERY, 0, &did)) { //пол интерфейсы
        DWORD cbRequired = 0;
        SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0); //размер буфера
        if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
            PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
            if (pdidd) {
                pdidd->cbSize = sizeof(*pdidd);
                if (SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, 0)) {
                    HANDLE hBattery = CreateFile(pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); // сама батарея
                    if (hBattery != INVALID_HANDLE_VALUE) {
                        BATTERY_QUERY_INFORMATION bqi = {0};
                        DWORD dwWait = 0, dwOut;
                        if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(dwWait), &bqi.BatteryTag, sizeof(bqi.BatteryTag), &dwOut, NULL) && bqi.BatteryTag) {
                            BATTERY_INFORMATION bi = {0};
                            bqi.InformationLevel = BatteryInformation;
                            if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), &bi, sizeof(bi), &dwOut, NULL)) {
                                char chem[5] = {0};
                                memcpy(chem, bi.Chemistry, 4);
                                m_batteryType = QString(chem);
                            }
                        }
                        CloseHandle(hBattery);
                    }
                }
                LocalFree(pdidd);
            }
        }
    }
    SetupDiDestroyDeviceInfoList(hdev);
    if (m_batteryType.isEmpty()) {
        m_batteryType = "Тип неизвестен";
    }
}




/*
HDEVINFO hdev = SetupDiGetClassDevs(
    &GUID_DEVCLASS_BATTERY,  // GUID класса устройств (батареи)
    0,                       // Enumerator (NULL = все)
    0,                       // Родительское окно (NULL)
    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE  // Флаги
DIGCF_PRESENT - только устройства, присутствующие в системе сейчас

DIGCF_DEVICEINTERFACE - включать интерфейсы устройств
); */

/*
BOOL success = SetupDiEnumDeviceInterfaces(
    hdev,                   // Дескриптор из SetupDiGetClassDevs
    0,                      // DeviceInfoData (NULL = первое устройство)
    &GUID_DEVCLASS_BATTERY, // GUID класса устройств
    0,                      // Index (0 = первое устройство)
    &did                    // Выходная структура с данными интерфейса
);*/

/*
SetupDiGetDeviceInterfaceDetail( Определяет размер буфера, необходимый для хранения детальной информации
    hdev,                   // Дескриптор набора устройств
    &did,                   // Структура интерфейса из предыдущего вызова
    0,                      // Output buffer (NULL = только получить размер)
    0,                      // Output buffer size (0)
    &cbRequired,            // Выход: требуемый размер буфера
    0                       // DeviceInfoData (NULL)
);8*/

// SetupDiDestroyDeviceInfoList  Освобождает системные ресурсы, связанные с дескриптором HDEVINFO

/*
typedef struct _SP_DEVICE_INTERFACE_DATA {
    DWORD cbSize;           // размер структуры
    GUID  InterfaceClassGuid; // GUID класса интерфейса
    DWORD Flags;            // флаги например, SPINT_ACTIVE для активных интерфейсов
    ULONG_PTR Reserved;     // зарезервировано
} SP_DEVICE_INTERFACE_DATA, *PSP_DEVICE_INTERFACE_DATA;*/


/*typedef struct _SP_DEVICE_INTERFACE_DETAIL_DATA {
    DWORD cbSize;           // Размер структуры
    TCHAR DevicePath[ANYSIZE_ARRAY]; // Путь к устройству
} SP_DEVICE_INTERFACE_DETAIL_DATA, *PSP_DEVICE_INTERFACE_DETAIL_DATA;
содержит детальную информацию об интерфейсе устройства, включая путь к устройству.

Структура:
 */

/*
 * typedef struct _BATTERY_QUERY_INFORMATION {
    BATTERY_QUERY_INFORMATION_LEVEL InformationLevel;
    ULONG                           AtRate;
    ULONG                           Reserved[3];
} BATTERY_QUERY_INFORMATION, *PBATTERY_QUERY_INFORMATION;
Структура для запроса информации о батарее через DeviceIoControl() с IOCTL кодом IOCTL_BATTERY_QUERY_INFORMATION.
InformationLevel - тип запрашиваемой информации:

BatteryInformation - основная информация

BatteryEstimatedTime - оставшееся время

BatteryDeviceName - имя устройства

BatteryManufactureDate - дата производства

BatteryManufactureName - производитель

BatterySerialNumber - серийный номер

GENERIC_READ | GENERIC_WRITE: Запрашивает права на чтение и запись (необходимы для отправки IOCTL-запросов).

FILE_SHARE_READ | FILE_SHARE_WRITE: Позволяет другим процессам также открывать это устройство.

OPEN_EXISTING: Открывает существующее устройство (не создает новое).

Это низкоуровневый доступ к драйверу батареи.
 */

/*
 * typedef struct _BATTERY_INFORMATION { Основная структура с информацией о состоянии и capabilities батареи
    ULONG Capabilities;     // Возможности батареи
    UCHAR Technology;       // Технология: 0 = не известно, 1 = PbAc, 2 = NiCd, etc.
    UCHAR Reserved[3];      // Зарезервировано
    ULONG DesignedCapacity; // Проектная емкость (mWh)
    ULONG FullChargedCapacity; // Полная емкость при 100% заряде (mWh)
    ULONG DefaultAlert1;    // Уровень предупреждения 1 (mWh)
    ULONG DefaultAlert2;    // Уровень предупреждения 2 (mWh)
    ULONG CriticalBias;     // Критическое смещение
    ULONG CycleCount;       // Количество циклов зарядки
} BATTERY_INFORMATION, *PBATTERY_INFORMATION;

Capabilities (флаги):
BATTERY_SYSTEM_BATTERY - системная батарея

BATTERY_CAPACITY_RELATIVE - емкость в относительных единицах

BATTERY_IS_SHORT_TERM - кратковременная батарея

BATTERY_SET_CHARGE_SUPPORTED - поддержка установки уровня заряда

BATTERY_SET_DISCHARGE_SUPPORTED - поддержка установки уровня разряда
 */

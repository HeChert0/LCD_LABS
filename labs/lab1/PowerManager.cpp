// labs/lab1/PowerManager.cpp
#include "PowerManager.h"
#include <Windows.h>
#include <powrprof.h>   // Для SetSuspendState и функций схем питания
#include <SetupAPI.h>
#include <devguid.h>    // Для GUID_DEVCLASS_BATTERY
#include <batclass.h>   // Для IOCTL_BATTERY... и структур

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Powrprof.lib")

// Windows.h переопределяет некоторые вещи, поэтому undef'иним их
#ifdef QT_NO_DEBUG
#undef QT_NO_DEBUG
#endif

// Определяем GUID для класса батарей. Он нужен для поиска устройств.
static const GUID GUID_DEVCLASS_BATTERY = { 0x72631e54, 0x78a4, 0x11d0, { 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a } };

PowerManager::PowerManager(QObject *parent) : QObject(parent)
{
    m_powerStatus = new SYSTEM_POWER_STATUS();

    // Динамически получаем тип батареи при запуске, используя WinAPI
    queryBatteryType();

    // Запускаем таймер, который будет обновлять остальную информацию каждую секунду
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PowerManager::updatePowerInfo);
    m_timer->start(1000);

    // Вызываем обновление сразу, чтобы при запуске были актуальные данные
    updatePowerInfo();
}

/**
 * @brief Обновляет информацию о состоянии питания.
 * Вызывается по таймеру каждую секунду.
 */
void PowerManager::updatePowerInfo()
{
    // Получаем основную информацию о питании
    if (GetSystemPowerStatus(m_powerStatus)) {
        // --- НОВЫЙ КОД ДЛЯ СХЕМЫ ЭЛЕКТРОПИТАНИЯ ---
        GUID *activePolicyGuid;
        // Запрашиваем GUID активной схемы
        if (PowerGetActiveScheme(NULL, &activePolicyGuid) == ERROR_SUCCESS) {
            DWORD bufferSize = 0;
            // Запрашиваем размер имени схемы
            if (PowerReadFriendlyName(NULL, activePolicyGuid, NULL, NULL, NULL, &bufferSize) == ERROR_SUCCESS && bufferSize > 0) {
                // Выделяем память и получаем имя
                std::wstring friendlyName(bufferSize / sizeof(wchar_t), L'\0');
                if (PowerReadFriendlyName(NULL, activePolicyGuid, NULL, NULL, (PUCHAR)&friendlyName[0], &bufferSize) == ERROR_SUCCESS) {
                    // Убираем нулевые символы в конце, если они есть
                    friendlyName.erase(friendlyName.find_last_not_of(L'\0') + 1);
                    m_windowsPowerPlan = QString::fromStdWString(friendlyName);
                }
            }
            LocalFree(activePolicyGuid); // Освобождаем память, выделенную системой
        } else {
            m_windowsPowerPlan = "Неизвестно";
        }
        // -----------------------------------------

        emit powerInfoChanged();
    }
}

/**
 * @brief Определяет тип источника питания (сеть/батарея).
 * @return QString с описанием источника.
 */
QString PowerManager::powerSourceType() const
{
    switch (m_powerStatus->ACLineStatus) {
    case 1: return "От сети";
    case 0: return "От батареи";
    default: return "Неизвестно";
    }
}

/**
 * @brief Возвращает тип батареи.
 * Это значение получается один раз при запуске через queryBatteryType().
 * @return QString с типом батареи.
 */
/**
 * @brief Возвращает текущий уровень заряда батареи в процентах.
 * @return int от 0 до 100, или -1 если данные недоступны.
 */
int PowerManager::batteryLevel() const
{
    return (m_powerStatus->BatteryLifePercent == 255) ? -1 : m_powerStatus->BatteryLifePercent;
}

/**
 * @brief Определяет, включен ли режим экономии энергии.
 * @return QString с состоянием режима.
 */
QString PowerManager::powerSavingMode() const
{
    // Согласно документации Microsoft, флаг SystemStatusFlag равен 1,
    // если режим экономии заряда включен (для Windows 10 и новее).
    if (m_powerStatus->SystemStatusFlag == 1) {
        return "Включен";
    }
    // Режим также считается выключенным, если питание от сети.
    if (m_powerStatus->ACLineStatus == 1) {
        return "Выключен (питание от сети)";
    }
    return "Выключен";
}

/**
 * @brief Возвращает расчетное время работы от полностью заряженной батареи.
 * @return QString с временем или "Неизвестно".
 */
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

/**
 * @brief Возвращает оставшееся время работы от батареи.
 * @return QString с временем или "Неизвестно".
 */
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


/**
 * @brief Возвращает тип батареи (химический состав).
 */
QString PowerManager::batteryType() const
{
    return m_batteryType;
}

/**
 * @brief Возвращает имя активной схемы электропитания Windows.
 */
QString PowerManager::windowsPowerPlan() const
{
    return m_windowsPowerPlan;
}

/**
 * @brief Переводит систему в спящий режим (Sleep).
 */
void PowerManager::sleep()
{
    // false - сон, true - гибернация
    SetSuspendState(false, true, true);
}

/**
 * @brief Переводит систему в режим гибернации (Hibernate).
 */
void PowerManager::hibernate()
{
    SetSuspendState(true, true, true);
}

/**
 * @brief Опрашивает систему для получения типа батареи, используя SetupAPI.
 * Эта функция реализует обязательное требование лабораторной работы.
 */
void PowerManager::queryBatteryType()
{
    // Получаем "контейнер" для информации о батареях
    HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hdev == INVALID_HANDLE_VALUE) {
        m_batteryType = "Доступ к устройствам не удался";
        return;
    }

    SP_DEVICE_INTERFACE_DATA did = {0};
    did.cbSize = sizeof(did);

    // Перебираем интерфейсы устройств-батарей
    if (SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVCLASS_BATTERY, 0, &did)) {
        DWORD cbRequired = 0;
        // Получаем необходимый размер буфера для детальной информации
        SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0);
        if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
            PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
            if (pdidd) {
                pdidd->cbSize = sizeof(*pdidd);
                // Получаем детальную информацию, включая путь к устройству
                if (SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, 0)) {
                    // Открываем файл устройства для отправки IOCTL команд
                    HANDLE hBattery = CreateFile(pdidd->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hBattery != INVALID_HANDLE_VALUE) {
                        BATTERY_QUERY_INFORMATION bqi = {0};
                        DWORD dwWait = 0, dwOut;

                        // Запрашиваем тэг батареи
                        if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(dwWait), &bqi.BatteryTag, sizeof(bqi.BatteryTag), &dwOut, NULL) && bqi.BatteryTag) {
                            BATTERY_INFORMATION bi = {0};
                            bqi.InformationLevel = BatteryInformation;
                            // Запрашиваем основную информацию о батарее
                            if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), &bi, sizeof(bi), &dwOut, NULL)) {
                                // Поле Chemistry содержит 4 символа, описывающих тип
                                // e.g. "LION", "NiCd"
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

    // Если после всех операций тип не определился
    if (m_batteryType.isEmpty()) {
        m_batteryType = "Тип неизвестен";
    }
}

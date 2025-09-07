// labs/lab1/PowerManager.cpp
#include "PowerManager.h"
#include <Windows.h>
#include <powrprof.h> // для SetSuspendState

// Windows.h переопределяет некоторые вещи, поэтому undef'иним их
#ifdef QT_NO_DEBUG
#undef QT_NO_DEBUG
#endif

PowerManager::PowerManager(QObject *parent) : QObject(parent)
{
    m_powerStatus = new SYSTEM_POWER_STATUS();

    // Запускаем таймер, который будет обновлять информацию каждую секунду
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PowerManager::updatePowerInfo);
    m_timer->start(1000);

    // Получаем тип батареи один раз при запуске (он не меняется)
    // Здесь мы могли бы использовать сложный код с SetupDiGetDeviceRegistryPropertyA,
    // но для простоты пока оставим заглушку.
    // Реальная реализация потребует парсинга данных из реестра.
    m_batteryType = "Литий-ионная (Li-ion)"; // Пример

    updatePowerInfo(); // Первоначальное обновление
}

void PowerManager::updatePowerInfo()
{
    if (GetSystemPowerStatus(m_powerStatus)) {
        emit powerInfoChanged();
    }
}

QString PowerManager::powerSourceType() const
{
    if (m_powerStatus->ACLineStatus == 1) {
        return "От сети";
    } else if (m_powerStatus->ACLineStatus == 0) {
        return "От батареи";
    }
    return "Неизвестно";
}

QString PowerManager::batteryType() const
{
    // Это значение мы могли бы получить через SetupDiGetDeviceRegistryPropertyA
    return m_batteryType;
}

int PowerManager::batteryLevel() const
{
    // 255 означает, что уровень неизвестен
    return (m_powerStatus->BatteryLifePercent == 255) ? -1 : m_powerStatus->BatteryLifePercent;
}

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

// Реализация методов управления
void PowerManager::sleep()
{
    // false - сон, true - гибернация
    SetSuspendState(false, false, false);
}

void PowerManager::hibernate()
{
    SetSuspendState(true, false, false);
}

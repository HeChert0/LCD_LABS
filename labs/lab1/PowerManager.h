// labs/lab1/PowerManager.h
#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <QObject>
#include <QTimer>

typedef struct _SYSTEM_POWER_STATUS SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

class PowerManager : public QObject
{
    Q_OBJECT
    // Свойства которые будут доступны в QML
    Q_PROPERTY(QString powerSourceType READ powerSourceType NOTIFY powerInfoChanged)
    Q_PROPERTY(QString batteryType READ batteryType NOTIFY powerInfoChanged)
    Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY powerInfoChanged)
    Q_PROPERTY(QString powerSavingMode READ powerSavingMode NOTIFY powerInfoChanged)
    Q_PROPERTY(QString batteryFullLifeTime READ batteryFullLifeTime NOTIFY powerInfoChanged)
    Q_PROPERTY(QString batteryLifeTime READ batteryLifeTime NOTIFY powerInfoChanged)

public:
    explicit PowerManager(QObject *parent = nullptr);

    // Методы которые можно будет вызывать из QML
    Q_INVOKABLE void sleep();
    Q_INVOKABLE void hibernate();

    QString powerSourceType() const;
    QString batteryType() const;
    int batteryLevel() const;
    QString powerSavingMode() const;
    QString batteryFullLifeTime() const;
    QString batteryLifeTime() const;

signals:
    // Сигнал для QML что данные обновились
    void powerInfoChanged();

private slots:
    // Слот для периодического обновления информации
    void updatePowerInfo();

private:
    QTimer *m_timer;
    SYSTEM_POWER_STATUS *m_powerStatus; // Указатель на структуру с данными
    QString m_batteryType;
};

#endif // POWERMANAGER_H

#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <QObject>
#include <QTimer>

typedef struct _SYSTEM_POWER_STATUS SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

class PowerManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString powerSourceType READ powerSourceType NOTIFY powerInfoChanged)
    Q_PROPERTY(QString batteryType READ batteryType NOTIFY powerInfoChanged)
    Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY powerInfoChanged)
    Q_PROPERTY(QString powerSavingMode READ powerSavingMode NOTIFY powerInfoChanged)
    Q_PROPERTY(QString batteryFullLifeTime READ batteryFullLifeTime NOTIFY powerInfoChanged)
    Q_PROPERTY(QString batteryLifeTime READ batteryLifeTime NOTIFY powerInfoChanged)

public:
    explicit PowerManager(QObject *parent = nullptr);

    Q_INVOKABLE void sleep();
    Q_INVOKABLE void hibernate();

    QString powerSourceType() const;
    QString batteryType() const;
    int batteryLevel() const;
    QString powerSavingMode() const;
    QString batteryFullLifeTime() const;
    QString batteryLifeTime() const;

signals:
    void powerInfoChanged();

private slots:
    void updatePowerInfo();

private:
    void queryBatteryType();

    QTimer *m_timer;
    SYSTEM_POWER_STATUS *m_powerStatus;
    QString m_batteryType;
};

#endif // POWERMANAGER_H

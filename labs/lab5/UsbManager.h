#ifndef USBMANAGER_H
#define USBMANAGER_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QVariantList>
#include <QTimer>
#include <qwindowdefs.h> // Для WId

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbt.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#endif

class UsbManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)

public:
    explicit UsbManager(QObject *parent = nullptr);
    ~UsbManager();

    Q_INVOKABLE void ejectDevice(const QString &deviceId);
    Q_INVOKABLE void disableDevice(qulonglong devInst);
    Q_INVOKABLE void initialScan();

    QVariantList devices() const;

signals:
    void devicesChanged();
    void logMessage(const QString &message);

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    void rescanDevices();

private:
    void addDevice(const QVariantMap &deviceDetails);

    QVariantList m_devices;
    HDEVNOTIFY m_notifyHandle;
    WId m_windowId;
    QTimer* m_rescanTimer;
};

#endif // USBMANAGER_H

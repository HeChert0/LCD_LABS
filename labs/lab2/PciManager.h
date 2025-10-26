#ifndef PCIMANAGER_H
#define PCIMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonArray>
#include <QJsonObject>
#include <memory>

struct PciDevice {
    int bus;
    int device;
    int function;
    QString vendorID;
    QString deviceID;
};

class PciManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool serverRunning READ isServerRunning NOTIFY serverRunningChanged)
    Q_PROPERTY(QString serverStatus READ serverStatus NOTIFY serverStatusChanged)
    Q_PROPERTY(QJsonArray devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QString clientIP READ clientIP NOTIFY clientIPChanged)
    Q_PROPERTY(int deviceCount READ deviceCount NOTIFY devicesChanged)

public:
    explicit PciManager(QObject *parent = nullptr);
    ~PciManager();

    bool isServerRunning() const;
    QString serverStatus() const { return m_serverStatus; }
    QJsonArray devices() const { return m_devices; }
    QString clientIP() const { return m_clientIP; }
    int deviceCount() const { return m_deviceList.size(); }

    Q_INVOKABLE void startServer();
    Q_INVOKABLE void stopServer();
    Q_INVOKABLE void clearDevices();
    Q_INVOKABLE QString getLocalIP() const;
    Q_INVOKABLE QString getVendorName(const QString &vendorID) const;

signals:
    void serverRunningChanged();
    void serverStatusChanged();
    void devicesChanged();
    void clientIPChanged();
    void logMessage(const QString &message);
    void errorOccurred(const QString &error);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    std::unique_ptr<QTcpServer> m_tcpServer;
    QTcpSocket* m_currentClient;
    QString m_serverStatus;
    QString m_clientIP;
    QByteArray m_buffer;
    QJsonArray m_devices;
    QList<PciDevice> m_deviceList;
    QHash<QString, QString> m_vendorDatabase;

    static constexpr int SERVER_PORT = 12345;

    void setServerStatus(const QString &status);
    void updateDevices(const QJsonArray &devices);
    void initVendorDatabase();
};

#endif // PCIMANAGER_H

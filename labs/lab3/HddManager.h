#ifndef HDDMANAGER_H
#define HDDMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>
#include <QVariantMap>
#include <QHostAddress>
#include <QNetworkInterface>

class HddManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool serverRunning READ serverRunning NOTIFY serverRunningChanged)
    Q_PROPERTY(QString serverStatus READ serverStatus NOTIFY serverStatusChanged)
    Q_PROPERTY(QString clientIP READ clientIP NOTIFY clientIPChanged)
    Q_PROPERTY(int driveCount READ driveCount NOTIFY driveCountChanged)
    Q_PROPERTY(QVariantList drives READ drives NOTIFY drivesChanged)

public:
    explicit HddManager(QObject *parent = nullptr);
    ~HddManager();

    bool serverRunning() const { return m_serverRunning; }
    QString serverStatus() const { return m_serverStatus; }
    QString clientIP() const { return m_clientIP; }
    int driveCount() const { return m_drives.size(); }
    QVariantList drives() const { return m_drives; }

    Q_INVOKABLE void startServer();
    Q_INVOKABLE void stopServer();
    Q_INVOKABLE void clearDrives();
    Q_INVOKABLE QString getLocalIP();
    Q_INVOKABLE QString formatBytes(qint64 bytes);
    Q_INVOKABLE QString getManufacturer(const QString& model);

signals:
    void serverRunningChanged();
    void serverStatusChanged();
    void clientIPChanged();
    void driveCountChanged();
    void drivesChanged();
    void logMessage(const QString& message);
    void errorOccurred(const QString& error);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onDataReceived();

private:
    void parseHddData(const QByteArray& data);

    QTcpServer* m_tcpServer;
    QTcpSocket* m_currentClient;
    bool m_serverRunning;
    QString m_serverStatus;
    QString m_clientIP;
    QVariantList m_drives;
    QByteArray m_buffer;
};

#endif // HDDMANAGER_H

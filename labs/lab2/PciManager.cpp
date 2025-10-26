#include "PciManager.h"
#include <QJsonDocument>
#include <QNetworkInterface>

PciManager::PciManager(QObject *parent)
    : QObject(parent)
    , m_currentClient(nullptr)
{
    initVendorDatabase();
    setServerStatus("Сервер остановлен");
}

PciManager::~PciManager()
{
    stopServer();
}

bool PciManager::isServerRunning() const
{
    return m_tcpServer && m_tcpServer->isListening();
}

void PciManager::startServer()
{
    if (m_tcpServer) {
        emit logMessage("Сервер уже запущен");
        return;
    }

    m_tcpServer = std::make_unique<QTcpServer>(this);

    connect(m_tcpServer.get(), &QTcpServer::newConnection,
            this, &PciManager::onNewConnection);

    if (m_tcpServer->listen(QHostAddress::Any, SERVER_PORT)) {
        setServerStatus(QString("Сервер запущен на порту %1").arg(SERVER_PORT));
        emit logMessage(QString("Сервер успешно запущен на порту %1").arg(SERVER_PORT));
        emit serverRunningChanged();
    } else {
        QString error = m_tcpServer->errorString();
        emit errorOccurred(QString("Ошибка запуска сервера: %1").arg(error));
        setServerStatus("Ошибка запуска");
        m_tcpServer.reset();
    }
}

void PciManager::stopServer()
{
    if (m_currentClient) {
        m_currentClient->disconnectFromHost();
        m_currentClient = nullptr;
    }

    if (m_tcpServer) {
        m_tcpServer->close();
        m_tcpServer.reset();
        setServerStatus("Сервер остановлен");
        emit logMessage("Сервер остановлен");
        emit serverRunningChanged();
    }
}

void PciManager::clearDevices()
{
    m_devices = QJsonArray();
    m_deviceList.clear();
    emit devicesChanged();
    emit logMessage("Список устройств очищен");
}

QString PciManager::getLocalIP() const
{
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol
            && address != QHostAddress::LocalHost
            && !address.toString().startsWith("169.254")) { // APIPA
            return address.toString();
        }
    }
    return "127.0.0.1";
}

QString PciManager::getVendorName(const QString &vendorID) const
{
    return m_vendorDatabase.value(vendorID.toUpper(), "Неизвестный производитель");
}

void PciManager::onNewConnection()
{
    if (m_currentClient) {
        m_currentClient->disconnectFromHost();
    }

    m_currentClient = m_tcpServer->nextPendingConnection();

    connect(m_currentClient, &QTcpSocket::readyRead,
            this, &PciManager::onReadyRead);
    connect(m_currentClient, &QTcpSocket::disconnected,
            this, &PciManager::onDisconnected);

    m_clientIP = m_currentClient->peerAddress().toString();
    emit clientIPChanged();

    setServerStatus(QString("Клиент подключен: %1").arg(m_clientIP));
    emit logMessage(QString("Подключение от %1").arg(m_clientIP));

    m_buffer.clear();
}

void PciManager::onReadyRead()
{
    if (!m_currentClient) return;

    m_buffer.append(m_currentClient->readAll());

    QJsonDocument doc = QJsonDocument::fromJson(m_buffer);
    if (!doc.isNull() && doc.isArray()) {
        updateDevices(doc.array());
        emit logMessage(QString("Получено %1 устройств").arg(doc.array().size()));
        m_buffer.clear();
    }
}

void PciManager::onDisconnected()
{
    if (m_currentClient) {
        emit logMessage(QString("Клиент %1 отключился").arg(m_clientIP));
        m_currentClient->deleteLater();
        m_currentClient = nullptr;
        m_clientIP.clear();
        emit clientIPChanged();
        setServerStatus("Ожидание подключения...");
    }
}

void PciManager::setServerStatus(const QString &status)
{
    if (m_serverStatus != status) {
        m_serverStatus = status;
        emit serverStatusChanged();
    }
}

void PciManager::updateDevices(const QJsonArray &devices)
{
    m_devices = devices;
    m_deviceList.clear();

    for (const auto &value : devices) {
        QJsonObject obj = value.toObject();
        PciDevice device;
        device.bus = obj["bus"].toInt();
        device.device = obj["device"].toInt();
        device.function = obj["function"].toInt();
        device.vendorID = obj["vendorID"].toString().toUpper();
        device.deviceID = obj["deviceID"].toString().toUpper();
        m_deviceList.append(device);
    }

    emit devicesChanged();
}

void PciManager::initVendorDatabase()
{
    m_vendorDatabase["8086"] = "Intel Corporation";
    m_vendorDatabase["1022"] = "AMD";
    m_vendorDatabase["10DE"] = "NVIDIA Corporation";
    m_vendorDatabase["1002"] = "AMD/ATI";
    m_vendorDatabase["14E4"] = "Broadcom";
    m_vendorDatabase["10EC"] = "Realtek Semiconductor";
    m_vendorDatabase["1B21"] = "ASMedia Technology";
    m_vendorDatabase["8087"] = "Intel Corporation";
    m_vendorDatabase["1033"] = "NEC Corporation";
    m_vendorDatabase["1106"] = "VIA Technologies";
    m_vendorDatabase["10B5"] = "PLX Technology";
    m_vendorDatabase["1039"] = "Silicon Integrated Systems";
    m_vendorDatabase["1000"] = "LSI Logic";
    m_vendorDatabase["15AD"] = "VMware";
    m_vendorDatabase["80EE"] = "VirtualBox";
    m_vendorDatabase["1D6B"] = "Linux Foundation";
    m_vendorDatabase["1AF4"] = "Red Hat (Virtio)";
}

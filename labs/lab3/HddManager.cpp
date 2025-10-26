#include "HddManager.h"
#include <QDebug>

HddManager::HddManager(QObject *parent)
    : QObject(parent)
    , m_tcpServer(new QTcpServer(this))
    , m_currentClient(nullptr)
    , m_serverRunning(false)
    , m_serverStatus("Не инициализирован")
{
    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &HddManager::onNewConnection);
}

HddManager::~HddManager()
{
    stopServer();
}

void HddManager::startServer()
{
    if (m_tcpServer->isListening()) {
        emit logMessage("Сервер уже запущен");
        return;
    }

    if (m_tcpServer->listen(QHostAddress::Any, 12346)) {
        m_serverRunning = true;
        m_serverStatus = "Сервер запущен";
        emit serverRunningChanged();
        emit serverStatusChanged();
        emit logMessage("Сервер запущен на порту 12346");
        emit logMessage("IP адрес: " + getLocalIP());
    } else {
        m_serverStatus = "Ошибка запуска";
        emit serverStatusChanged();
        emit errorOccurred("Не удалось запустить сервер: " + m_tcpServer->errorString());
    }
}

void HddManager::stopServer()
{
    if (m_currentClient) {
        m_currentClient->disconnectFromHost();
        m_currentClient = nullptr;
    }

    m_tcpServer->close();
    m_serverRunning = false;
    m_serverStatus = "Сервер остановлен";
    m_clientIP.clear();

    emit serverRunningChanged();
    emit serverStatusChanged();
    emit clientIPChanged();
    emit logMessage("Сервер остановлен");
}

void HddManager::clearDrives()
{
    m_drives.clear();
    emit drivesChanged();
    emit driveCountChanged();
    emit logMessage("Список дисков очищен");
}

QString HddManager::getLocalIP()
{
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            !address.isLoopback() &&
            address != QHostAddress::LocalHost) {
            return address.toString();
        }
    }
    return "127.0.0.1";
}

QString HddManager::formatBytes(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    const qint64 TB = GB * 1024;

    if (bytes >= TB) {
        double tb = static_cast<double>(bytes) / TB;
        return QString::number(tb, 'f', 2) + " TB";
    } else if (bytes >= GB) {
        double gb = static_cast<double>(bytes) / GB;
        return QString::number(gb, 'f', 2) + " GB";
    } else if (bytes >= MB) {
        double mb = static_cast<double>(bytes) / MB;
        return QString::number(mb, 'f', 2) + " MB";
    } else if (bytes >= KB) {
        double kb = static_cast<double>(bytes) / KB;
        return QString::number(kb, 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

QString HddManager::getManufacturer(const QString& model)
{
    QString modelUpper = model.toUpper();

    if (modelUpper.contains("WDC") || modelUpper.contains("WESTERN")) {
        return "Western Digital";
    } else if (modelUpper.contains("SEAGATE") || modelUpper.startsWith("ST")) {
        return "Seagate";
    } else if (modelUpper.contains("SAMSUNG")) {
        return "Samsung";
    } else if (modelUpper.contains("TOSHIBA")) {
        return "Toshiba";
    } else if (modelUpper.contains("HITACHI") || modelUpper.contains("HGST")) {
        return "Hitachi/HGST";
    } else if (modelUpper.contains("MAXTOR")) {
        return "Maxtor";
    } else if (modelUpper.contains("KINGSTON")) {
        return "Kingston";
    } else if (modelUpper.contains("SANDISK")) {
        return "SanDisk";
    } else if (modelUpper.contains("CRUCIAL")) {
        return "Crucial";
    } else if (modelUpper.contains("INTEL")) {
        return "Intel";
    } else if (modelUpper.contains("VMWARE")) {
        return "VMware Virtual";
    } else if (modelUpper.contains("VBOX")) {
        return "VirtualBox";
    }

    return "Unknown";
}

void HddManager::onNewConnection()
{
    if (m_currentClient) {
        m_currentClient->disconnectFromHost();
    }

    m_currentClient = m_tcpServer->nextPendingConnection();
    m_clientIP = m_currentClient->peerAddress().toString();

    emit clientIPChanged();
    emit logMessage("Подключен клиент: " + m_clientIP);

    connect(m_currentClient, &QTcpSocket::readyRead,
            this, &HddManager::onDataReceived);
    connect(m_currentClient, &QTcpSocket::disconnected,
            this, &HddManager::onClientDisconnected);

    m_buffer.clear();
}

void HddManager::onClientDisconnected()
{
    m_clientIP.clear();
    emit clientIPChanged();
    emit logMessage("Клиент отключился");

    if (m_currentClient) {
        m_currentClient->deleteLater();
        m_currentClient = nullptr;
    }
}

void HddManager::onDataReceived()
{
    if (!m_currentClient) return;

    QByteArray newData = m_currentClient->readAll();
    m_buffer.append(newData);

    // Проверка на завершение JSON
    if (m_buffer.contains(']')) {
        int endIndex = m_buffer.lastIndexOf(']');
        QByteArray jsonData = m_buffer.left(endIndex + 1);

        emit logMessage(QString("Получено %1 байт данных").arg(jsonData.size()));
        parseHddData(jsonData);

        m_buffer = m_buffer.mid(endIndex + 1);
    }
}

void HddManager::parseHddData(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        emit errorOccurred("Неверный формат данных");
        return;
    }

    m_drives.clear();
    QJsonArray array = doc.array();

    for (const QJsonValue& value : array) {
        if (!value.isObject()) continue;

        QJsonObject obj = value.toObject();
        QVariantMap drive;

        drive["index"] = obj["index"].toInt();
        drive["model"] = obj["model"].toString();
        drive["serial"] = obj["serial"].toString();
        drive["firmware"] = obj["firmware"].toString();
        drive["interface"] = obj["interface"].toString();

        qint64 totalBytes = obj["totalBytes"].toDouble();
        qint64 freeBytes = obj["freeBytes"].toDouble();
        qint64 usedBytes = obj["usedBytes"].toDouble();

        drive["totalBytes"] = totalBytes;
        drive["freeBytes"] = freeBytes;
        drive["usedBytes"] = usedBytes;

        drive["totalFormatted"] = formatBytes(totalBytes);
        drive["freeFormatted"] = formatBytes(freeBytes);
        drive["usedFormatted"] = formatBytes(usedBytes);

        // Процент использования
        if (totalBytes > 0) {
            double usedPercent = (static_cast<double>(usedBytes) / totalBytes) * 100;
            drive["usedPercent"] = QString::number(usedPercent, 'f', 1);
        } else {
            drive["usedPercent"] = "0.0";
        }

        drive["manufacturer"] = getManufacturer(obj["model"].toString());

        // Режимы
        QJsonArray modesArray = obj["modes"].toArray();
        QStringList modes;
        for (const QJsonValue& mode : modesArray) {
            modes.append(mode.toString());
        }
        drive["modes"] = modes.join(", ");

        m_drives.append(drive);
    }

    emit drivesChanged();
    emit driveCountChanged();
    emit logMessage(QString("Получена информация о %1 дисках").arg(m_drives.size()));
}

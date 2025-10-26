#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QQuickStyle>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QUrl>
#include "labs/lab1/PowerManager.h"
#include "labs/lab2/PciManager.h"
#include "labs/lab3/HddManager.h"
#include "labs/lab4/CameraManager.h"


static bool looksLikeProjectRoot(const QDir &dir) {
    QFileInfo f1(dir.filePath("qml/Main.qml"));
    QFileInfo f2(dir.filePath("resources/images/MM_left.png"));
    return f1.exists() || f2.exists();
}

static QString findProjectRootFrom(const QString &startPath, int maxLevels = 10) {
    QDir dir(startPath);
    for (int i = 0; i < maxLevels && dir.exists(); ++i) {
        if (looksLikeProjectRoot(dir)) {
            QFileInfo fi(dir.absolutePath());
            QString canon = fi.canonicalFilePath();
            if (!canon.isEmpty()) return canon;
            return fi.absoluteFilePath();
        }
        if (!dir.cdUp()) break;
    }
    return QString();
}

static QString detectProjectRoot() {
    QStringList starts;
    starts << QDir::currentPath();
    starts << QCoreApplication::applicationDirPath();
    starts << QDir::currentPath() + "/..";
    starts << QCoreApplication::applicationDirPath() + "/..";

    for (const QString &s : starts) {
        QString root = findProjectRootFrom(QDir(s).absolutePath(), 12);
        if (!root.isEmpty()) return root;
    }
    return QString();
}

static QString findMainQml(const QString &projectRoot) {
    if (!projectRoot.isEmpty()) {
        QDir root(projectRoot);
        QString candidate = root.filePath("qml/Main.qml");
        if (QFile::exists(candidate)) return QDir(candidate).canonicalPath();
        candidate = root.filePath("Main.qml");
        if (QFile::exists(candidate)) return QDir(candidate).canonicalPath();
    }

    QStringList candidates;
    candidates << QDir::current().filePath("qml/Main.qml");
    candidates << QDir(QCoreApplication::applicationDirPath()).filePath("qml/Main.qml");
    candidates << QDir(QCoreApplication::applicationDirPath()).filePath("../qml/Main.qml");
    candidates << QDir::current().filePath("Main.qml");

    for (const QString &p : candidates) {
        if (QFile::exists(p)) {
            QFileInfo fi(p);
            return fi.canonicalFilePath().isEmpty() ? fi.absoluteFilePath() : fi.canonicalFilePath();
        }
    }

    return QString();
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Fusion");
    QQmlApplicationEngine engine;

    qmlRegisterSingletonInstance<PowerManager>("com.company.PowerManager", 1, 0, "PowerManager", new PowerManager());
    qmlRegisterSingletonType<PciManager>("com.company.PciManager", 1, 0, "PciManager",
                                         [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
                                             Q_UNUSED(engine)
                                             Q_UNUSED(scriptEngine)
                                             return new PciManager();
                                         });
    qmlRegisterSingletonType<HddManager>("com.company.HddManager", 1, 0, "HddManager",
                                         [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
                                             Q_UNUSED(engine)
                                             Q_UNUSED(scriptEngine)
                                             return new HddManager();
                                         });
    qmlRegisterSingletonType<CameraManager>("com.company.CameraManager", 1, 0, "CameraManager",
                                            [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
                                                Q_UNUSED(engine)
                                                Q_UNUSED(scriptEngine)
                                                return new CameraManager();
                                            });


    QString projectRoot = detectProjectRoot();
    if (!projectRoot.isEmpty()) {
        qDebug() << "Detected project root (source dir):" << projectRoot;
    } else {
        qDebug() << "Could not detect project root; falling back to current/application dirs.";
        projectRoot = QDir::current().absolutePath();
    }

    if (!projectRoot.endsWith(QDir::separator()))
        projectRoot += QDir::separator();

    QString projectDirUrl = QUrl::fromLocalFile(projectRoot).toString(); //file:///C:
    engine.rootContext()->setContextProperty("projectDir", projectDirUrl);
    qDebug() << "projectDir set to:" << projectDirUrl;

    QString mainQmlPath = findMainQml(projectRoot);
    if (!mainQmlPath.isEmpty()) {
        qDebug() << "Loading QML from filesystem:" << mainQmlPath;
        engine.load(QUrl::fromLocalFile(mainQmlPath));
    } else {
        qDebug() << "Main.qml not found on filesystem. Trying qrc:/Main.qml as fallback.";
        const QUrl url(QStringLiteral("qrc:/Main.qml"));
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                         &app, [url](QObject *obj, const QUrl &objUrl) {
                             if (!obj && url == objUrl)
                                 QCoreApplication::exit(-1);
                         }, Qt::QueuedConnection);
        engine.load(url);
    }

    if (engine.rootObjects().isEmpty()) {
        qDebug() << "QQmlApplicationEngine has no root objects. Exiting.";
        return -1;
    }

    return app.exec();
}

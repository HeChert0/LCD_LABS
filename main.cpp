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

// содержит ли директория ожидаемую структуру проекта
static bool looksLikeProjectRoot(const QDir &dir) {
    QFileInfo f1(dir.filePath("qml/Main.qml"));
    QFileInfo f2(dir.filePath("resources/images/MM_left.png"));
    return f1.exists() || f2.exists();
}

// Пробуем подняться вверх по иерархии от start до корня диска и найти корень проекта
static QString findProjectRootFrom(const QString &startPath, int maxLevels = 10) {
    QDir dir(startPath);
    for (int i = 0; i < maxLevels && dir.exists(); ++i) {
        if (looksLikeProjectRoot(dir)) {
            // вернём абсолютную каноническую папку (без завершающего слэша)
            QFileInfo fi(dir.absolutePath());
            QString canon = fi.canonicalFilePath();
            if (!canon.isEmpty()) return canon;
            return fi.absoluteFilePath();
        }
        // подняться на уровень выше
        if (!dir.cdUp()) break;
    }
    return QString();
}

// Общая функция: ищем корень проекта, пробуем несколько стартовых точек
static QString detectProjectRoot() {
    // возможные места, откуда начать поиск:
    QStringList starts;
    starts << QDir::currentPath();                 // рабочая директория (например build при запуске из Qt Creator)
    starts << QCoreApplication::applicationDirPath(); // папка exe (build/.../Debug)
    // добавим родительские варианты (на всякий случай)
    starts << QDir::currentPath() + "/..";
    starts << QCoreApplication::applicationDirPath() + "/..";

    for (const QString &s : starts) {
        QString root = findProjectRootFrom(QDir(s).absolutePath(), 12);
        if (!root.isEmpty()) return root;
    }

    // как последний шанс — попробуем искать в фиксированных местах: предполагаемый исходный каталог рядом с исходниками
    // (полезно если проект располагается в C:/Qt/LCD_LABS)
    // Можно добавить здесь дополнительные известные пути по необходимости.

    return QString();
}

// Найдём файл Main.qml в файловой системе относительно найденного projectRoot
static QString findMainQml(const QString &projectRoot) {
    if (!projectRoot.isEmpty()) {
        QDir root(projectRoot);
        QString candidate = root.filePath("qml/Main.qml");
        if (QFile::exists(candidate)) return QDir(candidate).canonicalPath();
        candidate = root.filePath("Main.qml");
        if (QFile::exists(candidate)) return QDir(candidate).canonicalPath();
    }

    // fallback — те же варианты, которые мы пробовали ранее (по-старому)
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
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Fusion");
    QQmlApplicationEngine engine;

    //qmlRegisterSingletonInstance<PowerManager>("com.company.PowerManager", 1, 0, "PowerManager", new PowerManager());
    qmlRegisterType<PowerManager>("com.company.PowerManager", 1, 0, "PowerManager");

    // Попробуем обнаружить корень проекта (исходную папку, где лежат qml/ и resources/)
    QString projectRoot = detectProjectRoot();
    if (!projectRoot.isEmpty()) {
        qDebug() << "Detected project root (source dir):" << projectRoot;
    } else {
        qDebug() << "Could not detect project root; falling back to current/application dirs.";
        projectRoot = QDir::current().absolutePath(); // всё же зададим что-то, чтобы QML не крашился при обращении к projectDir
    }

    // Убедимся, что строка заканчивается на слэш (чтобы в QML можно было просто конкатенировать)
    if (!projectRoot.endsWith(QDir::separator()))
        projectRoot += QDir::separator();

    // Передаём как file:/// URL (QML ожидает URL-строку, у нас раньше был такой формат)
    QString projectDirUrl = QUrl::fromLocalFile(projectRoot).toString();
    engine.rootContext()->setContextProperty("projectDir", projectDirUrl);
    qDebug() << "projectDir set to:" << projectDirUrl;

    // Попытаемся найти Main.qml относительно найденного projectRoot (или в других местах)
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

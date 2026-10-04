#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QStyleFactory>

#include "library.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setOrganizationName(QStringLiteral("feedr"));
    QApplication::setApplicationName(QStringLiteral("feedr"));
    QApplication::setApplicationDisplayName(QStringLiteral("feedr"));
    QApplication::setApplicationVersion(QStringLiteral("0.2"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Offline ZIM reader"));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("zim"), QStringLiteral("ZIM files to open in tabs"), QStringLiteral("[files...]"));
    parser.process(app);

    Library library;
    MainWindow window(&library);
    window.show();

    const AppSettings settings = library.settings();
    if (settings.reopenTabs) {
        window.openPaths(library.lastSession());
    }
    for (const QString &path : parser.positionalArguments()) {
        window.openArchive(QFileInfo(path).absoluteFilePath(), true);
    }

    return app.exec();
}

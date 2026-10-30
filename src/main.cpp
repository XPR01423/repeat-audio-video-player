#include "ui/MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QIcon>
#include <QLocale>

#include <clocale>

int main(int argc, char *argv[])
{
    if (!std::setlocale(LC_ALL, ""))
        qFatal("RAV Player could not activate the user's system locale.");
    const QLocale userLocale = QLocale::system();
    if (!std::setlocale(LC_NUMERIC, "C"))
        qFatal("RAV Player could not set LC_NUMERIC to the required C locale before creating libmpv.");

    QApplication app(argc, argv);
    if (!std::setlocale(LC_NUMERIC, "C"))
        qFatal("RAV Player could not restore LC_NUMERIC=C after QApplication startup.");
    QLocale::setDefault(userLocale);
    QCoreApplication::setOrganizationName("Triasoft");
    QCoreApplication::setOrganizationDomain("triasoft.com");
    QCoreApplication::setApplicationName("RAV Player");
    QCoreApplication::setApplicationVersion("0.1.2");

    qInfo().noquote() << "RAV Player 0.1.2 starting; LC_NUMERIC="
                      << QString::fromLocal8Bit(std::setlocale(LC_NUMERIC, nullptr))
                      << "; Qt display locale=" << userLocale.name();

    QCommandLineParser parser;
    parser.setApplicationDescription("Repeat Audio Video Player");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "Media files to play", "[files…]");
    parser.process(app);

    MainWindow window;
    window.show();
    const auto files = parser.positionalArguments();
    if (!files.isEmpty())
        window.openFiles(files);
    return app.exec();
}

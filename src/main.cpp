#include "rosbag_editor.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("IcarusTechnology");
    app.setApplicationName("RosbagEditor");

    QCommandLineParser parser;
    parser.setApplicationDescription("RosbagEditor: because command line sucks");
    parser.addVersionOption();
    parser.addHelpOption();

    QCommandLineOption loadfile_option(QStringList() << "f" << "filename",
                                       "Load the rosbag",
                                       "file" );
    parser.addOption(loadfile_option);
    parser.process( *qApp );

    RosbagEditor main_win;
    main_win.show();
    return app.exec();
}

#include "verbosity.h"

#include <iostream>
#include <QString>
#include <QByteArray>

void VerbosityHandler(QtMsgType type, const QMessageLogContext & /*context*/, const QString & message) {
    auto stream = stdout;

    switch (type) {
    case QtDebugMsg:
        break;
    case QtInfoMsg:
        break;
    case QtWarningMsg:
        break;
    case QtCriticalMsg:
        stream = stderr;
        break;
    case QtFatalMsg:
        stream = stderr;
        break;
    }

    fprintf(stream, "%s", message.toLocal8Bit().constData());
    fflush(stream);
}

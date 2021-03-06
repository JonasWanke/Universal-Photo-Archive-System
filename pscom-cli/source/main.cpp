#include <pscom.h>

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QRegExp>

#include "commands.h"
#include "parsers.h"
#include "pscom_adapter.h"
#include "ui.h"
#include "verbosity.h"

int main(int argc, char *argv[]) {
    qInstallMessageHandler(VerbosityHandler);

    Command::app = new QCoreApplication(argc, argv);
    QCoreApplication::setApplicationVersion("1.0.0-beta.2");

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsOptions);

    const auto commands = QList<Command*>()
            << new HelpCommand()
            << new FeedbackCommand()
            << new VersionCommand()
            << new ImportCommand()
            << new ExportCommand()
            << new ListCommand()
            << new GroupCommand()
            << new UngroupCommand()
            << new ConvertCommand();
    parser.addPositionalArgument("command", QObject::tr("The command to execute."));
    parser.parse(Command::app->arguments());

    const auto args = parser.positionalArguments();
    if (args.isEmpty()) Formatter().exitWithMessage(QObject::tr("Please enter a command to execute. Run `pscom-cli help` to see a list of available commands."));

    const auto commandName = args.first();
    Command* command = nullptr;
    for (int i = 0; i < commands.size(); ++i) {
        if (commands.at(i)->name != args.first()) continue;
        command = commands.at(i);
    }
    if (command == nullptr) Formatter().exitWithMessage(QObject::tr("Invalid command. Run `pscom-cli help` to see a list of available commands."));

    command->configureCliParser(parser);
    parser.process(*Command::app);
    command->parse(parser, command->formatter);
    command->execute();
}

#include "commands.h"

#include <pscom.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QList>
#include <QSet>

#include "parsers.h"
#include "pscom_adapter.h"
#include "ui.h"


Command::Command(QString name, QString shortDescription, QString description, QList<QPair<QString, QString>> seeAlso, bool supportsUpa)
  : name(name),
    shortDescription(shortDescription),
    description(description),
    seeAlso(seeAlso),
    globalsParser(supportsUpa) {}

QCoreApplication* Command::app = nullptr;

void Command::addParser(QString groupName, Parser& parser) {
    MultiParser::addParser(parser);
}

void Command::configureCliParser(QCommandLineParser& parser) {
    globalsParser.configureCliParser(parser);

    for (auto arg: listArguments()) parser.addPositionalArgument(arg.first, arg.second);

    MultiParser::configureCliParser(parser);
}
QList<QCommandLineOption*> Command::listOptions() { return QList<QCommandLineOption*>(); }
QList<QPair<QString, QString>> Command::listArguments() { return QList<QPair<QString, QString>>(); }
void Command::parse(QCommandLineParser& parser, const Formatter&) {
    globalsParser.parse(parser, formatter);

    if (globalsParser.isHelpRequested()) {
        Command* command;
        HelpFormatter* helpFormatter;
        if (HelpCommand* c = dynamic_cast<HelpCommand*>(this)) {
            command = new HelpCommand();
            helpFormatter = new HelpFormatter(command->name, command->shortDescription, command->description, command->seeAlso);
        } else if (FeedbackCommand* c = dynamic_cast<FeedbackCommand*>(this)) {
            command = new FeedbackCommand();
            helpFormatter = new HelpFormatter(command->name, command->shortDescription, command->description, command->seeAlso);
        } else if (VersionCommand* c = dynamic_cast<VersionCommand*>(this)) {
            command = new VersionCommand();
            helpFormatter = new HelpFormatter(command->name, command->shortDescription, command->description, command->seeAlso);
        } else if (ImportCommand* c = dynamic_cast<ImportCommand*>(this)) {
            command = new ImportCommand();
            helpFormatter = new HelpFormatter(command->name, command->shortDescription, command->description, command->seeAlso);
            helpFormatter->addOptionGroup(QObject::tr("Filters"), (new FilterParser(false))->listOptions());
            helpFormatter->addOptionGroup(QObject::tr("Copy/Move"), (new CopyOrMoveParser())->listOptions());
        } else if (ExportCommand* c = dynamic_cast<ExportCommand*>(this)) {
            command = new ExportCommand();
            helpFormatter = new HelpFormatter(command->name, command->shortDescription, command->description, command->seeAlso);
            helpFormatter->addOptionGroup(QObject::tr("Filters"), (new FilterParser(true))->listOptions());
            helpFormatter->addOptionGroup(QObject::tr("Copy/Move"), (new CopyOrMoveParser())->listOptions());
        } else if (ListCommand* c = dynamic_cast<ListCommand*>(this)) {
            command = new ListCommand();
            helpFormatter = new HelpFormatter(command->name, command->shortDescription, command->description, command->seeAlso);
            helpFormatter->addOptionGroup(QObject::tr("Filters"), (new FilterParser(true))->listOptions());
        } else if (GroupCommand* c = dynamic_cast<GroupCommand*>(this)) {
            command = new GroupCommand();
            helpFormatter = new HelpFormatter(command->name, command->shortDescription, command->description, command->seeAlso);
            helpFormatter->addOptionGroup(QObject::tr("Filters"), (new FilterParser(true))->listOptions());
            helpFormatter->addOptionGroup(QObject::tr("Copy/Move"), (new CopyOrMoveParser())->listOptions());
        } else if (UngroupCommand* c = dynamic_cast<UngroupCommand*>(this)) {
            command = new UngroupCommand();
            helpFormatter = new HelpFormatter(command->name, command->shortDescription, command->description, command->seeAlso);
            helpFormatter->addOptionGroup(QObject::tr("Filters"), (new FilterParser(true))->listOptions());
            helpFormatter->addOptionGroup(QObject::tr("Copy/Move"), (new CopyOrMoveParser())->listOptions());
        } else if (ConvertCommand* c = dynamic_cast<ConvertCommand*>(this)) {
            command = new ConvertCommand();
            helpFormatter = new HelpFormatter(command->name, command->shortDescription, command->description, command->seeAlso);
            helpFormatter->addOptionGroup(QObject::tr("Filters"), (new FilterParser(true))->listOptions());
            helpFormatter->addOptionGroup(QObject::tr("Conversion"), (new ConversionParser())->listOptions());
            helpFormatter->addOptionGroup(QObject::tr("Yes to all"), (new YesToAllParser())->listOptions());
        }
        helpFormatter->addOptionGroup(QObject::tr("Global options"), (new GlobalsParser(globalsParser.supportsUpa))->listOptions());
        helpFormatter->print(formatter);
        exit(0);
    }

    MultiParser::parse(parser, formatter);
}

void Command::execute() {
    run();
}

// HelpCommand:
HelpCommand::HelpCommand()
  : Command(
        "help",
        QObject::tr("Print general help information."),
        QObject::tr("Running this prints a list of all available commands to the standard output.")
    ) {}

const QString HelpCommand::programTitle =
        "\n"
        "__/\\\\\\________/\\\\\\__/\\\\\\\\\\\\\\\\\\\\\\\\\\_______/\\\\\\\\\\\\\\\\\\____        \n"
        " _\\/\\\\\\_______\\/\\\\\\_\\/\\\\\\/////////\\\\\\___/\\\\\\\\\\\\\\\\\\\\\\\\\\__       \n"
        "  _\\/\\\\\\_______\\/\\\\\\_\\/\\\\\\_______\\/\\\\\\__/\\\\\\/////////\\\\\\_      \n"
        "   _\\/\\\\\\_______\\/\\\\\\_\\/\\\\\\\\\\\\\\\\\\\\\\\\\\/__\\/\\\\\\_______\\/\\\\\\_     \n"
        "    _\\/\\\\\\_______\\/\\\\\\_\\/\\\\\\/////////____\\/\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\_    \n"
        "     _\\/\\\\\\_______\\/\\\\\\_\\/\\\\\\_____________\\/\\\\\\/////////\\\\\\_   \n"
        "      _\\//\\\\\\______/\\\\\\__\\/\\\\\\_____________\\/\\\\\\_______\\/\\\\\\_  \n"
        "       __\\///\\\\\\\\\\\\\\\\\\/___\\/\\\\\\_____________\\/\\\\\\_______\\/\\\\\\_ \n"
        "        ____\\/////////_____\\///______________\\///________\\///__\n"
        "\n"
        "              The Universal Photo Archive System               ";

void HelpCommand::run() {
    formatter.info(formatter.programTitle(programTitle));
    formatter.info();

    formatter.info(formatter.header(QObject::tr("UPA Commands")));
    auto const upaCommands = QList<Command*>()
            << new ImportCommand()
            << new ExportCommand()
            << new ListCommand()
            << new GroupCommand()
            << new UngroupCommand()
            << new ConvertCommand();
    for (auto command: upaCommands) {
        formatter.info("• " + formatter.emphasize(command->name + ":") + " " + command->shortDescription);
    }
    formatter.info();

    formatter.info(formatter.header(QObject::tr("General Commands")));
    auto const generalCommands = QList<Command*>()
            << new HelpCommand()
            << new FeedbackCommand()
            << new VersionCommand();
    for (auto command: generalCommands) {
        formatter.info("• " + formatter.emphasize(command->name + ":") + " " + command->shortDescription);
    }
}

// VersionCommand:
VersionCommand::VersionCommand()
  : Command(
        "version",
        QObject::tr("Prints the program version."),
        QObject::tr("Prints detailed version information about the program itself as well as those of the underlying pscom & Qt libraries.")
    ) {}

void VersionCommand::run() {
    formatter.info(formatter.programTitle(HelpCommand::programTitle));
    formatter.info();

    formatter.info(PscomAdapter(true, formatter).versionInfo());
    formatter.info();
    formatter.info(QObject::tr("You're a beta tester. That's awesome! Let us know if you run into any problems:"));
    formatter.info("  beta-feedback.i-like-ps.com");
}

// FeedbackCommand:
FeedbackCommand::FeedbackCommand()
  : Command(
        "feedback",
        QObject::tr("Opens the UPA Beta feedback form."),
        QObject::tr("This shows the UPA Beta feedback form in your favorite browser.")
    ) {}

void FeedbackCommand::run() {
    auto const feedbackUrl = "beta-feedback.i-like-ps.com";
    formatter.info(QObject::tr("Opening %1 in your favorite browser…").arg(feedbackUrl));

    system("\"C:\\Program Files\\Internet Explorer\\iexplore.exe\" beta-feedback.i-like-ps.com");
}

// UpaCommand:
UpaCommand::UpaCommand(QString name, QString shortDescription, QString description, QList<QPair<QString, QString>> seeAlso)
  : Command(name, shortDescription, description, seeAlso, true) {}
void UpaCommand::parse(QCommandLineParser& parser, const Formatter&) {
    Command::parse(parser, formatter);

    pscom = new PscomAdapter(globalsParser.dryRun(), formatter);
    if (!pscom->doesDirectoryExist(globalsParser.getUpaRoot())) {
        formatter.exitWithMessage(QObject::tr("UPA root doesn't exist or is not a directory."));
    }
}

QString UpaCommand::getTargetDirectory(const QDate& date, const QPair<QString, QString>* locationAndEvent) const {
    auto target = globalsParser.getUpaRoot();
    target = pscom->getFilePathWithNewSubfolder(target, date, "yyyy");
    auto const monthDirectoryFormat = locationAndEvent == nullptr
            ? "yyyy-MM"
            : "yyyy-MM '" + locationAndEvent->first + " - " + locationAndEvent->second + "'";
    target = pscom->getFilePathWithNewSubfolder(target, date, monthDirectoryFormat);
    return target;
}
QString UpaCommand::getTargetFile(const QString photo, const QPair<QString, QString>* locationAndEvent) const {
    return getTargetFile(pscom->getFileSuffix(photo), pscom->getCreationTime(photo), locationAndEvent);
}
QString UpaCommand::getTargetFile(const QString suffix, const QDateTime& dateTime, const QPair<QString, QString>* locationAndEvent) const {
    auto target = getTargetDirectory(dateTime.date(), locationAndEvent);
    target = pscom->getFilePathWithNewFileName(target, dateTime, "yyyyMMdd_HHmmsszzz");
    target = pscom->getFilePathWithSuffix(target, suffix);
    return target;
}
QString UpaCommand::getTargetOutsideFile(const QString photo, const QString targetDirectory) const {
    auto target = targetDirectory;
    target = pscom->getFilePathWithNewFileName(target, pscom->getCreationTime(photo), "yyyyMMdd_HHmmsszzz");
    target = pscom->getFilePathWithSuffix(target, pscom->getFileSuffix(photo));
    return target;
}

void UpaCommand::checkIfTargetsExist(const QStringList& photos, const QPair<QString, QString>* locationAndEvent) const {
    QList<QPair<QString, QString>> photosAndTargets;
    for (auto photo: photos) {
        auto const target = getTargetFile(photo, locationAndEvent);
        photosAndTargets.append(QPair<QString, QString>(photo, target));
    }
    checkIfTargetsExist(photosAndTargets);
}
void UpaCommand::checkIfTargetsExist(const QList<QPair<QString, QString>>& photosAndTargets) const {
    QSet<QString> targets;
    for (auto photoAndTarget: photosAndTargets) {
        auto const photo = photoAndTarget.first;
        auto const target = photoAndTarget.second;

        if (pscom->doesExist(target)) {
            formatter.exitWithMessage(QObject::tr("Target location `%1` for photo `%2` already exists.").arg(target, photo));
        }
        if (targets.contains(target)) {
            formatter.exitWithMessage(QObject::tr("Target location `%1` for photo `%2` is the same as that of a different photo.").arg(target, photo));
        }
        targets.insert(target);
    }
}


// ImportCommand:
ImportCommand::ImportCommand()
  : UpaCommand(
        "import",
        QObject::tr("Imports photos into the UPA system."),
        QObject::tr("Given a list of directories and/or photo files, this imports all photos of supported formats in those locations, recursively walking inner directories. Whether imported photos are copied or moved into the UPA system can be controlled by the --copy and --move options.\n"
                    "\n"
                    "Supported formats are: %1.")
                .arg(PscomAdapter(true, Formatter()).supportedFormats().join(", ")),
        QList<QPair<QString, QString>>() << QPair<QString, QString>("export", QObject::tr("to export photos from the UPA system."))
    ),
    filterParser(FilterParser(false)) {
    addParser(QObject::tr("Filters"), filterParser);
    addParser(QObject::tr("Copy/Move"), copyOrMoveParser);
}

QList<QPair<QString, QString>> ImportCommand::listArguments() {
    return QList<QPair<QString, QString>>()
            << QPair<QString, QString>("photos…", QObject::tr("Locations (photos and/or directories) to import."));
}
void ImportCommand::parse(QCommandLineParser& parser, const Formatter&) {
    UpaCommand::parse(parser, formatter);
    importLocations = parser.positionalArguments();
    importLocations.removeFirst(); // Remove the command name.
    if (importLocations.isEmpty()) {
        formatter.exitWithMessage(QObject::tr("Please enter at least one photo or directory to import."));
    }
}

void ImportCommand::run() {
    auto photos = QStringList();
    for (auto location: importLocations) {
        if (pscom->doesFileExist(location)) {
            if (!pscom->isSupportedFile(location)) {
                formatter.exitWithMessage(QObject::tr("File `%1` has an invalid type. We support the following file endings: %2.")
                    .arg(location, pscom->supportedFormats().join(", ")));
            }

            if (filterParser.matchesRegexAndDateTime(*pscom, location)) photos.append(location);
        } else if (pscom->doesDirectoryExist(location)) {
            auto const files = pscom->listAll(location, true);
            for (auto file: files) {
                if (!pscom->isSupportedFile(file)) continue;
                if (filterParser.matchesRegexAndDateTime(*pscom, file)) photos.append(file);
            }
        } else { // pscom->doesNotExist(location)
            formatter.exitWithMessage(QObject::tr("The photo or directory `%1` doesn't exist.").arg(location));
        }
    }

    checkIfTargetsExist(photos);

    auto failures = QStringList();
    for (int i = 0; i < photos.length(); i++) {
        auto const photo = photos.at(i);
        formatter.progress(i, photos.length());
        auto const result = copyOrMoveParser.copyOrMove(*pscom, photo, getTargetFile(photo));
        if (!result) failures.append(photo);
    }

    if (!failures.isEmpty()) {
        formatter.error(QObject::tr("Failed to import %1/%2 photos:")
                .arg(failures.length())
                .arg(photos.length()));
        for (auto failure: failures) formatter.error("• " + failure);
    } else {
        formatter.info(QObject::tr("Successfully imported %1 photos 🎉", "", photos.length()).arg(photos.length()));
    }
}

// ExportCommand:
ExportCommand::ExportCommand()
  : UpaCommand(
        "export",
        QObject::tr("Exports photos from the UPA system."),
        QObject::tr("Exports all photos matching the given filters to the target directory. Whether exported photos are copied or moved out of the UPA system can be controlled by the --copy and --move options."),
        QList<QPair<QString, QString>>() << QPair<QString, QString>("import", QObject::tr("to import photos into the UPA system."))
    ),
    filterParser(FilterParser(true)) {
    addParser(QObject::tr("Filters"), filterParser);
    addParser(QObject::tr("Copy/Move"), copyOrMoveParser);
}

QList<QPair<QString, QString>> ExportCommand::listArguments() {
    return QList<QPair<QString, QString>>()
            << QPair<QString, QString>("target", QObject::tr("Directory to export to, including the trailing slash."));
}
void ExportCommand::parse(QCommandLineParser& parser, const Formatter&) {
    UpaCommand::parse(parser, formatter);
    auto args = parser.positionalArguments();
    args.removeFirst();
    if (args.isEmpty()) {
        formatter.exitWithMessage(QObject::tr("Please enter a target directory."));
    } else if (args.length() > 1) {
        formatter.exitWithMessage(QObject::tr("Please enter only one target directory."));
    }
    exportLocation = args.first();
    auto exportLocationPattern = QRegExp(".*[/\\\\]");
    if (!exportLocationPattern.exactMatch(exportLocation)) {
        formatter.exitWithMessage(QObject::tr("Target directory must end with a slash, but was: `%1`.").arg(exportLocation));
    }
}

void ExportCommand::run() {
    auto photos = filterParser.listMatches(*pscom, globalsParser.getUpaRoot());

    QList<QPair<QString, QString>> photosAndTargets;
    for (auto photo: photos) {
        auto const target = getTargetOutsideFile(photo, exportLocation);
        photosAndTargets.append(QPair<QString, QString>(photo, target));
    }
    checkIfTargetsExist(photosAndTargets);

    for (int i = 0; i < photosAndTargets.length(); i++) {
        auto const photoAndTarget = photosAndTargets.at(i);
        auto const photo = photoAndTarget.first;
        auto const target = photoAndTarget.second;
        formatter.progress(i, photos.length());
        copyOrMoveParser.copyOrMove(*pscom, photo, target);
    }
    formatter.info(QObject::tr("Successfully exported %1 photo(s) to `%2` 🎉", "", photos.length())
            .arg(photos.length())
            .arg(exportLocation));
}


// ListCommand:
ListCommand::ListCommand()
  : UpaCommand(
        "list",
        QObject::tr("List photos matching the filter criteria."),
        QObject::tr("Outputs a list of all photos in the UPA system matching the filter criteria you specified.")
    ) {
    addParser(QObject::tr("Filters"), filterParser);
}

void ListCommand::run() {
    auto const matches = filterParser.listMatches(*pscom, globalsParser.getUpaRoot());
    if (matches.isEmpty()) {
        formatter.info(QObject::tr("No photos found."));
        return;
    }

    formatter.info(QObject::tr("%1 photo(s):", "", matches.length()).arg(matches.length()));
    for (auto photo: matches) formatter.info("• " + photo);
}


// GroupCommand:
GroupCommand::GroupCommand()
  : UpaCommand(
        "group",
        QObject::tr("Groups photos by location and event."),
        QObject::tr("Photos that match the given filter criteria will be put into special folders, based on the given location and event. (Both are required.) Whether photos are copied or moved into the new folders can be controlled by the --copy and --move options."),
        QList<QPair<QString, QString>>() << QPair<QString, QString>("ungroup", QObject::tr("to remove photos from a grouping."))
    ),
    filterParser(FilterParser(true)) {
    addParser(QObject::tr("Filters"), filterParser);
    addParser(QObject::tr("Copy/Move"), copyOrMoveParser);
}

QList<QPair<QString, QString>> GroupCommand::listArguments() {
    return QList<QPair<QString, QString>>()
            << QPair<QString, QString>("location", QObject::tr("Location of the event."))
            << QPair<QString, QString>("event", QObject::tr("Name of the event."));
}
void GroupCommand::parse(QCommandLineParser& parser, const Formatter&) {
    UpaCommand::parse(parser, formatter);

    auto args = parser.positionalArguments();
    args.removeFirst(); // Remove the command name.
    if (args.length() < 2) {
        formatter.exitWithMessage(QObject::tr("Please enter a location and an event to group these photos under."));
    } else if (args.length() > 2) {
        formatter.exitWithMessage(QObject::tr("Too many arguments. Please only enter a location and an event. Use quotes if any of these contain spaces."));
    }

    location = args.at(0).trimmed();
    if (location.isEmpty()) formatter.exitWithMessage(QObject::tr("Please enter a non-empty location name."));
    event = args.at(1).trimmed();
    if (event.isEmpty()) formatter.exitWithMessage(QObject::tr("Please enter a non-empty event name."));
}

void GroupCommand::run() {
    auto photos = filterParser.listMatches(*pscom, globalsParser.getUpaRoot());
    auto const locationAndEvent = QPair<QString, QString>(location, event);
    checkIfTargetsExist(photos, &locationAndEvent);

    for (int i = 0; i < photos.length(); i++) {
        auto const photo = photos.at(i);
        formatter.progress(i, photos.length());
        auto const target = getTargetFile(photo, &locationAndEvent);
        copyOrMoveParser.copyOrMove(*pscom, photo, target);
    }
    formatter.info(QObject::tr("Successfully grouped %1 photo(s) to `%2 - %3` 🎉", "", photos.length())
            .arg(photos.length())
            .arg(location)
            .arg(event));
}

// UngroupCommand:
UngroupCommand::UngroupCommand()
  : UpaCommand(
        "ungroup",
        QObject::tr("Ungroups photos that are grouped by location and event."),
        QObject::tr("Photos that match the given filter criteria will be transferred out of location/event-based folders. Whether photos are copied or moved into the new folders can be controlled by the --copy and --move options."),
        QList<QPair<QString, QString>>() << QPair<QString, QString>("group", QObject::tr("to group photos by location and event."))
    ),
    filterParser(FilterParser(true)) {
    addParser(QObject::tr("Filters"), filterParser);
    addParser(QObject::tr("Copy/Move"), copyOrMoveParser);
}

void UngroupCommand::run() {
    auto photos = filterParser.listMatches(*pscom, globalsParser.getUpaRoot());

    QStringList groupedPhotos;
    for (auto photo: photos) {
        auto const normalizedPhoto = QString(photo).replace("\\", "/");
        auto const normalizedTarget = getTargetFile(photo).replace("\\", "/");
        if (normalizedPhoto == normalizedTarget) {
            // The photo isn't in a group.
            continue;
        }

        groupedPhotos.append(photo);
    }

    checkIfTargetsExist(groupedPhotos);

    for (int i = 0; i < groupedPhotos.length(); i++) {
        auto const photo = groupedPhotos.at(i);
        formatter.progress(i, groupedPhotos.length());
        auto const target = getTargetFile(photo);
        copyOrMoveParser.copyOrMove(*pscom, photo, target);
    }
    formatter.info(QObject::tr("Successfully ungrouped %1 photo(s) 🎉", "", groupedPhotos.length()).arg(groupedPhotos.length()));
}


// ConvertCommand:
ConvertCommand::ConvertCommand()
  : UpaCommand(
        "convert",
        QObject::tr("Converts photos to different sizes, qualities, or formats."),
        QObject::tr("Photos matching the given filter criteria are converted based on the conversion options. The originals are overriden, or, if the resulting file has a different format, the originals are just deleted. (You'll have to confirm this step interactively, or specify the --yes option.)")
    ),
    filterParser(FilterParser(true)) {
    addParser(QObject::tr("Input Filters"), filterParser);
    addParser(QObject::tr("Conversion"), conversionParser);
    addParser(QObject::tr("Yes to all"), yesToAllParser);
}

void ConvertCommand::run() {
    auto photos = filterParser.listMatches(*pscom, globalsParser.getUpaRoot());

    for (int i = 0; i < photos.length(); i++) {
        auto const photo = photos.at(i);
        formatter.progress(i, photos.length());

        auto const remove = yesToAllParser.promptYesNoAll(
            formatter,
            QObject::tr("Conversion will remove original photo `%1`. Convert this photo?").arg(photo)
        );
        if (!remove) continue;

        auto const outputName = conversionParser.getFormat().isEmpty()
                ? photo
                : pscom->getFilePathWithSuffix(photo, conversionParser.getFormat());
        auto const changeFormat = photo != outputName;

        if (conversionParser.getWidth() > 0) {
            if (conversionParser.getHeight() > 0) {
                pscom->scaleToSize(photo, conversionParser.getWidth(), conversionParser.getHeight());
            } else {
                pscom->scaleToWidth(photo, conversionParser.getWidth());
            }
        } else {
            if (conversionParser.getHeight() > 0) {
                pscom->scaleToHeight(photo, conversionParser.getHeight());
            }
        }

        if (changeFormat || conversionParser.getQuality() >= 0) {
            auto const suffix = conversionParser.getFormat().isEmpty()
                    ? pscom->getFileSuffix(photo)
                    : conversionParser.getFormat();
            pscom->changeFormat(photo, suffix, conversionParser.getQuality());
        }
    }

    formatter.info(QObject::tr("Successfully converted %1 photo(s) 🎉", "", photos.length()).arg(photos.length()));
}

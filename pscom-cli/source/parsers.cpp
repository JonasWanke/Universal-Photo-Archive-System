#include "parsers.h"

#include <pscom.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTextStream>
#include <QTranslator>

#include "commands.h"
#include "pscom_adapter.h"
#include "ui.h"

void Parser::configureCliParser(QCommandLineParser& parser) {
    for (auto option: listOptions()) {
        parser.addOption(*option);
    }
}

// MultiParser:
void MultiParser::configureCliParser(QCommandLineParser& parser) {
    for (auto p: parsers) {
        p->configureCliParser(parser);
    }
}
void MultiParser::parse(QCommandLineParser& parser, const Formatter& formatter) {
    for (auto p: parsers) {
        p->parse(parser, formatter);
    }
}

void MultiParser::addParser(Parser& parser) {
    parsers.append(&parser);
}


// GlobalsParser:
GlobalsParser::GlobalsParser(bool supportsUpa)
  : helpOption(
        "help",
        QObject::tr("Display this help and exit.")),
    verbosityOption(
        QStringList() << "v" << "verbosity",
        QObject::tr("Set the minimum verbosity to output.\n"
                "Supported values are: debug, info (the default), warning, error, none."),
        QObject::tr("verbosity")),
    localeOption(
        "locale",
        QObject::tr("The locale to use for all outputs.\n"
                "Supported values are: de (German), en (English; the default)."),
        QObject::tr("locale")),
    supportsUpa(supportsUpa),
    upaRootOption(
        "upa-root",
        QObject::tr("Path to the UPA root directory, including a trailing slash. Defaults to the current directory."),
        QObject::tr("path")),
    dryRunOption(
        "dry-run",
        QObject::tr("Simulate the execution of this command without actually modifying photos or directories.")) {}

QList<QCommandLineOption*> GlobalsParser::listOptions() {
    QList<QCommandLineOption*> result;
    result << &helpOption << &verbosityOption << &localeOption;
    if (supportsUpa) result << &upaRootOption << &dryRunOption;
    return result;
}
void GlobalsParser::parse(QCommandLineParser& parser, const Formatter& formatter) {
    help = parser.isSet(helpOption);

    auto const rawVerbosity = parser.value(verbosityOption);
    auto isVerbosityValid = true;
    if (rawVerbosity.compare("debug", Qt::CaseInsensitive) == 0) {
        verbosity = Verbosity::debug;
    } else if (rawVerbosity.compare("info", Qt::CaseInsensitive) == 0 || rawVerbosity.isEmpty()) {
        verbosity = Verbosity::info;
    } else if (rawVerbosity.compare("warning", Qt::CaseInsensitive) == 0) {
        verbosity = Verbosity::warning;
    } else if (rawVerbosity.compare("error", Qt::CaseInsensitive) == 0) {
        verbosity = Verbosity::error;
    } else if (rawVerbosity.compare("none", Qt::CaseInsensitive) == 0) {
        verbosity = Verbosity::none;
    } else {
        isVerbosityValid = false;
    }
    Formatter::verbosity = verbosity;

    auto translator = new QTranslator();
    auto const rawLocale = parser.value(localeOption);
    if (rawLocale.compare("de", Qt::CaseInsensitive) == 0) {
        translator->load("../main_de");
    } else if (rawLocale.compare("en", Qt::CaseInsensitive) == 0 || rawLocale.isEmpty()) {
        translator->load("../main_en");
    } else {
        formatter.exitWithMessage(QObject::tr("Invalid locale: `%1`. Supported values are: de (German), en (English; the default).").arg(rawLocale));
    }
    Command::app->installTranslator(translator);

    if (!isVerbosityValid) {
        formatter.exitWithMessage(QObject::tr("Invalid verbosity: `%1`. Supported values are: debug, info (the default), warning, error, none.").arg(rawVerbosity));
    }

    if (!supportsUpa) return;
    const auto upaRootOverride = parser.value(upaRootOption);
    if (!upaRootOverride.isEmpty()) {
        upaRoot = upaRootOverride;

        auto upaRootPattern = QRegExp(".*[/\\\\]");
        if (!upaRootPattern.exactMatch(upaRoot)) {
            formatter.exitWithMessage(QObject::tr("UPA root directory must end with a slash, but was:`%1`.").arg(upaRoot));
        }
    } else {
        upaRoot = ".";
    }

    _dryRun = parser.isSet(dryRunOption);
}

bool GlobalsParser::isHelpRequested() const { return help; }
Verbosity GlobalsParser::getVerbosity() const { return verbosity; }
QString GlobalsParser::getUpaRoot() const { return upaRoot; }
bool GlobalsParser::dryRun() const { return _dryRun; }


// FilterParser:
FilterParser::FilterParser(bool supportsEventLocationOptions)
  : supportsEventLocationOptions(supportsEventLocationOptions),
    locationOption(
        QStringList() << "l" << "location",
        QObject::tr("Filter input files by their location.\n"
                    "If this option is specified multiple times, photos with any of these locations are matched."),
        QObject::tr("location")),
    eventOption(
        QStringList() << "e" << "event",
        QObject::tr("Filter input files by their event.\n"
                    "If this option is specified multiple times, photos with any of these events are matched."),
        QObject::tr("event")),
    onlyInEventOption(
        QStringList() << "in-event",
        QObject::tr("Filter input files by having an event, i.e., being in an event/location folder.")),
    onlyOutsideEventOption(
        QStringList() << "no-in-event",
        QObject::tr("Filter input files by not having an event, i.e., being in a plain date-time-based folder.")),
    regexOption(
        QStringList() << "r" << "regex",
        QObject::tr("Filter input files with a regular expression. See doc.qt.io/archives/qt-4.8/qregexp.html#introduction for supported features.\n"
                    "If this option is specified multiple times, photos matching any of these regular expressions are matched."),
        QObject::tr("regex")),
    dateTimeOption(
        QStringList() << "d" << "date-time",
        QObject::tr("Filter input files by their date and time.\n"
                    "If this option is specified multiple times, photos matching any of these date-times are matched.\n"
                    "\n"
                    "Supported patterns (based on ISO 8601) are:\n"
                    "• <date>: Photos from that date (or range, e.g., a single year).\n"
                    "• <date>T<time>: Photos from that date (not a range) and time.\n"
                    "• <start>/<end>: Photos from the given range, whereas start and end may be any of the above.\n"
                    "\n"
                    "Supported date patterns are:\n"
                    "• yyyy: All photos within that year.\n"
                    "• yyyy-MM: All photos within that year and month.\n"
                    "• yyyy-MM-dd, yyyyMMdd: All photos taken on that day.\n"
                    "• yyyy-ddd, yyyyddd: All photos taken on the nth day of that year.\n"
                    "\n"
                    "Supported time patterns are:\n"
                    "• hh: All photos from that hour.\n"
                    "• hh:mm, hhmm: All photos from that hour and minute.\n"
                    "• hh:mm:ss, hhmmss: All photos from that hour, minute, and second.\n"
                    "• hh:mm:ss.z, hhmmss.z: All photos from that hour, minute, second, and millisecond. Either a dot (.) or a comma (,) may be used."),
        QObject::tr("date/date-time/period")) {}

QList<QCommandLineOption*> FilterParser::listOptions() {
    QList<QCommandLineOption*> options;
    if (supportsEventLocationOptions) {
        options << &locationOption << &eventOption << &onlyInEventOption << &onlyOutsideEventOption;
    }

    options << &regexOption << &dateTimeOption;
    return options;
}
void FilterParser::parse(QCommandLineParser& parser, const Formatter& formatter) {
    if (supportsEventLocationOptions) {
        locations = parser.values(locationOption);
        events = parser.values(eventOption);
        if (parser.isSet(onlyInEventOption) && parser.isSet(onlyOutsideEventOption)) {
            formatter.exitWithMessage(QObject::tr("At most one of --in-event and --no-in-event may be specified."));
        }
        onlyInEvent = parser.isSet(onlyInEventOption);
        onlyOutsideEvent = parser.isSet(onlyOutsideEventOption);
    }

    auto regexesRaw = parser.values(regexOption);
    for (auto regex: regexesRaw) {
        regexes.append(QRegExp(regex));
    }

    auto dateTimesRaw = parser.values(dateTimeOption);
    for (auto dateTime: dateTimesRaw) {
        auto dateTimePattern = QRegExp("([^T/]*)(T[^T/]*)?(/([^T/]*)(T.*)?)?");
        if (!dateTimePattern.exactMatch(dateTime)) {
            formatter.exitWithMessage(QObject::tr("Unsupported date-time: `%1`.").arg(dateTime));
        }

        auto const startDatePart = dateTimePattern.cap(1);
        auto const startTimePart = dateTimePattern.cap(2);
        auto const hasEndDateTime = !dateTimePattern.cap(3).isEmpty();
        auto const endDatePart = dateTimePattern.cap(4);
        auto const endTimePart = dateTimePattern.cap(5);

        auto const startDateRange = parseDate(formatter, startDatePart);
        if (startDateRange.first != startDateRange.second && !startTimePart.isEmpty()) {
            formatter.exitWithMessage(QObject::tr("Invalid date-time: `%1`. When specifying a time (range), the date must not be a range.").arg(dateTime));
        }

        auto const start = startTimePart.isEmpty()
                ? startDateRange.first.startOfDay()
                : QDateTime(startDateRange.first, parseTime(formatter, startTimePart).first);
        if (!hasEndDateTime) {
            auto const end = startTimePart.isEmpty()
                    ? startDateRange.second.endOfDay()
                    : QDateTime(startDateRange.second, parseTime(formatter, startTimePart).second);
            dateTimes.append(QPair<QDateTime, QDateTime>(start, end));
        } else {
            auto const endDateRange = parseDate(formatter, endDatePart);
            auto const end = endTimePart.isEmpty()
                    ? endDateRange.second.endOfDay()
                    : QDateTime(endDateRange.second, parseTime(formatter, endTimePart).second);
            dateTimes.append(QPair<QDateTime, QDateTime>(start, end));
        }
    }
}
QPair<QDate, QDate> FilterParser::parseDate(const Formatter& formatter, QString date) {
    // YYYY
    auto const yearResult = QDate::fromString(date, "yyyy");
    if (yearResult.isValid()) {
        return QPair<QDate, QDate>(
            yearResult,
            yearResult.addYears(1).addDays(-1)
        );
    }

    // YYYY-MM
    auto const monthResult = QDate::fromString(date, "yyyy-MM");
    if (monthResult.isValid()) {
        return QPair<QDate, QDate>(
            monthResult,
            monthResult.addMonths(1).addDays(-1)
        );
    }

    // YYYY-MM-DD
    auto const day1Result = QDate::fromString(date, "yyyy-MM-dd");
    if (day1Result.isValid()) {
        return QPair<QDate, QDate>(day1Result, day1Result);
    }
    // YYYYMMDD
    auto const day2Result = QDate::fromString(date, "yyyyMMdd");
    if (day2Result.isValid()) {
        return QPair<QDate, QDate>(day2Result, day2Result);
    }

    // TODO: weeks
    // YYYY-Www or YYYYWww
    // YYYY-Www-D or YYYYWwwD
    /*auto const weekResult = QDate::fromString(dateTime, "yyyy-MM");
    if (monthResult.isValid()) {
        dateTimes.append(QPair<QDateTime, QDateTime>(
            monthResult.startOfDay(),
            monthResult.addMonths(1).addDays(-1).endOfDay()
        ));
        continue;
    }*/

    // YYYY-DDD or YYYYDDD
    auto const yearDayPattern = QRegExp("(\\d{4})-?(\\d{3})");
    if (yearDayPattern.exactMatch(date)) {
        auto const year = yearDayPattern.cap(1).toInt();
        auto const day = yearDayPattern.cap(2).toInt();
        auto const date = QDate(year, 1, 1).addDays(day - 1);
        return QPair<QDate, QDate>(date, date);
    }

    formatter.exitWithMessage(QObject::tr("Invalid date: `%1`.").arg(date));
}
QPair<QTime, QTime> FilterParser::parseTime(const Formatter& formatter, QString time) {
    // Thh
    auto const hourResult = QTime::fromString(time, "Thh");
    if (hourResult.isValid()) {
        return QPair<QTime, QTime>(hourResult, hourResult.addSecs(3600).addMSecs(-1));
    }

    // Thh:mm
    auto const minute1Result = QTime::fromString(time, "Thh:mm");
    if (minute1Result.isValid()) {
        return QPair<QTime, QTime>(minute1Result, minute1Result.addSecs(60).addMSecs(-1));
    }
    // Thhmm
    auto const minute2Result = QTime::fromString(time, "Thhmm");
    if (minute2Result.isValid()) {
        return QPair<QTime, QTime>(minute2Result, minute2Result.addSecs(60).addMSecs(-1));
    }

    // Thh:mm:ss
    auto const second1Result = QTime::fromString(time, "Thh:mm:ss");
    if (second1Result.isValid()) {
        return QPair<QTime, QTime>(second1Result, second1Result.addSecs(1).addMSecs(-1));
    }
    // Thhmmss
    auto const second2Result = QTime::fromString(time, "Thhmmss");
    if (second2Result.isValid()) {
        return QPair<QTime, QTime>(second2Result, second2Result.addSecs(1).addMSecs(-1));
    }

    auto const timeForMillis = time.replace(",", ".");
    // Thh:mm:ss.sss
    auto const millis1Result = QTime::fromString(timeForMillis, "Thh:mm:ss.z");
    if (millis1Result.isValid()) {
        return QPair<QTime, QTime>(millis1Result, millis1Result);
    }
    // Thhmmss.sss
    auto const millis2Result = QTime::fromString(timeForMillis, "Thhmmss.z");
    if (millis2Result.isValid()) {
        return QPair<QTime, QTime>(millis2Result, millis2Result);
    }

    formatter.exitWithMessage(QObject::tr("Invalid time: `%1`.").arg(time));
}

QStringList FilterParser::getLocations() { return locations; }
QStringList FilterParser::getEvents() { return events; }
bool FilterParser::getOnlyInEvent() { return onlyInEvent; }
bool FilterParser::getOnlyOutsideEvent() { return onlyOutsideEvent; }

QList<QRegExp> FilterParser::getRegexes() { return regexes; }

QList<QPair<QDateTime, QDateTime>> FilterParser::getDateTimes() { return dateTimes; }

QStringList FilterParser::listMatches(const PscomAdapter& pscom, const QString& upaRoot) {
    assert(supportsEventLocationOptions);

    auto const allPhotos = pscom.listAll(upaRoot, true);

    QStringList results;
    for (auto photo: allPhotos) {
        if (onlyInEvent) {
            auto const regex = QRegExp(".*/\\d{4}-\\d{2} .+ - .+/[^/]+");
            if (!regex.exactMatch(photo)) continue;
        }
        if (onlyOutsideEvent) {
            auto const regex = QRegExp(".*/\\d{4}-\\d{2}/[^/]+");
            if (!regex.exactMatch(photo)) continue;
        }

        if (!locations.isEmpty()) {
            bool anyMatches = false;
            for (auto location: locations) {
                auto const regex = QRegExp(".*/\\d{4}-\\d{2} " + QRegExp::escape(location) + " - .+?/[^/]+");
                if (regex.exactMatch(photo)) continue;

                anyMatches = true;
                break;
            }
            if (!anyMatches) continue;
        }

        if (!events.isEmpty()) {
            bool anyMatches = false;
            for (auto event: events) {
                auto const regex = QRegExp(".*/\\d{4}-\\d{2} .+? - " + QRegExp::escape(event) + "/[^/]+");
                if (regex.exactMatch(photo)) continue;

                anyMatches = true;
                break;
            }
            if (!anyMatches) continue;
        }

        if (!matchesRegexAndDateTime(pscom, photo)) continue;
        results.append(photo);
    }

    return results;
}
bool FilterParser::matchesRegexAndDateTime(const PscomAdapter& pscom, const QString& path) {
    if (!regexes.isEmpty()) {
        bool anyMatches = false;
        for (auto regex: regexes) {
            if (regex.exactMatch(path)) continue;

            anyMatches = true;
            break;
        }
        if (!anyMatches) return false;
    }

    if (!dateTimes.isEmpty()) {
        auto const photoDateTime = pscom.getCreationTime(path);
        bool anyMatches = false;
        for (auto dateTime: dateTimes) {
            if (photoDateTime < dateTime.first || dateTime.second < photoDateTime) continue;

            anyMatches = true;
            break;
        }
        if (!anyMatches) return false;
    }

    return true;
}


// CopyOrMoveParser:
CopyOrMoveParser::CopyOrMoveParser()
  : copyOption(
        "copy",
        QObject::tr("Copy photos.")),
    moveOption(
        "move",
        QObject::tr("Move photos.")) {}

QList<QCommandLineOption*> CopyOrMoveParser::listOptions() {
    return QList<QCommandLineOption*>() << &copyOption << &moveOption;
}
void CopyOrMoveParser::parse(QCommandLineParser& parser, const Formatter& formatter) {
    if (parser.isSet(copyOption) && parser.isSet(moveOption)) {
        formatter.exitWithMessage(QObject::tr("At most one of --copy and --move may be specified."));
    } else if (parser.isSet(copyOption)) {
        _shouldMove = false;
    } else if (parser.isSet(moveOption)) {
        _shouldMove = true;
    } else {
        _shouldMove = promptBinary(
            formatter,
            QObject::tr("Do you want to copy or move these files?"),
            QObject::tr("Copy"),
            QObject::tr("Move")
        );
    }
}

bool CopyOrMoveParser::shouldMove() { return _shouldMove; }
bool CopyOrMoveParser::copyOrMove(PscomAdapter& pscom, const QString& source, const QString& destination) {
    if (shouldMove()) {
        return pscom.move(source, destination);
    } else {
        return pscom.copy(source, destination);
    }
}


// ConversionParser:
ConversionParser::ConversionParser()
  : formatOption(
        QStringList() << "f" << "format",
        QObject::tr("Image format to generate. Supported formats are: %1.")
            .arg(PscomAdapter(true, Formatter()).supportedFormats().join(", ")),
        QObject::tr("format")),
    qualityOption(
        QStringList() << "q" << "quality",
        QObject::tr("The quality of the final image (0 – 100 or -1). Specify 0 to obtain small compressed files, 100 for large uncompressed files, and -1 (the default) to use the default settings."),
        QObject::tr("quality")),
    widthOption(
        QStringList() << "w" << "width",
        QObject::tr("New width in pixels. The height will be scaled accordingly."),
        QObject::tr("width")),
    heightOption(
        QStringList() << "h" << "height",
        QObject::tr("New height in pixels. The width will be scaled accordingly."),
        QObject::tr("height")) {}

QList<QCommandLineOption*> ConversionParser::listOptions() {
    return QList<QCommandLineOption*>() << &formatOption << &qualityOption << &widthOption << &heightOption;
}
void ConversionParser::parse(QCommandLineParser& parser, const Formatter& formatter) {
    format = parser.value(formatOption);
    auto const supportedFormats = PscomAdapter(true, Formatter()).supportedFormats();
    if (!format.isEmpty() && !supportedFormats.contains(format)) {
        formatter.exitWithMessage(QObject::tr("Unsupported format: `%1`. Supported formats are: %2.").arg(format, supportedFormats.join(", ")));
    }

    bool okay;
    auto const rawQuality = parser.value(qualityOption);
    if (!rawQuality.isEmpty()) {
        quality = rawQuality.toInt(&okay);
        if (!okay || (quality != -1 && quality < 0 && quality > 100)) {
            formatter.exitWithMessage(QObject::tr("Invalid quality: `%1`. Supported values are 0 – 100 and -1.").arg(rawQuality));
        }
    } else {
        quality = -1;
    }

    auto const rawWidth = parser.value(widthOption);
    if (!rawWidth.isEmpty()) {
        width = rawWidth.toInt(&okay);
        if (!okay || width <= 0) {
            formatter.exitWithMessage(QObject::tr("Invalid width: `%1`. The width must be a positive whole number.").arg(rawWidth));
        }
    } else {
        width = -1;
    }

    auto const rawHeight = parser.value(heightOption);
    if (!rawHeight.isEmpty()) {
        height = rawHeight.toInt(&okay);
        if (!okay || height <= 0) {
            formatter.exitWithMessage(QObject::tr("Invalid height: `%1`. The heights must be a positive whole number.").arg(rawHeight));
        }
    } else {
        height = -1;
    }

    if (format.isEmpty() && quality < 0 && width < 0 && height < 0) {
        formatter.exitWithMessage(QObject::tr("Please enter at least one conversion option."));
    }
}

QString ConversionParser::getFormat() const { return format; }
int ConversionParser::getQuality() const { return quality; }

int ConversionParser::getWidth() const { return width; }
int ConversionParser::getHeight() const { return height; }


// YesToAllParser:
YesToAllParser::YesToAllParser()
  : yesToAllOption(
        QStringList() << "y" << "yes",
        QObject::tr("Say \"yes\" to all questions. Handy for non-interactive environments.")) {}

QList<QCommandLineOption*> YesToAllParser::listOptions() {
    return QList<QCommandLineOption*>() << &yesToAllOption;
}
void YesToAllParser::parse(QCommandLineParser& parser, const Formatter&) {
    saidYesToAll = parser.isSet(yesToAllOption);
}

bool YesToAllParser::promptYesNoAll(const Formatter& formatter, QString text) {
    if (saidYesToAll) return true;
    if (saidNoToAll) return false;

    auto const yes = QObject::tr("Yes");
    auto const yesLetter = yes.left(1);
    auto const yesText = boldUnderline(yes.at(0)) + yes.mid(1);

    auto const yesToAll = QObject::tr("Yes to All");
    auto const yesToAllLetter = "A";
    auto const yesToAllIndex = yesToAll.indexOf(yesToAllLetter, Qt::CaseInsensitive);
    auto const yesToAllText = yesToAll.left(yesToAllIndex) + boldUnderline(yesToAllLetter) + yesToAll.right(yesToAll.length() - yesToAllIndex - 1);

    auto const no = QObject::tr("No");
    auto const noLetter = no.left(1);
    auto const noText = boldUnderline(no.at(0)) + no.mid(1);

    auto const noToAll = QObject::tr("No to All");
    auto const noToAllLetter = "l";
    auto const noToAllIndex = noToAll.indexOf(noToAllLetter, Qt::CaseInsensitive);
    auto const noToAllText = noToAll.left(noToAllIndex) + boldUnderline(noToAllLetter) + noToAll.right(noToAll.length() - noToAllIndex - 1);

    while (true) {
        formatter.info(text + " [", false);
        formatter.info(yesText, false);
        formatter.info("/", false);
        formatter.info(yesToAllText, false);
        formatter.info("/", false);
        formatter.info(noText, false);
        formatter.info("/", false);
        formatter.info(noToAllText, false);
        formatter.info("] ", false);
        auto const input = QTextStream(stdin).readLine();

        if (input.compare(yes, Qt::CaseInsensitive) == 0) return true;
        if (input.compare(yesToAll, Qt::CaseInsensitive) == 0) {
          saidYesToAll = true;
          return true;
        }
        if (input.compare(no, Qt::CaseInsensitive) == 0) return false;
        if (input.compare(noToAll, Qt::CaseInsensitive) == 0) {
          saidNoToAll = true;
          return false;
        }
        if (input.compare(yesLetter, Qt::CaseInsensitive) == 0) return true;
        if (input.compare(yesToAllLetter, Qt::CaseInsensitive) == 0) {
          saidYesToAll = true;
          return true;
        }
        if (input.compare(noLetter, Qt::CaseInsensitive) == 0) return false;
        if (input.compare(noToAllLetter, Qt::CaseInsensitive) == 0) {
          saidNoToAll = true;
          return false;
        }
    }
}

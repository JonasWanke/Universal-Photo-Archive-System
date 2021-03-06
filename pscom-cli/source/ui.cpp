#include "ui.h"

#include <QDebug>

QString boldUnderline(QString text) { return QString("\033[1;4m") + text + "\033[0m"; }
bool promptBinary(const Formatter& formatter, QString text, QString variant1, QString variant2) {
    assert(!variant1.isEmpty());
    assert(!variant2.isEmpty());
    assert(variant1.compare(variant2, Qt::CaseInsensitive) != 0);

    auto const supportsSingleLetter = variant1.length() > 1 && variant2.length() > 1 && variant1.at(0) != variant2.at(0);
    auto const firstLetter = variant1.left(1);
    auto const first = supportsSingleLetter ? boldUnderline(variant1.at(0)) + variant1.mid(1) : variant1;
    auto const secondLetter = variant2.left(1);
    auto const second = supportsSingleLetter ? boldUnderline(variant2.at(0)) + variant2.mid(1) : variant2;

    while (true) {
        formatter.info(text + " [", false);
        formatter.info(first, false);
        formatter.info("/", false);
        formatter.info(second, false);
        formatter.info("] ", false);
        auto const input = QTextStream(stdin).readLine();

        if (input.compare(first, Qt::CaseInsensitive) == 0) return true;
        if (input.compare(second, Qt::CaseInsensitive) == 0) return false;
        if (input.compare(firstLetter, Qt::CaseInsensitive) == 0) return true;
        if (input.compare(secondLetter, Qt::CaseInsensitive) == 0) return false;
    }
}

Verbosity Formatter::verbosity = Verbosity::debug;

QString Formatter::lineWrap(QString text, QString lineStart) const {
    QStringList result;
    auto remaining = text;
    while (true) {
        auto split = splitLine(remaining);
        result.append(split.first);
        if (split.second.isEmpty()) break;
        remaining = lineStart + split.second;
    }
    return result.join("\n");
}
QPair<QString, QString> Formatter::splitLine(QString text) const {
    auto splitAt = -1;
    auto mustSplit = false;
    for (int i = 0; i < text.length(); i++) {
        auto const character = text.at(i);
        if (character == '\n') {
            splitAt = i;
            mustSplit = true;
            break;
        } else if (character == ' ') {
            splitAt = i;
        }
        if (i >= 80) break;
    }
    if (splitAt < 0 || (!mustSplit && text.length() <= 80)) return QPair<QString, QString>(text, "");

    return QPair<QString, QString>(text.left(splitAt), text.mid(splitAt + 1));
}

QString Formatter::format(QString format, QString text) const {
    return "\033[" + format + "m" + text + "\033[0m";
}

QString Formatter::programTitle(QString text) const {
    return format("32", text);
}
QString Formatter::header(QString text) const {
    return format("1;32", text.toUpper());
}
QString Formatter::emphasize(QString text) const {
    return format("1", text);
}

void Formatter::progress(int index, int total, bool newLine) const {
    auto result = QString("◀");

    auto const width = 20;
    auto const progress = (double) index / total;
    auto const scaled = progress * width;
    auto const full = (int) floor(scaled);
    result += QString("█").repeated(full);
    if (index < total) {
        switch ((int) floor((scaled - full) * 8)) {
            case 0: result += " "; break;
            case 1: result += "▏"; break;
            case 2: result += "▎"; break;
            case 3: result += "▍"; break;
            case 4: result += "▌"; break;
            case 5: result += "▋"; break;
            case 6: result += "▊"; break;
            case 7: result += "▉"; break;
        }
    }
    result += QString(" ").repeated(width - full - 1);
    result += "▶ " + QString::number(progress * 100, 'f', 2) + " % ";
    info(result, newLine);
}

void Formatter::debug(QString text, bool newLine) const {
    if (verbosity > Verbosity::debug) return;

    qDebug().noquote().nospace() << colorDebug(text);
    if (newLine) qDebug().noquote().nospace() << Qt::endl;
}
void Formatter::info(QString text, bool newLine) const {
    if (verbosity > Verbosity::info) return;

    qInfo().noquote().nospace() << colorInfo(text);
    if (newLine) qInfo().noquote().nospace() << Qt::endl;
}
void Formatter::warning(QString text, bool newLine) const {
    if (verbosity > Verbosity::warning) return;

    qWarning().noquote().nospace() << colorWarning(text);
    if (newLine) qWarning().noquote().nospace() << Qt::endl;
}
void Formatter::error(QString text, bool newLine) const {
    if (verbosity > Verbosity::error) return;

    if (!text.isEmpty()) text = QObject::tr("⚠ Error: ") + text;
    qCritical().noquote().nospace() << colorError(text);
    if (newLine) qCritical().noquote().nospace() << Qt::endl;
}
void Formatter::exitWithMessage(QString message) const {
    error(message);
    exit(1);
}
QString Formatter::colorDebug(QString text) const { return format("37;2", text); }
QString Formatter::colorInfo(QString text) const { return text; }
QString Formatter::colorWarning(QString text) const { return format("33", text); }
QString Formatter::colorError(QString text) const { return format("31", text); }


// HelpFormatter:
HelpFormatter::HelpFormatter(QString name, QString shortDescription, QString description, QList<QPair<QString, QString>> seeAlso)
  : name(name),
    shortDescription(shortDescription),
    description(description),
    seeAlso(seeAlso) {}

void HelpFormatter::addOptionGroup(QString title, QList<QCommandLineOption*> options) {
    this->options.append(QPair<QString, QList<QCommandLineOption*>>(title, options));
}
void HelpFormatter::addArguments(QList<QPair<QString, QString>> arguments) {
    this->arguments.append(arguments);
}

void HelpFormatter::print(const Formatter& formatter) const {
    auto usage = "pscom-cli " + name;
    for (auto optionGroup: options) {
        for (auto option: optionGroup.second) {
            QStringList names;
            for (auto name: option->names()) names.append(name.length() == 1 ? "-" + name : "--" + name);
            usage += " ["
                    + names.join("|")
                    + (option->valueName().isEmpty() ? "" : "=<" + option->valueName() + ">")
                    + "]";
        }
    }
    for (auto argument: arguments) usage += " <" + argument.first + ">";
    formatter.info(formatter.lineWrap(QObject::tr("Usage: %1").arg(usage), "  "));

    formatter.info("");
    formatter.info(formatter.lineWrap(shortDescription));

    formatter.info("");
    formatter.info(formatter.lineWrap(description));

    if (!arguments.isEmpty()) {
        formatter.info("");
        formatter.info(formatter.header(QObject::tr("Arguments")));

        for (auto argument: arguments) {
            formatter.info("• " + formatter.emphasize(argument.first + ":") + " " + argument.second);
        }
    }

    if (!options.isEmpty()) {
        formatter.info("");
        formatter.info(formatter.header(QObject::tr("Options")));
        auto isFirst = true;
        for (auto entry: options) {
            if (isFirst) isFirst = false; else formatter.info();

            formatter.info(formatter.lineWrap(formatter.emphasize(entry.first) + ":"));
            for (auto option: entry.second) {
                QStringList names;
                for (auto name: option->names()) {
                    names.append((name.length() == 1 ? "-" : "--") + name);
                }
                auto line = names.join(", ");

                if (!option->valueName().isEmpty()) {
                    line += "=<" + option->valueName() + ">";
                }

                line = "• " + formatter.emphasize(line) + "\n" + option->description();
                formatter.info(formatter.lineWrap(line, "  "));
            }
        }
    }

    if (!seeAlso.isEmpty()) {
        formatter.info("");
        formatter.info(formatter.header(QObject::tr("See also")));
        for (auto entry: seeAlso) {
            formatter.info(formatter.lineWrap("• " + formatter.emphasize(entry.first) + ", " + entry.second, "  "));
        }
    }

    if (name == "convert") {
        formatter.info("");
        formatter.info(QObject::tr("Yay, you've made it to the end!"));
    }
}

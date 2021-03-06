#pragma once

#include "parsers.h"
#include "pscom_adapter.h"
#include "ui.h"

class Command: public MultiParser {
public:
    QList<Parser*> parsers;

public:
    Command(QString name, QString shortDescription, QString description, QList<QPair<QString, QString>> seeAlso = QList<QPair<QString, QString>>(), bool supportsUpa = false);

    static QCoreApplication* app;
    const QString name;
    const QString shortDescription;
    const QString description;
    const QList<QPair<QString, QString>> seeAlso;
    Formatter formatter;
    GlobalsParser globalsParser;

    void addParser(QString groupName, Parser& parser);

    virtual void configureCliParser(QCommandLineParser& parser);
    virtual QList<QCommandLineOption*> listOptions();
    virtual QList<QPair<QString, QString>> listArguments();
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

    void execute();
    virtual void run() = 0;
};

class HelpCommand: public Command {
public:
    HelpCommand();

    static const QString programTitle;

    virtual void run();
};
class VersionCommand: public Command {
public:
    VersionCommand();

    virtual void run();
};
class FeedbackCommand: public Command {
public:
    FeedbackCommand();

    virtual void run();
};

class UpaCommand: public Command {
public:
    UpaCommand(QString name, QString shortDescription, QString description, QList<QPair<QString, QString>> seeAlso = QList<QPair<QString, QString>>());

    PscomAdapter* pscom;

    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

    QString getTargetDirectory(
        const QDate& date,
        const QPair<QString, QString>* locationAndEvent = nullptr
    ) const;
    QString getTargetFile(
        const QString photo,
        const QPair<QString, QString>* locationAndEvent = nullptr
    ) const;
    QString getTargetFile(
        const QString suffix,
        const QDateTime& dateTime,
        const QPair<QString, QString>* locationAndEvent = nullptr
    ) const;
    QString getTargetOutsideFile(
        const QString photo,
        const QString targetDirectory
    ) const;

    void checkIfTargetsExist(
        const QStringList& photos,
        const QPair<QString, QString>* locationAndEvent = nullptr
    ) const;
    void checkIfTargetsExist(const QList<QPair<QString, QString>>& photosAndTargets) const;
};


class ImportCommand: public UpaCommand {
private:
    FilterParser filterParser;
    CopyOrMoveParser copyOrMoveParser;

    QStringList importLocations;

public:
    ImportCommand();

    virtual QList<QPair<QString, QString>> listArguments();
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

    virtual void run();
};

class ExportCommand: public UpaCommand {
private:
    FilterParser filterParser;
    CopyOrMoveParser copyOrMoveParser;

    QString exportLocation;

public:
    ExportCommand();

    virtual QList<QPair<QString, QString>> listArguments();
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

    virtual void run();
};


class ListCommand: public UpaCommand {
private:
    FilterParser filterParser;

public:
    ListCommand();

    virtual void run();
};


class GroupCommand: public UpaCommand {
private:
    FilterParser filterParser;
    CopyOrMoveParser copyOrMoveParser;

    QString location;
    QString event;

public:
    GroupCommand();

    virtual QList<QPair<QString, QString>> listArguments();
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

    virtual void run();
};

class UngroupCommand: public UpaCommand {
private:
    FilterParser filterParser;
    CopyOrMoveParser copyOrMoveParser;

public:
    UngroupCommand();

    virtual void run();
};


class ConvertCommand: public UpaCommand {
private:
    FilterParser filterParser;
    ConversionParser conversionParser;
    YesToAllParser yesToAllParser;

    QString outputName;

public:
    ConvertCommand();

    virtual void run();
};

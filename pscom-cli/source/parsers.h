#pragma once

#include <pscom.h>

#include <QCommandLineOption>
#include <QCommandLineParser>

#include "pscom_adapter.h"

class Parser {
public:
    void configureCliParser(QCommandLineParser& parser);
    virtual QList<QCommandLineOption*> listOptions() = 0;
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter) = 0;
};


class MultiParser: public Parser {
private:
    QList<Parser*> parsers;

public:
    virtual void configureCliParser(QCommandLineParser& parser);
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

    void addParser(Parser& parser);
};


class GlobalsParser: public Parser {
private:
    QCommandLineOption helpOption;
    bool help;
    QCommandLineOption verbosityOption;
    Verbosity verbosity;
    QCommandLineOption localeOption;

public:
    const bool supportsUpa;
private:
    QCommandLineOption upaRootOption;
    QString upaRoot;
    QCommandLineOption dryRunOption;
    bool _dryRun;

public:
    GlobalsParser(bool supportsUpa);

    virtual QList<QCommandLineOption*> listOptions();
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

    bool isHelpRequested() const;
    Verbosity getVerbosity() const;
    QString getUpaRoot() const;
    bool dryRun() const;
};


class FilterParser: public Parser {
public:
    const bool supportsEventLocationOptions;
private:
    QCommandLineOption locationOption;
    QStringList locations;
    QCommandLineOption eventOption;
    QStringList events;
    QCommandLineOption onlyInEventOption;
    bool onlyInEvent;
    QCommandLineOption onlyOutsideEventOption;
    bool onlyOutsideEvent;

    QCommandLineOption regexOption;
    QList<QRegExp> regexes;

    QCommandLineOption dateTimeOption;
    QList<QPair<QDateTime, QDateTime>> dateTimes;

public:
    FilterParser(bool supportsEventLocationOptions = true);

    virtual QList<QCommandLineOption*> listOptions();
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);
private:
    QPair<QDate, QDate> parseDate(const Formatter& formatter, QString date);
    QPair<QTime, QTime> parseTime(const Formatter& formatter, QString time);

public:
    QStringList getLocations();
    QStringList getEvents();
    bool getOnlyInEvent();
    bool getOnlyOutsideEvent();

    QList<QRegExp> getRegexes();

    QList<QPair<QDateTime, QDateTime>> getDateTimes();

    QStringList listMatches(const PscomAdapter& pscom, const QString& upaRoot);
    bool matchesRegexAndDateTime(const PscomAdapter& pscom, const QString& path);
};


class CopyOrMoveParser: public Parser {
private:
    QCommandLineOption copyOption;
    QCommandLineOption moveOption;
    bool _shouldMove;

public:
    CopyOrMoveParser();

    virtual QList<QCommandLineOption*> listOptions();
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

    bool shouldMove();
    bool copyOrMove(PscomAdapter& pscom, const QString& source, const QString& destination);
};


class ConversionParser: public Parser {
private:
    QCommandLineOption formatOption;
    QString format;
    QCommandLineOption qualityOption;
    int quality;

    QCommandLineOption widthOption;
    int width;
    QCommandLineOption heightOption;
    int height;

public:
    ConversionParser();

    virtual QList<QCommandLineOption*> listOptions();
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

public:
    QString getFormat() const;
    int getQuality() const;

    int getWidth() const;
    int getHeight() const;
};


class YesToAllParser: public Parser {
private:
    QCommandLineOption yesToAllOption;
    bool saidYesToAll;
    bool saidNoToAll = false;

public:
    YesToAllParser();

    virtual QList<QCommandLineOption*> listOptions();
    virtual void parse(QCommandLineParser& parser, const Formatter& formatter);

public:
    bool promptYesNoAll(const Formatter& formatter, QString text);
};

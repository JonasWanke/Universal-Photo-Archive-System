#pragma once

#include <QCommandLineOption>

enum Verbosity { debug, info, warning, error, none };

class Formatter {
public:
    static Verbosity verbosity;

private:
    QPair<QString, QString> splitLine(QString text) const;
public:
    QString lineWrap(QString text, QString lineStart = "") const;

private:
    QString format(QString format, QString text) const;
public:
    QString programTitle(QString text) const;
    QString header(QString text) const;
    QString emphasize(QString text) const;

    void progress(int index, int total, bool newLine = true) const;

    void debug(QString text = "", bool newLine = true) const;
    void info(QString text = "", bool newLine = true) const;
    void warning(QString text = "", bool newLine = true) const;
    void error(QString text = "", bool newLine = true) const;
    void exitWithMessage(QString message) const;
private:
    QString colorDebug(QString text) const;
    QString colorInfo(QString text) const;
    QString colorWarning(QString text) const;
    QString colorError(QString text) const;
};

QString boldUnderline(QString text);
bool promptBinary(const Formatter& formatter, QString text, QString variant1, QString variant2);

class HelpFormatter {
private:
    QList<QPair<QString, QList<QCommandLineOption*>>> options;
    QList<QPair<QString, QString>> arguments;

public:
    HelpFormatter(QString name, QString shortDescription, QString description, QList<QPair<QString, QString>> seeAlso);

    const QString name;
    const QString shortDescription;
    const QString description;
    const QList<QPair<QString, QString>> seeAlso;

    void addOptionGroup(QString title, QList<QCommandLineOption*> options);
    void addArguments(QList<QPair<QString, QString>> arguments);

    void print(const Formatter& formatter) const;
};

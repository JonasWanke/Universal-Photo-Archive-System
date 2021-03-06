#pragma once

#include <pscom.h>

#include <QRegExp>

#include "ui.h"
#include "verbosity.h"

class PscomAdapter {
private:
    const bool dryRun;
    const Formatter formatter;

public:
    PscomAdapter(bool dryRun, Formatter formatter);

    QString versionInfo() const;
    QStringList supportedFormats() const;

    bool doesExist(const QString& path) const;
    bool doesDirectoryExist(const QString& path) const;
    bool doesFileExist(const QString& path) const;

    QString getFileSuffix(const QString& path) const;
    bool isSupportedFile(const QString& path) const;
    QString getFilePathWithSuffix(const QString& path, const QString& suffix) const;

    QString getFilePathWithNewFileName(const QString& path, const QDateTime& dateTime, const QString& format) const;
    QString getFilePathWithNewSubfolder(const QString& path, const QDate& date, const QString& format) const;

    bool move(const QString& source, const QString& destination) const;
    bool copy(const QString& source, const QString& destination) const;
    bool remove(const QString& path) const;
    bool createDirectory(const QString& path) const;

    QDateTime getCreationTime(const QString& path) const;

    bool scaleToWidth(const QString& path, const int width) const;
    bool scaleToHeight(const QString& path, const int height) const;
    bool scaleToSize(const QString& path, const int width, const int height) const;

    bool changeFormat(const QString& path, const QString& suffix, const int quality) const;

    QStringList listAll(const QString& path, bool recursive) const;
    QStringList listMatches(const QString& path, const QRegExp& regex, const bool recursive) const;
    QStringList listMatches(const QString& path, const QDateTime& from, const QDateTime& until, bool recursive) const;
};

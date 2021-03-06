#include "pscom_adapter.h"

#include <pscom.h>

#include <QFileInfo>
#include <QRegExp>

PscomAdapter::PscomAdapter(bool dryRun, Formatter formatter): dryRun(dryRun), formatter(formatter) {}

QString PscomAdapter::versionInfo() const { return pscom::vi(); }
QStringList PscomAdapter::supportedFormats() const { return pscom::sf(); }

bool PscomAdapter::doesExist(const QString& path) const {
    formatter.debug(QObject::tr("Checking whether path `%1` exists… ").arg(path), false);
    auto const result = !pscom::ne(path);
    formatter.debug(result ? QObject::tr("It exists.") : QObject::tr("It doesn't exist."));
    return result;
}
bool PscomAdapter::doesDirectoryExist(const QString& path) const {
    formatter.debug(QObject::tr("Checking whether directory `%1` exists… ").arg(path), false);
    auto const result = pscom::de(path);
    formatter.debug(result ? QObject::tr("It exists.") : QObject::tr("It doesn't exist."));
    return result;
}
bool PscomAdapter::doesFileExist(const QString& path) const {
    formatter.debug(QObject::tr("Checking whether file `%1` exists… ").arg(path), false);
    auto const result = pscom::fe(path);
    formatter.debug(result ? QObject::tr("It exists.") : QObject::tr("It doesn't exist."));
    return result;
}

QString PscomAdapter::getFileSuffix(const QString& path) const { return pscom::fs(path); }
bool PscomAdapter::isSupportedFile(const QString& path) const {
    return supportedFormats().contains(getFileSuffix(path));
}
QString PscomAdapter::getFilePathWithSuffix(const QString& path, const QString& suffix) const {
    return pscom::cs(path, suffix);
}

QString PscomAdapter::getFilePathWithNewFileName(const QString& path, const QDateTime& dateTime, const QString& format) const {
    return pscom::fn(path, dateTime, format);
}
QString PscomAdapter::getFilePathWithNewSubfolder(const QString& path, const QDate& date, const QString& format) const {
    return pscom::fp(path, date, format);
}

bool PscomAdapter::move(const QString& source, const QString& destination) const {
    createDirectory(destination);
    formatter.debug(QObject::tr("Moving `%1` to `%2`… ").arg(source, destination), false);
    if (dryRun) {
        formatter.debug(QObject::tr("Simulated to succeed!"));
        return true;
    } else {
        auto const result = pscom::mv(source, destination);
        formatter.debug(result ? QObject::tr("Success!") : QObject::tr("Error."));
        return result;
    }
}
bool PscomAdapter::copy(const QString& source, const QString& destination) const {
    createDirectory(destination);
    formatter.debug(QObject::tr("Copying `%1` to `%2`… ").arg(source, destination), false);
    if (dryRun) {
        formatter.debug(QObject::tr("Simulated to succeed!"));
        return true;
    } else {
        auto const result = pscom::cp(source, destination);
        formatter.debug(result ? QObject::tr("Success!") : QObject::tr("Error."));
        return result;
    }
}
bool PscomAdapter::remove(const QString& path) const {
    formatter.debug(QObject::tr("Deleting `%1`… ").arg(path), false);
    if (dryRun) {
        formatter.debug(QObject::tr("Simulated to succeed!"));
        return true;
    } else {
        auto const result = pscom::rm(path);
        formatter.debug(result ? QObject::tr("Success!") : QObject::tr("Error."));
        return result;
    }
}
bool PscomAdapter::createDirectory(const QString& path) const {
    formatter.debug(QObject::tr("Creating directory `%1`… ").arg(path), false);
    if (dryRun) {
        formatter.debug(QObject::tr("Simulated to succeed!"));
        return true;
    } else {
        auto const result = pscom::mk(path);
        formatter.debug(result ? QObject::tr("Success!") : QObject::tr("Error."));
        return result;
    }
}

QDateTime PscomAdapter::getCreationTime(const QString& path) const { return pscom::et(path); }

bool PscomAdapter::scaleToWidth(const QString& path, const int width) const {
    formatter.debug(QObject::tr("Scaling photo `%1` to width %2… ").arg(path, width), false);
    if (dryRun) {
        formatter.debug(QObject::tr("Simulated to succeed!"));
        return true;
    } else {
        auto const result = pscom::sw(path, width);
        formatter.debug(result ? QObject::tr("Success!") : QObject::tr("Error."));
        return result;
    }
}
bool PscomAdapter::scaleToHeight(const QString& path, const int height) const {
    formatter.debug(QObject::tr("Scaling photo `%1` to height %2… ").arg(path, height), false);
    if (dryRun) {
        formatter.debug(QObject::tr("Simulated to succeed!"));
        return true;
    } else {
        auto const result = pscom::sh(path, height);
        formatter.debug(result ? QObject::tr("Success!") : QObject::tr("Error."));
        return result;
    }
}
bool PscomAdapter::scaleToSize(const QString& path, const int width, const int height) const {
    formatter.debug(QObject::tr("Scaling photo `%1` to size %2 ⋅ %3… ").arg(path).arg(width).arg(height), false);
    if (dryRun) {
        formatter.debug(QObject::tr("Simulated to succeed!"));
        return true;
    } else {
        auto const result = pscom::ss(path, width, height);
        formatter.debug(result ? QObject::tr("Success!") : QObject::tr("Error."));
        return result;
    }
}

bool PscomAdapter::changeFormat(const QString& path, const QString& suffix, const int quality) const {
    formatter.debug(QObject::tr("Converting photo `%1` to format %2 with quality %3… ").arg(path, suffix, QString::number(quality)), false);
    if (dryRun) {
        formatter.debug(QObject::tr("Simulated to succeed!"));
        return true;
    } else {
        if (getFileSuffix(path) == suffix) {
            auto const tempSuffix = suffix == "jpg" ? "png" : "jpg";
            auto result = pscom::cf(path, tempSuffix, 100);
            if (!result) {
                formatter.debug(QObject::tr("Error."));
                return false;
            }
            auto const tempPath = getFilePathWithSuffix(path, tempSuffix);

            result = remove(path);
            if (!result) {
                formatter.debug(QObject::tr("Error."));
                return false;
            }

            result = pscom::cf(tempPath, suffix, quality);
            if (!result) {
                formatter.debug(QObject::tr("Error."));
                return false;
            }

            result = remove(tempPath);
            formatter.debug(result ? QObject::tr("Success!") : QObject::tr("Error."));
            return result;
        }

        auto const result = pscom::cf(path, suffix, quality);
        formatter.debug(result ? QObject::tr("Success!") : QObject::tr("Error."));
        return result;
    }
}

QStringList PscomAdapter::listAll(const QString& path, bool recursive) const {
    return listMatches(path, QRegExp(".*"), recursive);
}
QStringList PscomAdapter::listMatches(const QString& path, const QRegExp& regex, const bool recursive) const {
    return pscom::re(path, regex, recursive);
}
QStringList PscomAdapter::listMatches(const QString& path, const QDateTime& from, const QDateTime& until, bool recursive) const {
    return pscom::dt(path, from, until, recursive);
}

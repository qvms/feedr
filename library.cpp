#include "library.h"

#include <QFileInfo>
#include <QSettings>
#include <QVariant>

#include <algorithm>

#include <zim/archive.h>
#include <zim/error.h>

namespace {

QString canonical(const QString &path)
{
    const QFileInfo info(path);
    const QString canon = info.canonicalFilePath();
    return canon.isEmpty() ? info.absoluteFilePath() : canon;
}

} // namespace

QString probeTitle(const QString &path, qint64 *articleCount)
{
    QString title = QFileInfo(path).completeBaseName();
    try {
        zim::Archive archive(path.toStdString());
        if (articleCount) {
            *articleCount = static_cast<qint64>(archive.getArticleCount());
        }
        try {
            const std::string meta = archive.getMetadata("Title");
            if (!meta.empty()) {
                title = QString::fromStdString(meta);
            }
        } catch (const std::exception &) {
        }
    } catch (const std::exception &) {
        if (articleCount) {
            *articleCount = 0;
        }
    }
    return title;
}

Library::Library(QObject *parent)
    : QObject(parent)
{
    load();
}

QVector<ArchiveRecord> Library::archives() const
{
    return archives_;
}

QStringList Library::recents() const
{
    return recents_;
}

AppSettings Library::settings() const
{
    return settings_;
}

bool Library::contains(const QString &path) const
{
    const QString key = canonical(path);
    for (const ArchiveRecord &record : archives_) {
        if (record.path == key) {
            return true;
        }
    }
    return false;
}

ArchiveRecord Library::recordFor(const QString &path) const
{
    const QString key = canonical(path);
    for (const ArchiveRecord &record : archives_) {
        if (record.path == key) {
            return record;
        }
    }
    ArchiveRecord missing;
    missing.path = key;
    missing.title = QFileInfo(key).completeBaseName();
    return missing;
}

bool Library::addArchive(const QString &path, QString *error)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        if (error) {
            *error = QStringLiteral("File does not exist: %1").arg(path);
        }
        return false;
    }
    if (info.suffix().compare(QStringLiteral("zim"), Qt::CaseInsensitive) != 0) {
        if (error) {
            *error = QStringLiteral("Not a .zim file: %1").arg(path);
        }
        return false;
    }

    const QString key = canonical(path);
    qint64 articles = 0;
    QString title;
    try {
        zim::Archive archive(key.toStdString());
        articles = static_cast<qint64>(archive.getArticleCount());
        title = probeTitle(key, nullptr);
    } catch (const std::exception &ex) {
        if (error) {
            *error = QStringLiteral("Could not open ZIM: %1").arg(QString::fromUtf8(ex.what()));
        }
        return false;
    }

    const QDateTime now = QDateTime::currentDateTime();
    for (ArchiveRecord &record : archives_) {
        if (record.path == key) {
            record.title = title;
            record.articleCount = articles;
            record.bytes = info.size();
            record.lastOpened = now;
            saveArchives();
            rememberRecent(key);
            emit archivesChanged();
            return true;
        }
    }

    ArchiveRecord record;
    record.path = key;
    record.title = title;
    record.articleCount = articles;
    record.bytes = info.size();
    record.added = now;
    record.lastOpened = now;
    archives_.prepend(record);
    saveArchives();
    rememberRecent(key);
    emit archivesChanged();
    return true;
}

void Library::removeArchive(const QString &path)
{
    const QString key = canonical(path);
    const auto before = archives_.size();
    archives_.erase(std::remove_if(archives_.begin(), archives_.end(),
                                   [&key](const ArchiveRecord &record) { return record.path == key; }),
                    archives_.end());
    if (archives_.size() != before) {
        saveArchives();
        emit archivesChanged();
    }
}

void Library::touch(const QString &path)
{
    const QString key = canonical(path);
    for (ArchiveRecord &record : archives_) {
        if (record.path == key) {
            record.lastOpened = QDateTime::currentDateTime();
            saveArchives();
            emit archivesChanged();
            break;
        }
    }
    rememberRecent(key);
}

void Library::rememberRecent(const QString &path)
{
    const QString key = canonical(path);
    recents_.removeAll(key);
    recents_.prepend(key);
    while (recents_.size() > settings_.recentLimit) {
        recents_.removeLast();
    }
    saveRecents();
    emit recentsChanged();
}

void Library::setSettings(const AppSettings &settings)
{
    settings_ = settings;
    if (settings_.searchLimit < 1) {
        settings_.searchLimit = 1;
    }
    if (settings_.searchLimit > 200) {
        settings_.searchLimit = 200;
    }
    if (settings_.fontSize < 10) {
        settings_.fontSize = 10;
    }
    if (settings_.fontSize > 28) {
        settings_.fontSize = 28;
    }
    if (settings_.recentLimit < 4) {
        settings_.recentLimit = 4;
    }
    while (recents_.size() > settings_.recentLimit) {
        recents_.removeLast();
    }
    saveSettings();
    saveRecents();
    emit settingsChanged();
}

void Library::saveSession(const QStringList &openPaths, const QByteArray &geometry, const QByteArray &state)
{
    session_ = openPaths;
    geometry_ = geometry;
    windowState_ = state;
    QSettings settings;
    settings.beginGroup(QStringLiteral("session"));
    settings.setValue(QStringLiteral("tabs"), session_);
    settings.setValue(QStringLiteral("geometry"), geometry_);
    settings.setValue(QStringLiteral("state"), windowState_);
    settings.endGroup();
}

QStringList Library::lastSession() const
{
    return session_;
}

QByteArray Library::geometry() const
{
    return geometry_;
}

QByteArray Library::windowState() const
{
    return windowState_;
}

void Library::load()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("library"));
    const int count = settings.beginReadArray(QStringLiteral("archives"));
    archives_.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        ArchiveRecord record;
        record.path = settings.value(QStringLiteral("path")).toString();
        record.title = settings.value(QStringLiteral("title")).toString();
        record.articleCount = settings.value(QStringLiteral("articleCount")).toLongLong();
        record.bytes = settings.value(QStringLiteral("bytes")).toLongLong();
        record.added = settings.value(QStringLiteral("added")).toDateTime();
        record.lastOpened = settings.value(QStringLiteral("lastOpened")).toDateTime();
        if (!record.path.isEmpty()) {
            archives_.push_back(record);
        }
    }
    settings.endArray();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("recents"));
    recents_ = settings.value(QStringLiteral("paths")).toStringList();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("settings"));
    settings_.reopenTabs = settings.value(QStringLiteral("reopenTabs"), true).toBool();
    settings_.showExplorer = settings.value(QStringLiteral("showExplorer"), true).toBool();
    settings_.lightMode = settings.value(QStringLiteral("lightMode"), false).toBool();
    settings_.searchLimit = settings.value(QStringLiteral("searchLimit"), 25).toInt();
    settings_.fontSize = settings.value(QStringLiteral("fontSize"), 15).toInt();
    settings_.recentLimit = settings.value(QStringLiteral("recentLimit"), 12).toInt();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("session"));
    session_ = settings.value(QStringLiteral("tabs")).toStringList();
    geometry_ = settings.value(QStringLiteral("geometry")).toByteArray();
    windowState_ = settings.value(QStringLiteral("state")).toByteArray();
    settings.endGroup();
}

void Library::saveArchives() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("library"));
    settings.beginWriteArray(QStringLiteral("archives"));
    for (int i = 0; i < archives_.size(); ++i) {
        settings.setArrayIndex(i);
        const ArchiveRecord &record = archives_.at(i);
        settings.setValue(QStringLiteral("path"), record.path);
        settings.setValue(QStringLiteral("title"), record.title);
        settings.setValue(QStringLiteral("articleCount"), record.articleCount);
        settings.setValue(QStringLiteral("bytes"), record.bytes);
        settings.setValue(QStringLiteral("added"), record.added);
        settings.setValue(QStringLiteral("lastOpened"), record.lastOpened);
    }
    settings.endArray();
    settings.endGroup();
}

void Library::saveRecents() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("recents"));
    settings.setValue(QStringLiteral("paths"), recents_);
    settings.endGroup();
}

void Library::saveSettings() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("settings"));
    settings.setValue(QStringLiteral("reopenTabs"), settings_.reopenTabs);
    settings.setValue(QStringLiteral("showExplorer"), settings_.showExplorer);
    settings.setValue(QStringLiteral("lightMode"), settings_.lightMode);
    settings.setValue(QStringLiteral("searchLimit"), settings_.searchLimit);
    settings.setValue(QStringLiteral("fontSize"), settings_.fontSize);
    settings.setValue(QStringLiteral("recentLimit"), settings_.recentLimit);
    settings.endGroup();
}

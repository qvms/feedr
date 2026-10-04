#ifndef LIBRARY_H
#define LIBRARY_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct ArchiveRecord {
    QString path;
    QString title;
    qint64 articleCount = 0;
    qint64 bytes = 0;
    QDateTime added;
    QDateTime lastOpened;
};

struct AppSettings {
    bool reopenTabs = true;
    bool showExplorer = true;
    bool lightMode = false;
    int searchLimit = 25;
    int fontSize = 15;
    int recentLimit = 12;
};

class Library : public QObject
{
    Q_OBJECT

public:
    explicit Library(QObject *parent = nullptr);

    QVector<ArchiveRecord> archives() const;
    QStringList recents() const;
    AppSettings settings() const;

    bool contains(const QString &path) const;
    ArchiveRecord recordFor(const QString &path) const;

    bool addArchive(const QString &path, QString *error = nullptr);
    void removeArchive(const QString &path);
    void touch(const QString &path);
    void rememberRecent(const QString &path);

    void setSettings(const AppSettings &settings);
    void saveSession(const QStringList &openPaths, const QByteArray &geometry, const QByteArray &state);
    QStringList lastSession() const;
    QByteArray geometry() const;
    QByteArray windowState() const;

signals:
    void archivesChanged();
    void recentsChanged();
    void settingsChanged();

private:
    void load();
    void saveArchives() const;
    void saveRecents() const;
    void saveSettings() const;

    QVector<ArchiveRecord> archives_;
    QStringList recents_;
    AppSettings settings_;
    QStringList session_;
    QByteArray geometry_;
    QByteArray windowState_;
};

QString probeTitle(const QString &path, qint64 *articleCount = nullptr);

#endif

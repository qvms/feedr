#ifndef HTMLAREA_H
#define HTMLAREA_H

#include <QTextBrowser>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <optional>
#include <vector>

#include <zim/archive.h>
#include <zim/entry.h>

class HtmlArea : public QTextBrowser
{
    Q_OBJECT

public:
    explicit HtmlArea(QWidget *parent = nullptr);

    bool openArchive(const QString &path, QString *error = nullptr);
    QString archivePath() const;
    QString archiveTitle() const;
    bool hasArchive() const;

    void setSearchLimit(int limit);
    void setReaderFontSize(int points);
    void setLightMode(bool light);

    void searchArticle(const QString &term);

public slots:
    void linking(const QUrl &txt);
    void sourceChange(const QUrl &txt);
    void getRandom();
    void goToTop();
    void goBack();
    void goForward();
    void goHome();

signals:
    void statusMessage(const QString &message);
    void navigationChanged(bool canBack, bool canForward);

protected:
    QVariant loadResource(int type, const QUrl &name) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    const zim::Archive &archive() const;
    std::optional<zim::Entry> getRandomArticle();
    std::optional<zim::Entry> getArticleFromUrl(const QString &url);
    std::vector<zim::Entry> searchArticleFromTitle(const QString &phrase);
    void setNewContent(int method, const QString &str = QString(), bool recordHistory = true);
    void showHtml(const QString &html);
    QString appearanceStyle() const;
    void remember(const QString &path);
    void emitNavigation();

    std::optional<zim::Archive> archive_;
    QString archivePath_;
    QString archiveTitle_;
    QString url;
    int searchLimit_ = 25;
    bool lightMode_ = false;
    QStringList history_;
    int historyIndex_ = -1;
    bool navigatingHistory_ = false;
};

#endif

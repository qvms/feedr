#include "htmlarea.h"

#include <QDebug>
#include <QFileInfo>
#include <QFont>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QUrl>

#include <algorithm>
#include <exception>
#include <limits>
#include <string>

#include <zim/blob.h>
#include <zim/error.h>
#include <zim/item.h>
#include <zim/search.h>
#include <zim/suggestion.h>

Q_LOGGING_CATEGORY(feedr_htmlarea, "feedr.htmlarea")

namespace {

constexpr int kRedirectGuard = 16;

bool isArticleEntry(const zim::Entry &entry)
{
    const std::string path = entry.getPath();
    if (path.size() >= 2 && path[1] == '/') {
        const char ns = path[0];
        return ns == 'A' || ns == 'C';
    }
    return true;
}

QString htmlEscape(const QString &text)
{
    return text.toHtmlEscaped();
}

std::optional<zim::Entry> followRedirects(zim::Entry entry)
{
    int guard = kRedirectGuard;
    while (entry.isRedirect() && guard-- > 0) {
        entry = entry.getRedirectEntry();
    }
    if (entry.isRedirect()) {
        return std::nullopt;
    }
    return entry;
}

} // namespace

HtmlArea::HtmlArea(QWidget *parent)
    : QTextBrowser(parent)
{
    setOpenLinks(false);
    setOpenExternalLinks(false);
    setReaderFontSize(15);
}

bool HtmlArea::openArchive(const QString &path, QString *error)
{
    try {
        archive_.emplace(path.toStdString());
        archivePath_ = path;
        archiveTitle_ = QFileInfo(path).completeBaseName();
        try {
            const std::string meta = archive_->getMetadata("Title");
            if (!meta.empty()) {
                archiveTitle_ = QString::fromStdString(meta);
            }
        } catch (const std::exception &) {
        }
        history_.clear();
        historyIndex_ = -1;
        url.clear();
        emit statusMessage(QStringLiteral("%1 · %2 articles")
                               .arg(archiveTitle_)
                               .arg(archive_->getArticleCount()));
        if (archive_->hasMainEntry()) {
            const zim::Entry main = archive_->getMainEntry();
            setNewContent(1, QString::fromStdString(main.getPath()));
        } else {
            getRandom();
        }
        return true;
    } catch (const std::exception &ex) {
        archive_.reset();
        archivePath_.clear();
        if (error) {
            *error = QString::fromUtf8(ex.what());
        }
        return false;
    }
}

QString HtmlArea::archivePath() const
{
    return archivePath_;
}

QString HtmlArea::archiveTitle() const
{
    return archiveTitle_;
}

bool HtmlArea::hasArchive() const
{
    return archive_.has_value();
}

void HtmlArea::setSearchLimit(int limit)
{
    searchLimit_ = std::clamp(limit, 1, 200);
}

void HtmlArea::setReaderFontSize(int points)
{
    QFont font = this->font();
    font.setPointSize(std::clamp(points, 10, 28));
    setFont(font);
}

void HtmlArea::setLightMode(bool light)
{
    lightMode_ = light;
    const QString current = toHtml();
    if (!current.isEmpty()) {
        QString stripped = current;
        const int start = stripped.indexOf(QStringLiteral("<style id=\"feedr-chrome\">"));
        if (start >= 0) {
            const int end = stripped.indexOf(QStringLiteral("</style>"), start);
            if (end > start) {
                stripped.remove(start, end + 8 - start);
            }
        }
        showHtml(stripped);
    }
}

QString HtmlArea::appearanceStyle() const
{
    if (lightMode_) {
        return QStringLiteral(
            "<style id=\"feedr-chrome\">body{background:#fffdf8;color:#1c1915;} a{color:#1f6b52;}</style>");
    }
    return QStringLiteral(
        "<style id=\"feedr-chrome\">html,body{background:#161410!important;color:#e6e0d4!important;}"
        "a{color:#9dccb4!important;} table,th,td{border-color:#3a342c;}</style>");
}

void HtmlArea::showHtml(const QString &html)
{
    setHtml(appearanceStyle() + html);
}

void HtmlArea::linking(const QUrl &txt)
{
    const QString str = txt.toString(QUrl::FullyDecoded);
    qCDebug(feedr_htmlarea) << str;

    if (str.startsWith(QLatin1Char('#'))) {
        scrollToAnchor(str.mid(1));
        return;
    }
    if (txt.scheme() == QLatin1String("http") || txt.scheme() == QLatin1String("https")) {
        emit statusMessage(QStringLiteral("External link ignored: %1").arg(txt.toString()));
        return;
    }

    setSource(source());
    setNewContent(1, str);
}

void HtmlArea::setNewContent(int method, const QString &str, bool recordHistory)
{
    if (!archive_) {
        QMessageBox::information(this, QStringLiteral("Error"), QStringLiteral("No open ZIM archives"));
        return;
    }

    setEnabled(false);
    const QString current = toHtml();
    showHtml(QStringLiteral("<html><body><p>Loading …</p></body></html>"));
    qApp->processEvents();

    auto restore = [this]() {
        qApp->processEvents();
        setEnabled(true);
    };

    std::optional<zim::Entry> article;
    try {
        if (method == 0) {
            article = getRandomArticle();
        } else if (method == 1) {
            article = getArticleFromUrl(str);
        } else if (method == 2) {
            const std::vector<zim::Entry> results = searchArticleFromTitle(str);
            if (results.size() == 1) {
                article = results.front();
            } else if (results.size() > 1) {
                QString res = QStringLiteral("<html><body><p>Results for <b>%1</b></p><ul>").arg(htmlEscape(str));
                for (const zim::Entry &entry : results) {
                    const QString path = QString::fromStdString(entry.getPath());
                    const QString title = QString::fromStdString(entry.getTitle());
                    const QString label = title.isEmpty() ? path : title;
                    res += QStringLiteral("<li><a href=\"%1\">%2</a></li>")
                               .arg(htmlEscape(path), htmlEscape(label));
                }
                res += QStringLiteral("</ul></body></html>");
                showHtml(res);
                emit statusMessage(QStringLiteral("%1 results").arg(results.size()));
                restore();
                return;
            }
        }
    } catch (const std::exception &e) {
        qCWarning(feedr_htmlarea) << "lookup failed:" << e.what();
        article.reset();
    }

    if (!article) {
        showHtml(current);
        const QString errText = QStringLiteral("The article you requested (%1) could not be found.").arg(str);
        qCDebug(feedr_htmlarea) << errText;
        restore();
        QMessageBox::information(this, QStringLiteral("Error"), errText, QMessageBox::Close);
        return;
    }

    try {
        const std::optional<zim::Entry> resolved = followRedirects(*article);
        if (!resolved) {
            throw std::runtime_error("redirect loop");
        }

        url = QString::fromStdString(resolved->getPath());
        const zim::Item item = resolved->getItem(true);
        const zim::Blob blob = item.getData();

        std::string content = "<a name=\"top\"></a>";
        if (blob.data() != nullptr && blob.size() > 0) {
            content.append(blob.data(), static_cast<std::size_t>(blob.size()));
        }
        showHtml(QString::fromUtf8(content.data(), static_cast<qsizetype>(content.size())));
        if (recordHistory && !navigatingHistory_) {
            remember(url);
        }
        const QString title = QString::fromStdString(resolved->getTitle());
        emit statusMessage(title.isEmpty() ? url : title);
        qCDebug(feedr_htmlarea) << "string:" << str;
    } catch (const std::exception &e) {
        showHtml(current);
        const QString errText = QStringLiteral("The article you requested (%1) could not be read: %2")
                                    .arg(str, QString::fromUtf8(e.what()));
        qCWarning(feedr_htmlarea) << errText;
        restore();
        QMessageBox::information(this, QStringLiteral("Error"), errText, QMessageBox::Close);
        return;
    }

    restore();
}

void HtmlArea::remember(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }
    if (historyIndex_ >= 0 && history_.value(historyIndex_) == path) {
        emitNavigation();
        return;
    }
    while (history_.size() > historyIndex_ + 1) {
        history_.removeLast();
    }
    history_.push_back(path);
    historyIndex_ = history_.size() - 1;
    emitNavigation();
}

void HtmlArea::emitNavigation()
{
    emit navigationChanged(historyIndex_ > 0, historyIndex_ >= 0 && historyIndex_ < history_.size() - 1);
}

void HtmlArea::getRandom()
{
    if (isEnabled()) {
        setNewContent(0);
    }
}

void HtmlArea::searchArticle(const QString &term)
{
    if (isEnabled() && !term.trimmed().isEmpty()) {
        setNewContent(2, term.trimmed());
    }
}

void HtmlArea::goToTop()
{
    scrollToAnchor(QStringLiteral("top"));
}

void HtmlArea::goBack()
{
    if (historyIndex_ <= 0) {
        return;
    }
    --historyIndex_;
    navigatingHistory_ = true;
    setNewContent(1, history_.at(historyIndex_), false);
    navigatingHistory_ = false;
    emitNavigation();
}

void HtmlArea::goForward()
{
    if (historyIndex_ < 0 || historyIndex_ >= history_.size() - 1) {
        return;
    }
    ++historyIndex_;
    navigatingHistory_ = true;
    setNewContent(1, history_.at(historyIndex_), false);
    navigatingHistory_ = false;
    emitNavigation();
}

void HtmlArea::goHome()
{
    if (!archive_) {
        return;
    }
    try {
        if (archive_->hasMainEntry()) {
            setNewContent(1, QString::fromStdString(archive_->getMainEntry().getPath()));
            return;
        }
    } catch (const std::exception &) {
    }
    getRandom();
}

void HtmlArea::sourceChange(const QUrl &txt)
{
    qCDebug(feedr_htmlarea) << "source:" << txt.toString();
}

QVariant HtmlArea::loadResource(int type, const QUrl &name)
{
    if (type != QTextDocument::ImageResource || !archive_) {
        return QVariant();
    }

    try {
        const std::optional<zim::Entry> article = getArticleFromUrl(name.toString(QUrl::FullyDecoded));
        if (!article) {
            return QVariant();
        }

        const zim::Item item = article->getItem(true);
        QString mtype = QString::fromStdString(item.getMimetype());
        const zim::Blob blob = item.getData();
        if (blob.data() == nullptr || blob.size() == 0) {
            return QVariant();
        }

        const auto nbytes = static_cast<qsizetype>(
            std::min(blob.size(), static_cast<zim::size_type>(std::numeric_limits<int>::max())));
        const QByteArray data(blob.data(), static_cast<int>(nbytes));

        QImage img;
        mtype.remove(QStringLiteral("image/"));
        if (!img.loadFromData(data, mtype.toUpper().toLatin1().constData())) {
            img.loadFromData(data);
        }
        if (!img.isNull()) {
            return img;
        }
    } catch (const std::exception &e) {
        qCWarning(feedr_htmlarea) << "image load failed:" << e.what();
    }

    return QVariant();
}

const zim::Archive &HtmlArea::archive() const
{
    if (!archive_) {
        throw std::runtime_error("no ZIM archive open");
    }
    return *archive_;
}

std::optional<zim::Entry> HtmlArea::getRandomArticle()
{
    const zim::Archive &file = archive();
    for (int attempt = 0; attempt < 8; ++attempt) {
        const std::optional<zim::Entry> resolved = followRedirects(file.getRandomEntry());
        if (!resolved) {
            continue;
        }
        if (QString::fromStdString(resolved->getPath()) != url) {
            return resolved;
        }
    }
    return followRedirects(file.getRandomEntry());
}

std::optional<zim::Entry> HtmlArea::getArticleFromUrl(const QString &requested)
{
    const zim::Archive &file = archive();

    QString term = QUrl::fromPercentEncoding(requested.toUtf8());
    term.replace(QLatin1Char('+'), QLatin1Char(' '));
    while (term.startsWith(QLatin1Char('/'))) {
        term.remove(0, 1);
    }
    if (term.isEmpty()) {
        return std::nullopt;
    }

    std::vector<std::string> candidates;
    candidates.push_back(term.toStdString());
    const int slash = term.indexOf(QLatin1Char('/'));
    if (slash == 1) {
        candidates.push_back(term.mid(2).toStdString());
    } else if (!term.contains(QLatin1Char('/'))) {
        candidates.push_back(std::string("A/") + term.toStdString());
        candidates.push_back(std::string("C/") + term.toStdString());
        candidates.push_back(std::string("I/") + term.toStdString());
    }

    for (const std::string &path : candidates) {
        if (!file.hasEntryByPath(path)) {
            continue;
        }
        try {
            return file.getEntryByPath(path);
        } catch (const zim::EntryNotFound &) {
        }
    }

    if (file.hasEntryByTitle(term.toStdString())) {
        try {
            return file.getEntryByTitle(term.toStdString());
        } catch (const zim::EntryNotFound &) {
        }
    }
    return std::nullopt;
}

std::vector<zim::Entry> HtmlArea::searchArticleFromTitle(const QString &phrase)
{
    const zim::Archive &file = archive();
    const std::string query = phrase.toUtf8().toStdString();
    std::vector<zim::Entry> out;
    out.reserve(static_cast<std::size_t>(searchLimit_));

    auto pushUnique = [&out](const zim::Entry &entry) {
        if (!isArticleEntry(entry)) {
            return;
        }
        const std::string path = entry.getPath();
        for (const zim::Entry &existing : out) {
            if (existing.getPath() == path) {
                return;
            }
        }
        out.push_back(entry);
    };

    try {
        zim::SuggestionSearcher suggester(file);
        zim::SuggestionSearch search = suggester.suggest(query);
        const zim::SuggestionResultSet results = search.getResults(0, searchLimit_);
        for (auto it = results.begin(); it != results.end() && static_cast<int>(out.size()) < searchLimit_; ++it) {
            pushUnique(it.getEntry());
        }
    } catch (const std::exception &e) {
        qCWarning(feedr_htmlarea) << "suggestion search failed:" << e.what();
    }

    if (out.empty()) {
        try {
            int count = 0;
            for (const zim::Entry &entry : file.findByTitle(query)) {
                pushUnique(entry);
                if (++count >= searchLimit_ || static_cast<int>(out.size()) >= searchLimit_) {
                    break;
                }
            }
        } catch (const std::exception &e) {
            qCWarning(feedr_htmlarea) << "title prefix search failed:" << e.what();
        }
    }

    if (out.empty()) {
        try {
            zim::Searcher searcher(file);
            zim::Search search = searcher.search(zim::Query(query));
            const zim::SearchResultSet results = search.getResults(0, searchLimit_);
            for (auto it = results.begin(); it != results.end() && static_cast<int>(out.size()) < searchLimit_; ++it) {
                pushUnique(*it);
            }
        } catch (const std::exception &e) {
            qCWarning(feedr_htmlarea) << "full-text search failed:" << e.what();
        }
    }

    qCDebug(feedr_htmlarea) << "term:" << phrase << "hits:" << out.size();
    return out;
}

void HtmlArea::keyPressEvent(QKeyEvent *event)
{
    if (event == nullptr) {
        return;
    }

    int dx = 0;
    int dy = 0;
    switch (event->key()) {
    case Qt::Key_Right:
        dx = 5;
        break;
    case Qt::Key_Left:
        dx = -5;
        break;
    case Qt::Key_Down:
        dy = 5;
        break;
    case Qt::Key_Up:
        dy = -5;
        break;
    default:
        QTextBrowser::keyPressEvent(event);
        return;
    }

    const QPoint pos = QCursor::pos();
    QCursor::setPos(pos.x() + dx, pos.y() + dy);
}

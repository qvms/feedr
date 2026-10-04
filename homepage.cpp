#include "homepage.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QString humanSize(qint64 bytes)
{
    const double value = static_cast<double>(bytes);
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(value / 1024.0, 0, 'f', 1);
    }
    if (bytes < 1024LL * 1024 * 1024) {
        return QStringLiteral("%1 MB").arg(value / (1024.0 * 1024.0), 0, 'f', 1);
    }
    return QStringLiteral("%1 GB").arg(value / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

} // namespace

HomePage::HomePage(Library *library, QWidget *parent)
    : QWidget(parent)
    , library_(library)
{
    setAcceptDrops(true);
    setObjectName(QStringLiteral("homePage"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(16);

    auto *title = new QLabel(QStringLiteral("feedr"), this);
    title->setObjectName(QStringLiteral("homeTitle"));
    auto *subtitle = new QLabel(QStringLiteral("Offline ZIM library. Drop a .zim file anywhere, or open one from the explorer."), this);
    subtitle->setWordWrap(true);
    subtitle->setObjectName(QStringLiteral("homeSubtitle"));

    auto *actions = new QHBoxLayout();
    auto *addButton = new QPushButton(QStringLiteral("Add archive"), this);
    auto *openButton = new QPushButton(QStringLiteral("Open file…"), this);
    addButton->setObjectName(QStringLiteral("primaryButton"));
    connect(addButton, &QPushButton::clicked, this, &HomePage::addRequested);
    connect(openButton, &QPushButton::clicked, this, &HomePage::browseRequested);
    actions->addWidget(addButton);
    actions->addWidget(openButton);
    actions->addStretch(1);

    root->addWidget(title);
    root->addWidget(subtitle);
    root->addLayout(actions);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *body = new QWidget(scroll);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(18);

    auto *libraryHeading = new QLabel(QStringLiteral("Library"), body);
    libraryHeading->setObjectName(QStringLiteral("sectionHeading"));
    emptyLabel_ = new QLabel(QStringLiteral("No archives yet. Add a file."), body);
    emptyLabel_->setObjectName(QStringLiteral("emptyLabel"));
    archiveLayout_ = new QVBoxLayout();
    archiveLayout_->setSpacing(10);

    auto *recentHeading = new QLabel(QStringLiteral("Previous files"), body);
    recentHeading->setObjectName(QStringLiteral("sectionHeading"));
    recentLayout_ = new QVBoxLayout();
    recentLayout_->setSpacing(6);

    bodyLayout->addWidget(libraryHeading);
    bodyLayout->addWidget(emptyLabel_);
    bodyLayout->addLayout(archiveLayout_);
    bodyLayout->addSpacing(8);
    bodyLayout->addWidget(recentHeading);
    bodyLayout->addLayout(recentLayout_);
    bodyLayout->addStretch(1);
    scroll->setWidget(body);
    root->addWidget(scroll, 1);

    connect(library_, &Library::archivesChanged, this, &HomePage::rebuild);
    connect(library_, &Library::recentsChanged, this, &HomePage::rebuild);
    rebuild();
}

void HomePage::rebuild()
{
    while (QLayoutItem *item = archiveLayout_->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    while (QLayoutItem *item = recentLayout_->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    const QVector<ArchiveRecord> archives = library_->archives();
    emptyLabel_->setVisible(archives.isEmpty());
    for (const ArchiveRecord &record : archives) {
        archiveLayout_->addWidget(makeCard(record));
    }

    const QStringList recents = library_->recents();
    if (recents.isEmpty()) {
        auto *none = new QLabel(QStringLiteral("Nothing to see here"), this);
        none->setObjectName(QStringLiteral("emptyLabel"));
        recentLayout_->addWidget(none);
        return;
    }
    for (const QString &path : recents) {
        recentLayout_->addWidget(makeRecentRow(path));
    }
}

QWidget *HomePage::makeCard(const ArchiveRecord &record)
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("archiveCard"));
    auto *layout = new QHBoxLayout(card);

    auto *text = new QVBoxLayout();
    auto *name = new QLabel(record.title.isEmpty() ? QFileInfo(record.path).completeBaseName() : record.title, card);
    name->setObjectName(QStringLiteral("cardTitle"));
    name->setWordWrap(true);
    const QString meta = QStringLiteral("%1 articles  ·  %2  ·  %3")
                             .arg(record.articleCount)
                             .arg(humanSize(record.bytes))
                             .arg(record.lastOpened.isValid()
                                      ? record.lastOpened.toString(QStringLiteral("yyyy-MM-dd hh:mm"))
                                      : QStringLiteral("never opened"));
    auto *detail = new QLabel(meta, card);
    detail->setObjectName(QStringLiteral("cardMeta"));
    auto *path = new QLabel(record.path, card);
    path->setObjectName(QStringLiteral("cardPath"));
    path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    path->setWordWrap(true);
    text->addWidget(name);
    text->addWidget(detail);
    text->addWidget(path);

    auto *open = new QPushButton(QStringLiteral("Open"), card);
    open->setObjectName(QStringLiteral("primaryButton"));
    auto *remove = new QPushButton(QStringLiteral("Remove"), card);
    const QString stored = record.path;
    connect(open, &QPushButton::clicked, this, [this, stored]() { emit openRequested(stored); });
    connect(remove, &QPushButton::clicked, this, [this, stored]() { library_->removeArchive(stored); });

    layout->addLayout(text, 1);
    layout->addWidget(open);
    layout->addWidget(remove);
    return card;
}

QWidget *HomePage::makeRecentRow(const QString &path)
{
    auto *row = new QFrame(this);
    row->setObjectName(QStringLiteral("recentRow"));
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(10, 6, 10, 6);
    auto *label = new QLabel(QStringLiteral("%1\n%2").arg(QFileInfo(path).fileName(), path), row);
    label->setWordWrap(true);
    auto *open = new QPushButton(QStringLiteral("Open"), row);
    const bool missing = !QFileInfo::exists(path);
    open->setEnabled(!missing);
    if (missing) {
        label->setText(label->text() + QStringLiteral("\n(missing)"));
    }
    connect(open, &QPushButton::clicked, this, [this, path]() { emit openRequested(path); });
    layout->addWidget(label, 1);
    layout->addWidget(open);
    return row;
}

void HomePage::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void HomePage::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QString path = url.toLocalFile();
        if (QFileInfo(path).suffix().compare(QStringLiteral("zim"), Qt::CaseInsensitive) == 0) {
            emit openRequested(path);
        }
    }
    event->acceptProposedAction();
}

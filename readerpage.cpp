#include "readerpage.h"

#include "htmlarea.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

ReaderPage::ReaderPage(const QString &path, int fontSize, int searchLimit, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *bar = new QHBoxLayout();
    bar->setContentsMargins(8, 6, 8, 6);
    auto *back = new QPushButton(QStringLiteral("Back"), this);
    auto *forward = new QPushButton(QStringLiteral("Forward"), this);
    auto *home = new QPushButton(QStringLiteral("Home"), this);
    auto *random = new QPushButton(QStringLiteral("Random"), this);
    auto *top = new QPushButton(QStringLiteral("Top"), this);
    search_ = new QLineEdit(this);
    search_->setPlaceholderText(QStringLiteral("Search this archive"));
    search_->setClearButtonEnabled(true);
    auto *find = new QPushButton(QStringLiteral("Search"), this);

    bar->addWidget(back);
    bar->addWidget(forward);
    bar->addWidget(home);
    bar->addWidget(random);
    bar->addWidget(top);
    bar->addWidget(search_, 1);
    bar->addWidget(find);
    layout->addLayout(bar);

    reader_ = new HtmlArea(this);
    reader_->setSearchLimit(searchLimit);
    reader_->setReaderFontSize(fontSize);
    reader_->setLightMode(false);
    layout->addWidget(reader_, 1);

    connect(back, &QPushButton::clicked, reader_, &HtmlArea::goBack);
    connect(forward, &QPushButton::clicked, reader_, &HtmlArea::goForward);
    connect(home, &QPushButton::clicked, reader_, &HtmlArea::goHome);
    connect(random, &QPushButton::clicked, reader_, &HtmlArea::getRandom);
    connect(top, &QPushButton::clicked, reader_, &HtmlArea::goToTop);
    connect(find, &QPushButton::clicked, this, [this]() { reader_->searchArticle(search_->text()); });
    connect(search_, &QLineEdit::returnPressed, this, [this]() { reader_->searchArticle(search_->text()); });
    connect(reader_, &HtmlArea::statusMessage, this, &ReaderPage::statusMessage);
    connect(reader_, &HtmlArea::navigationChanged, this, [back, forward](bool canBack, bool canForward) {
        back->setEnabled(canBack);
        forward->setEnabled(canForward);
    });
    back->setEnabled(false);
    forward->setEnabled(false);

    QString error;
    ok_ = reader_->openArchive(path, &error);
    error_ = error;
    if (!ok_) {
        auto *banner = new QLabel(QStringLiteral("Could not open %1\n%2").arg(path, error), this);
        banner->setWordWrap(true);
        layout->insertWidget(1, banner);
    }
}

HtmlArea *ReaderPage::reader() const
{
    return reader_;
}

QString ReaderPage::path() const
{
    return reader_->archivePath();
}

bool ReaderPage::ok() const
{
    return ok_;
}

QString ReaderPage::error() const
{
    return error_;
}

void ReaderPage::focusSearch()
{
    search_->setFocus();
    search_->selectAll();
}

void ReaderPage::applyFontSize(int points)
{
    reader_->setReaderFontSize(points);
}

void ReaderPage::applySearchLimit(int limit)
{
    reader_->setSearchLimit(limit);
}

void ReaderPage::applyLightMode(bool light)
{
    reader_->setLightMode(light);
}

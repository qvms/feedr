#include <QtGui>
#include <QDebug>
#include <QInputDialog>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QWidget>
#include "qmain.h"
#include "ui_qmain.h"

Q_LOGGING_CATEGORY(feedr_qmain, "feedr.qmain")

QMain::QMain(std::string File)
: fileLocation(std::move(File))
{
    resize(320, 240);

    centralWidget = new HtmlArea(this);
    setCentralWidget(centralWidget);

    centralWidget->getRandom();
    registerCommands();
    QObject::connect(centralWidget, &QTextBrowser::anchorClicked,
                     centralWidget, &HtmlArea::linking);
    QObject::connect(centralWidget, &QTextBrowser::sourceChanged,
                     centralWidget, &HtmlArea::sourceChange);

    if (qApp->overrideCursor() == nullptr) {
        qApp->setOverrideCursor(Qt::ArrowCursor);
        qCDebug(feedr_qmain) << "no cursor";
    }
}

QMain::~QMain() = default;

void QMain::searchDialog()
{
    bool ok = false;
    const QString text = QInputDialog::getText(this, tr("Search Article"),
                                               tr("Article Title"), QLineEdit::Normal,
                                               QString(), &ok);
    if (ok && !text.isEmpty()) {
        centralWidget->searchArticle(text);
    }
}

void QMain::registerCommands()
{
    randomArticleAct = new QAction(tr("Load &Random Article"), this);
    randomArticleAct->setShortcut(QKeySequence(tr("Ctrl+R")));
    randomArticleAct->setStatusTip(tr("Random Article"));
    connect(randomArticleAct, &QAction::triggered, centralWidget, &HtmlArea::getRandom);
    addAction(randomArticleAct);

    searchArticleAct = new QAction(tr("&Search Dialog"), this);
    searchArticleAct->setShortcut(QKeySequence(tr("Ctrl+S")));
    searchArticleAct->setStatusTip(tr("Search for Article"));
    connect(searchArticleAct, &QAction::triggered, this, &QMain::searchDialog);
    addAction(searchArticleAct);

    displayHelpAct = new QAction(tr("Display &Help"), this);
    displayHelpAct->setShortcut(QKeySequence(Qt::Key_F1));
    displayHelpAct->setStatusTip(tr("Display Help"));
    connect(displayHelpAct, &QAction::triggered, this, &QMain::displayHelp);
    addAction(displayHelpAct);

    goToTopAct = new QAction(tr("Go to &Top"), this);
    goToTopAct->setShortcut(QKeySequence(tr("Ctrl+T")));
    goToTopAct->setStatusTip(tr("Go to article top"));
    connect(goToTopAct, &QAction::triggered, centralWidget, &HtmlArea::goToTop);
    addAction(goToTopAct);

    quitAct = new QAction(tr("Quit feedr"), this);
    quitAct->setShortcut(QKeySequence(tr("Ctrl+Q")));
    quitAct->setStatusTip(tr("Quit"));
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    addAction(quitAct);
}

void QMain::displayHelp()
{
    QString txt = QStringLiteral("Usage: \n");
    txt.append(QStringLiteral("Ctrl + R = Display random article\n"));
    txt.append(QStringLiteral("Ctrl + S = Search for article\n"));
    txt.append(QStringLiteral("Ctrl + T = Go to articles top\n"));
    txt.append(QStringLiteral("Tab = Rotate through links\n"));
    txt.append(QStringLiteral("Enter = Activate link\n"));
    // 	txt.append("Ctrl + H = Display history\n");
    // 	txt.append("Ctrl + B = Go back in history\n");
    // 	txt.append("Ctrl + F = Go forward in history\n");
    txt.append(QStringLiteral("Ctrl + Q = Quit feedr\n"));
    txt.append(QStringLiteral("F1 = Display this help\n"));

    QMessageBox::information(this, QStringLiteral("Help"), txt, QMessageBox::Close);
}

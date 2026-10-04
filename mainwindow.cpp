#include "mainwindow.h"

#include "homepage.h"
#include "htmlarea.h"
#include "readerpage.h"
#include "settingsdialog.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPalette>
#include <QPushButton>
#include <QShortcut>
#include <QStyle>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

namespace {

bool isZim(const QString &path)
{
    return QFileInfo(path).suffix().compare(QStringLiteral("zim"), Qt::CaseInsensitive) == 0;
}

QStringList zimPathsFromUrls(const QList<QUrl> &urls)
{
    QStringList paths;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QString path = url.toLocalFile();
        if (isZim(path)) {
            paths.push_back(path);
        }
    }
    return paths;
}

} // namespace

MainWindow::MainWindow(Library *library, QWidget *parent)
    : QMainWindow(parent)
    , library_(library)
{
    setWindowTitle(QStringLiteral("feedr"));
    resize(1100, 720);
    setAcceptDrops(true);
    applyStyle();

    tabs_ = new QTabWidget(this);
    tabs_->setDocumentMode(true);
    tabs_->setMovable(true);
    tabs_->setTabsClosable(true);
    setCentralWidget(tabs_);

    home_ = new HomePage(library_, tabs_);
    tabs_->addTab(home_, QStringLiteral("Home"));
    tabs_->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
    tabs_->tabBar()->setTabButton(0, QTabBar::LeftSide, nullptr);

    connect(home_, &HomePage::openRequested, this, [this](const QString &path) { openArchive(path, true); });
    connect(home_, &HomePage::addRequested, this, &MainWindow::addFromDialog);
    connect(home_, &HomePage::browseRequested, this, &MainWindow::browseFiles);
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(tabs_, &QTabWidget::currentChanged, this, [this](int index) {
        if (index <= 0) {
            setWindowTitle(QStringLiteral("feedr"));
            statusBar()->showMessage(QStringLiteral("Library · %1 archives").arg(library_->archives().size()));
            return;
        }
        if (ReaderPage *page = readerAt(index)) {
            setWindowTitle(QStringLiteral("feedr — %1").arg(QFileInfo(page->path()).fileName()));
        }
    });

    buildExplorer();
    buildMenus();
    statusBar()->showMessage(QStringLiteral("Drag and drop .zim"));

    connect(library_, &Library::archivesChanged, this, &MainWindow::refreshLibraryList);
    connect(library_, &Library::recentsChanged, this, &MainWindow::refreshRecentMenu);
    connect(library_, &Library::settingsChanged, this, &MainWindow::applySettings);
    refreshLibraryList();
    refreshRecentMenu();

    if (!library_->geometry().isEmpty()) {
        restoreGeometry(library_->geometry());
    }
    if (!library_->windowState().isEmpty()) {
        restoreState(library_->windowState());
    }
    applySettings();
    if (QDockWidget *dock = findChild<QDockWidget *>(QStringLiteral("explorerDock"))) {
        dock->setVisible(library_->settings().showExplorer);
    }

    auto *closeTabShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+W")), this);
    connect(closeTabShortcut, &QShortcut::activated, this, [this]() { closeTab(tabs_->currentIndex()); });
    auto *nextTab = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Tab")), this);
    connect(nextTab, &QShortcut::activated, this, [this]() {
        tabs_->setCurrentIndex((tabs_->currentIndex() + 1) % tabs_->count());
    });
}

void MainWindow::applyStyle()
{
    const bool light = library_->settings().lightMode;
    qApp->setStyle(QStringLiteral("Fusion"));

    QPalette palette;
    if (light) {
        palette.setColor(QPalette::Window, QColor(QStringLiteral("#f3efe6")));
        palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#1c1915")));
        palette.setColor(QPalette::Base, QColor(QStringLiteral("#fffdf8")));
        palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#f7f4ee")));
        palette.setColor(QPalette::Text, QColor(QStringLiteral("#1c1915")));
        palette.setColor(QPalette::Button, QColor(QStringLiteral("#e7e1d6")));
        palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#1c1915")));
        palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#2f5d50")));
        palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
        palette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#fffdf8")));
        palette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#1c1915")));
    } else {
        palette.setColor(QPalette::Window, QColor(QStringLiteral("#141210")));
        palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#e8e2d6")));
        palette.setColor(QPalette::Base, QColor(QStringLiteral("#1c1a17")));
        palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#24211c")));
        palette.setColor(QPalette::Text, QColor(QStringLiteral("#e8e2d6")));
        palette.setColor(QPalette::Button, QColor(QStringLiteral("#2a2622")));
        palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#e8e2d6")));
        palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#3d7a64")));
        palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#f4f1ea")));
        palette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#24211c")));
        palette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#e8e2d6")));
        palette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#8c8478")));
    }
    qApp->setPalette(palette);

    const QString sheet = light
        ? QStringLiteral(
              "QMainWindow { background: #f3efe6; color: #1c1915; }"
              "QTabWidget::pane { border: 1px solid #d9d2c5; background: #fffdf8; }"
              "QTabBar::tab { background: #e7e1d6; color: #1c1915; padding: 8px 14px; margin-right: 2px; }"
              "QTabBar::tab:selected { background: #fffdf8; }"
              "QTextBrowser { background: #fffdf8; color: #1c1915; }"
              "QDockWidget, QLabel, QMenuBar, QMenu { color: #1c1915; }"
              "QListWidget, QTreeView, QLineEdit { background: #fffdf8; color: #1c1915; border: 1px solid #e0d9cc; }"
              "#homePage { background: #f7f4ee; }"
              "#homeTitle, #sectionHeading, #cardTitle { color: #1c1915; }"
              "#homeSubtitle, #emptyLabel, #cardMeta, #cardPath { color: #5e584e; }"
              "#archiveCard, #recentRow { background: #fffdf8; border: 1px solid #e0d9cc; border-radius: 8px; }"
              "#primaryButton { background: #2f5d50; color: white; padding: 6px 12px; border: none; border-radius: 4px; }"
              "QPushButton { padding: 6px 10px; }"
              "QStatusBar { background: #efeae1; color: #1c1915; }")
        : QStringLiteral(
              "QMainWindow, QDialog { background: #141210; color: #e8e2d6; }"
              "QTabWidget::pane { border: 1px solid #3a342c; background: #1c1a17; }"
              "QTabBar::tab { background: #24211c; color: #e8e2d6; padding: 8px 14px; margin-right: 2px; }"
              "QTabBar::tab:selected { background: #1c1a17; }"
              "QTextBrowser { background: #161410; color: #e6e0d4; }"
              "QDockWidget, QLabel, QMenuBar, QMenu, QCheckBox { color: #e8e2d6; background: #141210; }"
              "QListWidget, QTreeView, QLineEdit, QSpinBox { background: #1c1a17; color: #e8e2d6; border: 1px solid #3a342c; }"
              "#homePage { background: #141210; }"
              "#homeTitle, #sectionHeading, #cardTitle { color: #f4f1ea; }"
              "#homeSubtitle, #emptyLabel, #cardMeta, #cardPath { color: #a39b8c; }"
              "#archiveCard, #recentRow { background: #1c1a17; border: 1px solid #3a342c; border-radius: 8px; }"
              "#primaryButton { background: #3d7a64; color: #f4f1ea; padding: 6px 12px; border: none; border-radius: 4px; }"
              "QPushButton { padding: 6px 10px; background: #2a2622; color: #e8e2d6; border: 1px solid #3a342c; border-radius: 4px; }"
              "QStatusBar { background: #1c1a17; color: #e8e2d6; }");

    setStyleSheet(sheet + QStringLiteral(
        "#homeTitle { font-size: 32px; font-weight: 650; }"
        "#sectionHeading, #cardTitle { font-size: 16px; font-weight: 650; }"
        "QDockWidget { font-weight: 600; }"));

    if (tabs_ == nullptr) {
        return;
    }
    for (int i = 1; i < tabs_->count(); ++i) {
        if (ReaderPage *page = readerAt(i)) {
            page->applyLightMode(light);
        }
    }
}

void MainWindow::buildMenus()
{
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    auto *open = fileMenu->addAction(QStringLiteral("&Open…"));
    open->setShortcut(QKeySequence::Open);
    connect(open, &QAction::triggered, this, &MainWindow::browseFiles);

    auto *add = fileMenu->addAction(QStringLiteral("&Add to library…"));
    add->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    connect(add, &QAction::triggered, this, &MainWindow::addFromDialog);

    recentMenu_ = fileMenu->addMenu(QStringLiteral("Previous files"));
    fileMenu->addSeparator();
    auto *close = fileMenu->addAction(QStringLiteral("Close tab"));
    close->setShortcut(QKeySequence(QStringLiteral("Ctrl+W")));
    connect(close, &QAction::triggered, this, [this]() { closeTab(tabs_->currentIndex()); });
    auto *quit = fileMenu->addAction(QStringLiteral("&Quit"));
    quit->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));
    connect(quit, &QAction::triggered, this, &QWidget::close);

    auto *viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
    auto *home = viewMenu->addAction(QStringLiteral("&Home"));
    home->setShortcut(QKeySequence(QStringLiteral("Ctrl+H")));
    connect(home, &QAction::triggered, this, &MainWindow::showHome);
    auto *explorer = viewMenu->addAction(QStringLiteral("File &explorer"));
    explorer->setCheckable(true);
    explorer->setChecked(true);
    connect(explorer, &QAction::toggled, this, [this](bool on) {
        if (QDockWidget *dock = findChild<QDockWidget *>(QStringLiteral("explorerDock"))) {
            dock->setVisible(on);
        }
    });

    auto *goMenu = menuBar()->addMenu(QStringLiteral("&Go"));
    auto *random = goMenu->addAction(QStringLiteral("Random article"));
    random->setShortcut(QKeySequence(QStringLiteral("Ctrl+R")));
    connect(random, &QAction::triggered, this, [this]() {
        if (ReaderPage *page = currentReader()) {
            page->reader()->getRandom();
        }
    });
    auto *search = goMenu->addAction(QStringLiteral("Search"));
    search->setShortcut(QKeySequence(QStringLiteral("Ctrl+F")));
    search->setShortcuts({QKeySequence(QStringLiteral("Ctrl+F")), QKeySequence(QStringLiteral("Ctrl+S"))});
    connect(search, &QAction::triggered, this, [this]() {
        if (ReaderPage *page = currentReader()) {
            page->focusSearch();
        }
    });
    auto *top = goMenu->addAction(QStringLiteral("Article top"));
    top->setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
    connect(top, &QAction::triggered, this, [this]() {
        if (ReaderPage *page = currentReader()) {
            page->reader()->goToTop();
        }
    });
    auto *back = goMenu->addAction(QStringLiteral("Back"));
    back->setShortcut(QKeySequence::Back);
    connect(back, &QAction::triggered, this, [this]() {
        if (ReaderPage *page = currentReader()) {
            page->reader()->goBack();
        }
    });

    auto *settingsMenu = menuBar()->addMenu(QStringLiteral("&Settings"));
    auto *prefs = settingsMenu->addAction(QStringLiteral("Preferences…"));
    prefs->setShortcut(QKeySequence(QStringLiteral("Ctrl+,")));
    connect(prefs, &QAction::triggered, this, &MainWindow::openSettings);

    auto *helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    auto *help = helpMenu->addAction(QStringLiteral("Shortcuts"));
    help->setShortcut(QKeySequence(Qt::Key_F1));
    connect(help, &QAction::triggered, this, &MainWindow::showHelp);
}

void MainWindow::buildExplorer()
{
    auto *dock = new QDockWidget(QStringLiteral("Library and files"), this);
    dock->setObjectName(QStringLiteral("explorerDock"));
    auto *host = new QWidget(dock);
    auto *layout = new QVBoxLayout(host);
    layout->setContentsMargins(8, 8, 8, 8);

    auto *libraryLabel = new QLabel(QStringLiteral("Added archives"), host);
    libraryList_ = new QListWidget(host);
    libraryList_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(libraryList_, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        openArchive(item->data(Qt::UserRole).toString(), false);
    });

    auto *fileLabel = new QLabel(QStringLiteral("File explorer"), host);
    auto *rootRow = new QHBoxLayout();
    rootEdit_ = new QLineEdit(QDir::homePath(), host);
    auto *rootButton = new QPushButton(QStringLiteral("…"), host);
    rootButton->setFixedWidth(32);
    rootRow->addWidget(rootEdit_);
    rootRow->addWidget(rootButton);

    fileModel_ = new QFileSystemModel(this);
    fileModel_->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    fileModel_->setNameFilters({QStringLiteral("*.zim"), QStringLiteral("*.ZIM")});
    fileModel_->setNameFilterDisables(false);
    fileModel_->setRootPath(QDir::homePath());

    files_ = new QTreeView(host);
    files_->setModel(fileModel_);
    files_->setRootIndex(fileModel_->index(QDir::homePath()));
    files_->setHeaderHidden(true);
    files_->hideColumn(1);
    files_->hideColumn(2);
    files_->hideColumn(3);
    files_->setAnimated(true);
    connect(files_, &QTreeView::activated, this, [this](const QModelIndex &index) {
        const QString path = fileModel_->filePath(index);
        if (isZim(path)) {
            openArchive(path, true);
        }
    });

    auto setRoot = [this](const QString &path) {
        const QFileInfo info(path);
        if (!info.exists() || !info.isDir()) {
            statusBar()->showMessage(QStringLiteral("Not a directory: %1").arg(path));
            return;
        }
        const QString canon = info.canonicalFilePath();
        rootEdit_->setText(canon);
        fileModel_->setRootPath(canon);
        files_->setRootIndex(fileModel_->index(canon));
    };
    connect(rootButton, &QPushButton::clicked, this, [this, setRoot]() {
        const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Explorer root"), rootEdit_->text());
        if (!dir.isEmpty()) {
            setRoot(dir);
        }
    });
    connect(rootEdit_, &QLineEdit::returnPressed, this, [this, setRoot]() { setRoot(rootEdit_->text()); });

    layout->addWidget(libraryLabel);
    layout->addWidget(libraryList_, 1);
    layout->addWidget(fileLabel);
    layout->addLayout(rootRow);
    layout->addWidget(files_, 2);
    dock->setWidget(host);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void MainWindow::refreshLibraryList()
{
    libraryList_->clear();
    for (const ArchiveRecord &record : library_->archives()) {
        auto *item = new QListWidgetItem(record.title.isEmpty() ? QFileInfo(record.path).fileName() : record.title);
        item->setToolTip(record.path);
        item->setData(Qt::UserRole, record.path);
        libraryList_->addItem(item);
    }
}

void MainWindow::refreshRecentMenu()
{
    recentMenu_->clear();
    const QStringList recents = library_->recents();
    if (recents.isEmpty()) {
        recentMenu_->addAction(QStringLiteral("(empty)"))->setEnabled(false);
        return;
    }
    for (const QString &path : recents) {
        auto *action = recentMenu_->addAction(QFileInfo(path).fileName());
        action->setToolTip(path);
        action->setEnabled(QFileInfo::exists(path));
        connect(action, &QAction::triggered, this, [this, path]() { openArchive(path, true); });
    }
}

void MainWindow::showHome()
{
    tabs_->setCurrentIndex(0);
}

void MainWindow::applySettings()
{
    const AppSettings settings = library_->settings();
    applyStyle();
    if (QDockWidget *dock = findChild<QDockWidget *>(QStringLiteral("explorerDock"))) {
        if (!dock->isVisible() && settings.showExplorer) {
            dock->show();
        }
    }
    for (int i = 1; i < tabs_->count(); ++i) {
        if (ReaderPage *page = readerAt(i)) {
            page->applyFontSize(settings.fontSize);
            page->applySearchLimit(settings.searchLimit);
            page->applyLightMode(settings.lightMode);
        }
    }
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(library_->settings(), this);
    if (dialog.exec() == QDialog::Accepted) {
        library_->setSettings(dialog.settings());
    }
}

void MainWindow::addFromDialog()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, QStringLiteral("Add ZIM archives"), rootEdit_->text(), QStringLiteral("ZIM archives (*.zim)"));
    for (const QString &path : paths) {
        QString error;
        if (!library_->addArchive(path, &error)) {
            QMessageBox::warning(this, QStringLiteral("Add archive"), error);
        }
    }
}

void MainWindow::browseFiles()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, QStringLiteral("Open ZIM archives"), rootEdit_->text(), QStringLiteral("ZIM archives (*.zim)"));
    openPaths(paths);
}

ReaderPage *MainWindow::currentReader() const
{
    return readerAt(tabs_->currentIndex());
}

ReaderPage *MainWindow::readerAt(int index) const
{
    return qobject_cast<ReaderPage *>(tabs_->widget(index));
}

int MainWindow::tabForPath(const QString &path) const
{
    const QString key = QFileInfo(path).canonicalFilePath();
    for (int i = 1; i < tabs_->count(); ++i) {
        if (ReaderPage *page = readerAt(i)) {
            if (QFileInfo(page->path()).canonicalFilePath() == key) {
                return i;
            }
        }
    }
    return -1;
}

void MainWindow::openArchive(const QString &path, bool addToLibrary)
{
    if (!isZim(path)) {
        QMessageBox::warning(this, QStringLiteral("Open"), QStringLiteral("Not a .zim file: %1").arg(path));
        return;
    }
    const int existing = tabForPath(path);
    if (existing >= 0) {
        tabs_->setCurrentIndex(existing);
        library_->touch(path);
        return;
    }

    if (addToLibrary) {
        QString error;
        if (!library_->addArchive(path, &error)) {
            QMessageBox::warning(this, QStringLiteral("Open"), error);
            return;
        }
    } else {
        library_->touch(path);
    }

    const AppSettings settings = library_->settings();
    const QString canon = QFileInfo(path).canonicalFilePath();
    auto *page = new ReaderPage(canon.isEmpty() ? path : canon, settings.fontSize, settings.searchLimit, tabs_);
    page->applyLightMode(settings.lightMode);
    if (!page->ok()) {
        QMessageBox::warning(this, QStringLiteral("Open"), page->error());
        page->deleteLater();
        return;
    }
    const QString title = page->reader()->archiveTitle();
    const int index = tabs_->addTab(page, title.isEmpty() ? QFileInfo(path).completeBaseName() : title);
    tabs_->setTabToolTip(index, page->path());
    tabs_->setCurrentIndex(index);
    connect(page, &ReaderPage::statusMessage, this, [this](const QString &message) {
        statusBar()->showMessage(message, 8000);
    });
    statusBar()->showMessage(QStringLiteral("Opened %1").arg(page->path()), 5000);
}

void MainWindow::openPaths(const QStringList &paths)
{
    for (const QString &path : paths) {
        openArchive(path, true);
    }
}

void MainWindow::closeTab(int index)
{
    if (index <= 0 || index >= tabs_->count()) {
        return;
    }
    QWidget *page = tabs_->widget(index);
    tabs_->removeTab(index);
    page->deleteLater();
}

void MainWindow::saveSession()
{
    QStringList openPaths;
    for (int i = 1; i < tabs_->count(); ++i) {
        if (ReaderPage *page = readerAt(i)) {
            openPaths.push_back(page->path());
        }
    }
    library_->saveSession(openPaths, saveGeometry(), saveState());
}

void MainWindow::showHelp()
{
    QMessageBox::information(
        this,
        QStringLiteral("feedr"),
        QStringLiteral("Ctrl+O  Open archive in a new tab\n"
                       "Ctrl+Shift+O  Add to library without opening\n"
                       "Ctrl+H  Home, with added archives and previous files\n"
                       "Ctrl+W  Close the current tab\n"
                       "Ctrl+Tab  Next tab\n"
                       "Ctrl+R  Random article\n"
                       "Ctrl+F  Search the current archive\n"
                       "Ctrl+T  Article top\n"
                       "Alt+Left  Back\n"
                       "Ctrl+,  Settings\n"
                       "Ctrl+Q  Quit\n"
                       "F1  Help\n\n"
                       "Drop .zim files on the window to open them in tabs.\n"
                       "The file explorer lists directories and .zim files; double-click a file to open it."));
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    openPaths(zimPathsFromUrls(event->mimeData()->urls()));
    event->acceptProposedAction();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSession();
    QMainWindow::closeEvent(event);
}

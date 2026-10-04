#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "library.h"

#include <QMainWindow>

class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class HomePage;
class QFileSystemModel;
class QLineEdit;
class QListWidget;
class QTreeView;
class QTabWidget;
class ReaderPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Library *library, QWidget *parent = nullptr);

    void openArchive(const QString &path, bool addToLibrary = true);
    void openPaths(const QStringList &paths);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void buildMenus();
    void buildExplorer();
    void applyStyle();
    void refreshLibraryList();
    void refreshRecentMenu();
    void showHome();
    void applySettings();
    void openSettings();
    void addFromDialog();
    void browseFiles();
    ReaderPage *currentReader() const;
    ReaderPage *readerAt(int index) const;
    int tabForPath(const QString &path) const;
    void closeTab(int index);
    void saveSession();
    void showHelp();

    Library *library_ = nullptr;
    QTabWidget *tabs_ = nullptr;
    HomePage *home_ = nullptr;
    QListWidget *libraryList_ = nullptr;
    QTreeView *files_ = nullptr;
    QFileSystemModel *fileModel_ = nullptr;
    QLineEdit *rootEdit_ = nullptr;
    QMenu *recentMenu_ = nullptr;
};

#endif

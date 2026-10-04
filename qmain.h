#ifndef QMAIN_H
#define QMAIN_H

#include <QMainWindow>
#include <string>
#include "htmlarea.h"


class QMain : public QMainWindow
{
    Q_OBJECT

public:
    QMain(std::string);
    std::string fileLocation;
    ~QMain();

private:
    void registerCommands();
    HtmlArea *centralWidget;
    QAction *randomArticleAct;
    QAction *searchArticleAct;
    QAction *displayHelpAct;
    QAction *quitAct;
    QAction *goToTopAct;

private Q_SLOTS:
    void displayHelp();
    void searchDialog();
};

#endif

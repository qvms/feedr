#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include "library.h"

#include <QWidget>

class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QVBoxLayout;

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(Library *library, QWidget *parent = nullptr);

signals:
    void openRequested(const QString &path);
    void addRequested();
    void browseRequested();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void rebuild();
    QWidget *makeCard(const ArchiveRecord &record);
    QWidget *makeRecentRow(const QString &path);

    Library *library_ = nullptr;
    QVBoxLayout *archiveLayout_ = nullptr;
    QVBoxLayout *recentLayout_ = nullptr;
    QLabel *emptyLabel_ = nullptr;
};

#endif

#ifndef READERPAGE_H
#define READERPAGE_H

#include <QWidget>

class HtmlArea;
class QLineEdit;

class ReaderPage : public QWidget
{
    Q_OBJECT

public:
    explicit ReaderPage(const QString &path, int fontSize, int searchLimit, QWidget *parent = nullptr);

    HtmlArea *reader() const;
    QString path() const;
    bool ok() const;
    QString error() const;

    void focusSearch();
    void applyFontSize(int points);
    void applySearchLimit(int limit);
    void applyLightMode(bool light);

signals:
    void statusMessage(const QString &message);

private:
    HtmlArea *reader_ = nullptr;
    QLineEdit *search_ = nullptr;
    QString error_;
    bool ok_ = false;
};

#endif

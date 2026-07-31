#pragma once
#include <QWidget>
#include <QLabel>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QScrollArea>
#include <QToolButton>

class PreviewPanel : public QWidget {
    Q_OBJECT
public:
    explicit PreviewPanel(QWidget *parent = nullptr);

    void previewFile(const QString &path);
    void previewFolderGraph(const QString &folderPath);
    void clearPreview();
    QString currentPath() const { return m_path; }
    bool isEmpty() const { return m_path.isEmpty(); }

signals:
    void closeRequested();

private:
    void showEmpty();
    void showText(const QString &path);
    void showImage(const QString &path);
    void showBinaryStub(const QString &path, const QString &reason);
    bool loadTextFile(const QString &path, QString *out, QString *error);

    QString m_path;
    QLabel *m_title = nullptr;
    QToolButton *m_closeBtn = nullptr;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_emptyPage = nullptr;
    QPlainTextEdit *m_textView = nullptr;
    QLabel *m_imageLabel = nullptr;
    QScrollArea *m_imageScroll = nullptr;
    QLabel *m_stubLabel = nullptr;
};

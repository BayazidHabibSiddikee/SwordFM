#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QStyle>

class ToolBar : public QWidget {
    Q_OBJECT
public:
    explicit ToolBar(QWidget *parent = nullptr);

    void setPath(const QString &path);
    void focusPath();
    void setCanGoBack(bool on);
    void setCanGoForward(bool on);
    void setDetailsMode(bool details);

signals:
    void pathEntered(const QString &path);
    void goBack();
    void goForward();
    void goUp();
    void goHome();
    void refreshRequested();
    void searchQuery(const QString &query);
    void viewModeToggled();

private:
    QToolButton *makeNavButton(const QString &themeIcon, QStyle::StandardPixmap fallback,
                               const QString &tip);

    QLineEdit *m_pathEdit;
    QLineEdit *m_searchEdit;
    QToolButton *m_backBtn;
    QToolButton *m_forwardBtn;
    QToolButton *m_upBtn;
    QToolButton *m_homeBtn;
    QToolButton *m_refreshBtn;
    QToolButton *m_viewBtn;
};

#include "toolbar.h"
#include "theme.h"

#include <QIcon>
#include <QApplication>
#include <QStyle>

ToolBar::ToolBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(42);
    setStyleSheet(QString(
        "ToolBar { background: %1; border-bottom: 1px solid %2; }"
        "QToolButton {"
        "  background: transparent; border: none; border-radius: 4px;"
        "  padding: 4px; margin: 1px; color: %3;"
        "}"
        "QToolButton:hover { background: %2; }"
        "QToolButton:pressed { background: %4; }"
        "QToolButton:disabled { color: %5; }"
        "QLineEdit {"
        "  background: %4; color: %3; border: 1px solid %2;"
        "  border-radius: 4px; padding: 5px 10px; font-size: 13px;"
        "  selection-background-color: %2; selection-color: %6;"
        "}"
        "QLineEdit:focus { border-color: %6; }"
    ).arg(Theme::BG2, Theme::DIM, Theme::FG, Theme::BG, Theme::FG_DIM, Theme::CYAN));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(2);

    m_backBtn = makeNavButton("go-previous", QStyle::SP_ArrowBack, "Back (Alt+Left)");
    m_forwardBtn = makeNavButton("go-next", QStyle::SP_ArrowForward, "Forward (Alt+Right)");
    m_upBtn = makeNavButton("go-up", QStyle::SP_ArrowUp, "Up (Alt+Up)");
    m_homeBtn = makeNavButton("go-home", QStyle::SP_DirHomeIcon, "Home");
    m_refreshBtn = makeNavButton("view-refresh", QStyle::SP_BrowserReload, "Refresh (F5)");

    connect(m_backBtn, &QToolButton::clicked, this, &ToolBar::goBack);
    connect(m_forwardBtn, &QToolButton::clicked, this, &ToolBar::goForward);
    connect(m_upBtn, &QToolButton::clicked, this, &ToolBar::goUp);
    connect(m_homeBtn, &QToolButton::clicked, this, &ToolBar::goHome);
    connect(m_refreshBtn, &QToolButton::clicked, this, &ToolBar::refreshRequested);

    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText("Location… (Ctrl+L)");
    m_pathEdit->setClearButtonEnabled(true);
    connect(m_pathEdit, &QLineEdit::returnPressed, this, [this]() {
        emit pathEntered(m_pathEdit->text().trimmed());
    });

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search…");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(180);
    m_searchEdit->setMinimumWidth(120);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ToolBar::searchQuery);

    m_viewBtn = makeNavButton("view-list-details", QStyle::SP_FileDialogDetailedView,
                              "Toggle Icon / Details view");
    connect(m_viewBtn, &QToolButton::clicked, this, &ToolBar::viewModeToggled);

    layout->addWidget(m_backBtn);
    layout->addWidget(m_forwardBtn);
    layout->addWidget(m_upBtn);
    layout->addWidget(m_homeBtn);
    layout->addWidget(m_refreshBtn);
    layout->addSpacing(6);
    layout->addWidget(m_pathEdit, 1);
    layout->addSpacing(4);
    layout->addWidget(m_searchEdit);
    layout->addWidget(m_viewBtn);

    setCanGoBack(false);
    setCanGoForward(false);
}

QToolButton *ToolBar::makeNavButton(const QString &themeIcon, QStyle::StandardPixmap fallback,
                                    const QString &tip) {
    auto *btn = new QToolButton(this);
    QIcon icon = QIcon::fromTheme(themeIcon);
    if (icon.isNull())
        icon = QApplication::style()->standardIcon(fallback);
    btn->setIcon(icon);
    btn->setIconSize(QSize(20, 20));
    btn->setToolTip(tip);
    btn->setAutoRaise(true);
    btn->setFixedSize(32, 32);
    return btn;
}

void ToolBar::setPath(const QString &path) {
    if (m_pathEdit->text() != path)
        m_pathEdit->setText(path);
}

void ToolBar::focusPath() {
    m_pathEdit->setFocus();
    m_pathEdit->selectAll();
}

void ToolBar::setCanGoBack(bool on) {
    m_backBtn->setEnabled(on);
}

void ToolBar::setCanGoForward(bool on) {
    m_forwardBtn->setEnabled(on);
}

void ToolBar::setDetailsMode(bool details) {
    QIcon icon = QIcon::fromTheme(details ? "view-grid-symbolic" : "view-list-details");
    if (icon.isNull()) {
        icon = QApplication::style()->standardIcon(
            details ? QStyle::SP_FileDialogListView : QStyle::SP_FileDialogDetailedView);
    }
    m_viewBtn->setIcon(icon);
}

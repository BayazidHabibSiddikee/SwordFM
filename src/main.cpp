#include "mainwindow.h"
#include "theme.h"
#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QPalette>
#include <QColor>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SwordFM");
    app.setApplicationDisplayName("SwordFM");
    app.setOrganizationName("swordfm");
    app.setDesktopFileName("swordfm");
    app.setApplicationVersion("1.0");

    if (QStyleFactory::keys().contains("Fusion", Qt::CaseInsensitive))
        app.setStyle(QStyleFactory::create("Fusion"));

    // One Dark palette (matches sworddeck)
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(Theme::BG));
    pal.setColor(QPalette::WindowText, QColor(Theme::FG));
    pal.setColor(QPalette::Base, QColor(Theme::BG));
    pal.setColor(QPalette::AlternateBase, QColor(Theme::BG2));
    pal.setColor(QPalette::Text, QColor(Theme::FG));
    pal.setColor(QPalette::Button, QColor(Theme::DIM));
    pal.setColor(QPalette::ButtonText, QColor(Theme::FG));
    pal.setColor(QPalette::Highlight, QColor(Theme::DIM));
    pal.setColor(QPalette::HighlightedText, QColor(Theme::CYAN));
    pal.setColor(QPalette::ToolTipBase, QColor(Theme::BG2));
    pal.setColor(QPalette::ToolTipText, QColor(Theme::FG));
    pal.setColor(QPalette::PlaceholderText, QColor(Theme::FG_DIM));
    pal.setColor(QPalette::Link, QColor(Theme::CYAN));
    pal.setColor(QPalette::BrightText, QColor(Theme::RED));
    app.setPalette(pal);
    app.setStyleSheet(Theme::appStylesheet());

    QString startPath;
    if (argc > 1)
        startPath = QDir::fromNativeSeparators(QString::fromLocal8Bit(argv[1]));

    MainWindow w(startPath);
    w.show();
    return app.exec();
}

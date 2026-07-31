#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QStack>
#include <QKeyEvent>

class ToolBar;
class SideBar;
class FileView;
class StatusBar;
class PreviewPanel;
class QFileSystemModel;
class QModelIndex;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString &startPath = QString(), QWidget *parent = nullptr);

public slots:
    void navigateTo(const QString &path);
    void navigateUp();
    void navigateBack();
    void navigateForward();
    void navigateHome();
    void refresh();
    void openFile(const QString &path);
    void previewSelected();
    void selectAll();
    void copySelection();
    void cutSelection();
    void pasteClipboard();
    void deleteSelection();
    void renameSelected();
    void createNewFolder();
    void createNewFile();
    void toggleHidden();
    void toggleViewMode();
    void search(const QString &query);
    void bookmarkCurrent();
    void bookmarkSelection();
    void openTerminalHere();
    void selectNext();
    void selectPrev();
    void openSelected();

private slots:
    void onFileActivated(const QModelIndex &index);
    void onSelectionChanged();

private:
    void setupUi();
    void setupMenus();
    void updateStatusBar();
    void updateNavButtons();
    void applyDirectory(const QString &path, bool pushHistory);
    void setClipboard(const QStringList &paths, bool cut);
    void showPreview(const QString &path);
    int currentItemCount() const;

    ToolBar *m_toolbar = nullptr;
    SideBar *m_sidebar = nullptr;
    FileView *m_fileView = nullptr;
    PreviewPanel *m_preview = nullptr;
    StatusBar *m_statusbar = nullptr;
    QSplitter *m_splitter = nullptr;

    QString m_currentPath;
    QStack<QString> m_backStack;
    QStack<QString> m_forwardStack;
    QFileSystemModel *m_fsModel = nullptr;

    QStringList m_clipboard;
    bool m_clipboardIsCut = false;
    bool m_showHidden = false;
};

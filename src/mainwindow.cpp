#include "mainwindow.h"
#include "toolbar.h"
#include "sidebar.h"
#include "fileview.h"
#include "statusbar.h"
#include "previewpanel.h"
#include "fileops.h"
#include "contextmenu.h"
#include "termutil.h"
#include "theme.h"
#include "openwith.h"

#include <QVBoxLayout>
#include <QFileSystemModel>
#include <QDesktopServices>
#include <QMessageBox>
#include <QInputDialog>
#include <QKeySequence>
#include <QShortcut>
#include <QDir>
#include <QUrl>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QStyle>
#include <QIcon>
#include <QFileInfo>
#include <QLineEdit>
#include <QFile>

static bool isHiddenSystemRoot(const QString &path) {
    const QString abs = QDir(path).absolutePath();
    return abs == "/" || abs == "/home";
}

MainWindow::MainWindow(const QString &startPath, QWidget *parent)
    : QMainWindow(parent),
      m_currentPath(startPath.isEmpty() ? QDir::homePath() : QFileInfo(startPath).absoluteFilePath())
{
    setWindowTitle("SwordFM");
    setWindowIcon(QIcon::fromTheme("system-file-manager",
                                   style()->standardIcon(QStyle::SP_DirIcon)));
    setMinimumSize(800, 500);
    resize(1100, 700);

    m_fsModel = new QFileSystemModel(this);
    m_fsModel->setRootPath(QStringLiteral("/"));
    m_fsModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    m_fsModel->setResolveSymlinks(true);
    m_fsModel->setReadOnly(false);
    m_fsModel->setNameFilterDisables(false);

    setupUi();
    setupMenus();

    // Navigation
    new QShortcut(QKeySequence("Ctrl+L"), this, [this]() { m_toolbar->focusPath(); });
    new QShortcut(QKeySequence("Alt+Left"), this, [this]() { navigateBack(); });
    new QShortcut(QKeySequence("Alt+Right"), this, [this]() { navigateForward(); });
    new QShortcut(QKeySequence("Alt+Up"), this, [this]() { navigateUp(); });
    new QShortcut(QKeySequence("Backspace"), this, [this]() { navigateUp(); });
    new QShortcut(QKeySequence("F5"), this, [this]() { refresh(); });
    new QShortcut(QKeySequence("Ctrl+H"), this, [this]() { toggleHidden(); });
    new QShortcut(QKeySequence("Ctrl+1"), this, [this]() {
        if (!m_fileView->isDetailsMode()) toggleViewMode();
    });
    new QShortcut(QKeySequence("Ctrl+2"), this, [this]() {
        if (m_fileView->isDetailsMode()) toggleViewMode();
    });

    // Edit
    new QShortcut(QKeySequence::SelectAll, this, [this]() { selectAll(); });
    new QShortcut(QKeySequence::Copy, this, [this]() { copySelection(); });
    new QShortcut(QKeySequence::Cut, this, [this]() { cutSelection(); });
    new QShortcut(QKeySequence::Paste, this, [this]() { pasteClipboard(); });
    new QShortcut(QKeySequence::Delete, this, [this]() { deleteSelection(); });
    new QShortcut(QKeySequence("F2"), this, [this]() { renameSelected(); });
    new QShortcut(QKeySequence("Ctrl+Shift+N"), this, [this]() { createNewFolder(); });
    new QShortcut(QKeySequence("Ctrl+N"), this, [this]() { createNewFolder(); });
    new QShortcut(QKeySequence("Space"), this, [this]() { previewSelected(); });
    new QShortcut(QKeySequence("F3"), this, [this]() { previewSelected(); });

    if (isHiddenSystemRoot(m_currentPath))
        m_currentPath = QDir::homePath();
    applyDirectory(m_currentPath, false);
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_toolbar = new ToolBar(this);
    layout->addWidget(m_toolbar);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_sidebar = new SideBar(this);
    m_fileView = new FileView(m_fsModel, this);
    m_preview = new PreviewPanel(this);

    m_splitter->addWidget(m_sidebar);
    m_splitter->addWidget(m_fileView);
    m_splitter->addWidget(m_preview);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 1);
    m_splitter->setSizes({200, 520, 380});
    m_splitter->setCollapsible(0, false);
    m_splitter->setCollapsible(2, true);

    layout->addWidget(m_splitter, 1);

    m_statusbar = new StatusBar(this);
    layout->addWidget(m_statusbar);

    setCentralWidget(central);

    connect(m_toolbar, &ToolBar::pathEntered, this, &MainWindow::navigateTo);
    connect(m_toolbar, &ToolBar::goBack, this, &MainWindow::navigateBack);
    connect(m_toolbar, &ToolBar::goForward, this, &MainWindow::navigateForward);
    connect(m_toolbar, &ToolBar::goUp, this, &MainWindow::navigateUp);
    connect(m_toolbar, &ToolBar::goHome, this, &MainWindow::navigateHome);
    connect(m_toolbar, &ToolBar::refreshRequested, this, &MainWindow::refresh);
    connect(m_toolbar, &ToolBar::searchQuery, this, &MainWindow::search);
    connect(m_toolbar, &ToolBar::viewModeToggled, this, &MainWindow::toggleViewMode);

    connect(m_sidebar, &SideBar::pathSelected, this, &MainWindow::navigateTo);

    connect(m_fileView, &FileView::fileActivated, this, &MainWindow::onFileActivated);
    connect(m_fileView, &FileView::selectionChanged, this, &MainWindow::onSelectionChanged);
    connect(m_fileView, &FileView::contextMenuRequested, this, [this](const QPoint &globalPos) {
        showContextMenu(this, globalPos, m_fileView->selectedPaths(), m_currentPath);
    });
    connect(m_fsModel, &QFileSystemModel::directoryLoaded, this, [this](const QString &path) {
        if (path == m_currentPath || QFileInfo(path).absoluteFilePath() == m_currentPath)
            updateStatusBar();
    });
}

void MainWindow::setupMenus() {
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(QIcon::fromTheme("folder-new"), "New &Folder…",
                        QKeySequence("Ctrl+Shift+N"), this, &MainWindow::createNewFolder);
    fileMenu->addAction(QIcon::fromTheme("document-new"), "New F&ile…",
                        this, &MainWindow::createNewFile);
    fileMenu->addSeparator();
    fileMenu->addAction(QIcon::fromTheme("document-open"), "&Open",
                        QKeySequence::Open, this, [this]() {
        auto paths = m_fileView->selectedPaths();
        if (paths.isEmpty()) return;
        openFile(paths.first());
    });
    fileMenu->addSeparator();
    fileMenu->addAction(QIcon::fromTheme("application-exit"), "&Close",
                        QKeySequence::Close, this, &QWidget::close);

    auto *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction(QIcon::fromTheme("edit-cut"), "Cu&t",
                        QKeySequence::Cut, this, &MainWindow::cutSelection);
    editMenu->addAction(QIcon::fromTheme("edit-copy"), "&Copy",
                        QKeySequence::Copy, this, &MainWindow::copySelection);
    editMenu->addAction(QIcon::fromTheme("edit-paste"), "&Paste",
                        QKeySequence::Paste, this, &MainWindow::pasteClipboard);
    editMenu->addSeparator();
    editMenu->addAction(QIcon::fromTheme("edit-select-all"), "Select &All",
                        QKeySequence::SelectAll, this, &MainWindow::selectAll);
    editMenu->addSeparator();
    editMenu->addAction(QIcon::fromTheme("edit-rename"), "&Rename…",
                        QKeySequence("F2"), this, &MainWindow::renameSelected);
    editMenu->addAction(QIcon::fromTheme("edit-delete"), "&Delete",
                        QKeySequence::Delete, this, &MainWindow::deleteSelection);

    auto *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Details View", QKeySequence("Ctrl+1"), this, [this]() {
        if (!m_fileView->isDetailsMode()) toggleViewMode();
    });
    viewMenu->addAction("Icon View", QKeySequence("Ctrl+2"), this, [this]() {
        if (m_fileView->isDetailsMode()) toggleViewMode();
    });
    viewMenu->addSeparator();
    viewMenu->addAction("Show &Hidden Files", QKeySequence("Ctrl+H"),
                        this, &MainWindow::toggleHidden);
    viewMenu->addAction("&Preview Panel", QKeySequence("F3"),
                        this, &MainWindow::previewSelected);
    viewMenu->addAction(QIcon::fromTheme("view-refresh"), "&Refresh",
                        QKeySequence::Refresh, this, &MainWindow::refresh);

    auto *goMenu = menuBar()->addMenu("&Go");
    goMenu->addAction(QIcon::fromTheme("go-previous"), "&Back",
                      QKeySequence("Alt+Left"), this, &MainWindow::navigateBack);
    goMenu->addAction(QIcon::fromTheme("go-next"), "&Forward",
                      QKeySequence("Alt+Right"), this, &MainWindow::navigateForward);
    goMenu->addAction(QIcon::fromTheme("go-up"), "&Up",
                      QKeySequence("Alt+Up"), this, &MainWindow::navigateUp);
    goMenu->addAction(QIcon::fromTheme("go-home"), "&Home",
                      QKeySequence("Alt+Home"), this, &MainWindow::navigateHome);
    goMenu->addSeparator();
    goMenu->addAction(QIcon::fromTheme("document-preview"), "&Preview",
                      QKeySequence("F3"), this, &MainWindow::previewSelected);
    goMenu->addAction(QIcon::fromTheme("utilities-terminal"), "Open &Terminal Here",
                      QKeySequence("F4"), this, &MainWindow::openTerminalHere);
    goMenu->addAction(QIcon::fromTheme("bookmark-new"), "Bookmark This Folder",
                      this, &MainWindow::bookmarkCurrent);
}

void MainWindow::applyDirectory(const QString &path, bool pushHistory) {
    QFileInfo fi(path);
    QString target = fi.exists()
                         ? (fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath())
                         : QDir::homePath();

    if (!QDir(target).exists() || isHiddenSystemRoot(target))
        target = QDir::homePath();

    if (pushHistory && target != m_currentPath) {
        m_backStack.push(m_currentPath);
        m_forwardStack.clear();
    }

    m_currentPath = target;

    QModelIndex idx = m_fsModel->index(target);
    // Ensure model has loaded this path
    m_fsModel->setRootPath(target);
    idx = m_fsModel->index(target);

    m_fileView->setRootIndex(idx);
    m_toolbar->setPath(target);
    m_sidebar->setCurrentPath(target);
    if (m_preview)
        m_preview->clearPreview();
    updateNavButtons();
    updateStatusBar();

    QString name = QFileInfo(target).fileName();
    if (name.isEmpty()) name = target;
    setWindowTitle(QString("%1 — SwordFM").arg(name));
}

void MainWindow::navigateTo(const QString &path) {
    QString expanded = path;
    if (expanded.startsWith("~/"))
        expanded = QDir::homePath() + expanded.mid(1);
    else if (expanded == "~")
        expanded = QDir::homePath();

    QFileInfo fi(expanded);
    if (!fi.exists()) {
        QMessageBox::warning(this, "SwordFM",
                             QString("Path does not exist:\n%1").arg(expanded));
        m_toolbar->setPath(m_currentPath);
        return;
    }
    if (fi.isDir()) {
        if (isHiddenSystemRoot(fi.absoluteFilePath())) {
            m_toolbar->setPath(m_currentPath);
            return;
        }
        applyDirectory(fi.absoluteFilePath(), true);
    } else {
        openFile(fi.absoluteFilePath());
    }
}

void MainWindow::navigateUp() {
    QDir dir(m_currentPath);
    if (!dir.cdUp())
        return;
    if (isHiddenSystemRoot(dir.absolutePath()))
        return; // stay inside user space — no / or /home
    applyDirectory(dir.absolutePath(), true);
}

void MainWindow::navigateBack() {
    if (m_backStack.isEmpty()) return;
    m_forwardStack.push(m_currentPath);
    applyDirectory(m_backStack.pop(), false);
}

void MainWindow::navigateForward() {
    if (m_forwardStack.isEmpty()) return;
    m_backStack.push(m_currentPath);
    applyDirectory(m_forwardStack.pop(), false);
}

void MainWindow::navigateHome() {
    applyDirectory(QDir::homePath(), true);
}

void MainWindow::refresh() {
    // Force model refresh
    QString path = m_currentPath;
    m_fsModel->setRootPath(QString());
    m_fsModel->setRootPath(path);
    m_fileView->setRootIndex(m_fsModel->index(path));
    updateStatusBar();
}

void MainWindow::openFile(const QString &path) {
    QFileInfo fi(path);
    if (fi.isDir()) {
        if (isHiddenSystemRoot(fi.absoluteFilePath()))
            return;
        applyDirectory(path, true);
        return;
    }
    // Always use XDG/desktop handlers (Thunar-style), especially for video
    if (!openWithDefault(path)) {
        QMessageBox::warning(this, "SwordFM",
                             QString("No application found to open:\n%1").arg(fi.fileName()));
    }
}

void MainWindow::showPreview(const QString &path) {
    if (!m_preview->isVisible())
        m_preview->show();
    m_preview->previewFile(path);
    QList<int> sizes = m_splitter->sizes();
    if (sizes.size() >= 3 && sizes[2] < 200) {
        int take = 320 - sizes[2];
        if (sizes[1] > take + 200)
            sizes[1] -= take;
        sizes[2] = 320;
        m_splitter->setSizes(sizes);
    }
}

void MainWindow::previewSelected() {
    auto paths = m_fileView->selectedPaths();
    if (paths.isEmpty()) {
        m_preview->clearPreview();
        return;
    }
    QFileInfo fi(paths.first());
    if (fi.isDir())
        return;
    showPreview(paths.first());
}

void MainWindow::openTerminalHere() {
    auto paths = m_fileView->selectedPaths();
    openTerminalAt(paths.isEmpty() ? m_currentPath : paths.first());
}

void MainWindow::bookmarkSelection() {
    auto paths = m_fileView->selectedPaths();
    if (!paths.isEmpty())
        m_sidebar->addBookmark(paths.first());
    else
        m_sidebar->addCurrentAsBookmark();
}

void MainWindow::selectAll() {
    m_fileView->selectAll();
}

void MainWindow::setClipboard(const QStringList &paths, bool cut) {
    m_clipboard = paths;
    m_clipboardIsCut = cut;

    auto *mime = new QMimeData();
    QList<QUrl> urls;
    QStringList uriLines;
    for (const auto &p : paths) {
        QUrl url = QUrl::fromLocalFile(p);
        urls.append(url);
        uriLines.append(url.toString());
    }
    mime->setUrls(urls);
    mime->setText(paths.join('\n'));
    QByteArray gnome = (cut ? "cut\n" : "copy\n") + uriLines.join('\n').toUtf8();
    mime->setData("x-special/gnome-copied-files", gnome);
    QApplication::clipboard()->setMimeData(mime);
    updateStatusBar();
}

void MainWindow::copySelection() {
    auto paths = m_fileView->selectedPaths();
    if (!paths.isEmpty())
        setClipboard(paths, false);
}

void MainWindow::cutSelection() {
    auto paths = m_fileView->selectedPaths();
    if (!paths.isEmpty())
        setClipboard(paths, true);
}

void MainWindow::pasteClipboard() {
    // Prefer internal clipboard; fall back to system file URLs
    QStringList sources = m_clipboard;
    bool cut = m_clipboardIsCut;

    if (sources.isEmpty()) {
        const QMimeData *mime = QApplication::clipboard()->mimeData();
        if (mime && mime->hasUrls()) {
            for (const QUrl &u : mime->urls()) {
                if (u.isLocalFile())
                    sources.append(u.toLocalFile());
            }
            QByteArray gnome = mime->data("x-special/gnome-copied-files");
            cut = gnome.startsWith("cut");
        }
    }

    if (sources.isEmpty()) return;

    bool ok = cut ? moveFiles(sources, m_currentPath)
                  : copyFiles(sources, m_currentPath);
    if (!ok) {
        QMessageBox::warning(this, "SwordFM",
                             cut ? "Some items could not be moved."
                                 : "Some items could not be copied.");
    }
    if (cut && ok) {
        m_clipboard.clear();
        m_clipboardIsCut = false;
    }
    refresh();
}

void MainWindow::deleteSelection() {
    QStringList paths = m_fileView->selectedPaths();
    if (paths.isEmpty()) return;

    QString msg;
    if (paths.size() == 1) {
        msg = QString("Delete \"%1\"?").arg(QFileInfo(paths[0]).fileName());
    } else {
        msg = QString("Delete %1 items?").arg(paths.size());
    }

    auto btn = QMessageBox::question(this, "Delete", msg,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    if (btn != QMessageBox::Yes) return;

    for (const auto &p : paths)
        deleteFileOrDir(p);
    refresh();
}

void MainWindow::renameSelected() {
    QStringList paths = m_fileView->selectedPaths();
    if (paths.size() != 1) return;

    QString oldPath = paths[0];
    QFileInfo fi(oldPath);

    bool ok = false;
    QString newName = QInputDialog::getText(this, "Rename", "New name:",
                                            QLineEdit::Normal, fi.fileName(), &ok);
    if (!ok || newName.isEmpty() || newName == fi.fileName()) return;

    QString newPath = fi.absolutePath() + "/" + newName;
    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, "Rename", "A file with that name already exists.");
        return;
    }
    if (!QFile::rename(oldPath, newPath))
        QMessageBox::warning(this, "Rename", "Could not rename the item.");
    refresh();
}

void MainWindow::createNewFolder() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "New Folder", "Folder name:",
                                         QLineEdit::Normal, "New Folder", &ok);
    if (!ok || name.isEmpty()) return;

    if (!QDir(m_currentPath).mkdir(name))
        QMessageBox::warning(this, "New Folder", "Could not create folder.");
    refresh();
}

void MainWindow::createNewFile() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "New File", "File name:",
                                         QLineEdit::Normal, "untitled.txt", &ok);
    if (!ok || name.isEmpty()) return;

    QFile f(m_currentPath + "/" + name);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "New File", "Could not create file.");
        return;
    }
    f.close();
    refresh();
}

void MainWindow::toggleHidden() {
    m_showHidden = !m_showHidden;
    QDir::Filters f = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (m_showHidden)
        f |= QDir::Hidden | QDir::System;
    m_fsModel->setFilter(f);
    refresh();
}

void MainWindow::toggleViewMode() {
    bool details = !m_fileView->isDetailsMode();
    m_fileView->setDetailsMode(details);
    m_toolbar->setDetailsMode(details);
}

void MainWindow::search(const QString &query) {
    if (query.trimmed().isEmpty()) {
        m_fsModel->setNameFilters({});
        m_fsModel->setNameFilterDisables(false);
    } else {
        m_fsModel->setNameFilters({"*" + query.trimmed() + "*"});
        m_fsModel->setNameFilterDisables(false);
    }
}

void MainWindow::bookmarkCurrent() {
    m_sidebar->addCurrentAsBookmark();
}

void MainWindow::onFileActivated(const QModelIndex &index) {
    // Enter / double-click → folders navigate, files open in default app
    openFile(m_fsModel->filePath(index));
}

void MainWindow::onSelectionChanged() {
    updateStatusBar();
    auto paths = m_fileView->selectedPaths();
    if (paths.size() == 1 && QFileInfo(paths.first()).isFile()
        && isPreviewableFile(paths.first())) {
        showPreview(paths.first());
    }
}

int MainWindow::currentItemCount() const {
    QModelIndex root = m_fsModel->index(m_currentPath);
    return m_fsModel->rowCount(root);
}

void MainWindow::updateStatusBar() {
    auto selected = m_fileView->selectedPaths();
    qint64 size = selected.isEmpty() ? 0 : selectedTotalSize(selected);
    m_statusbar->updateInfo(currentItemCount(), selected.size(), size,
                            m_clipboard.size(), m_clipboardIsCut);
}

void MainWindow::updateNavButtons() {
    m_toolbar->setCanGoBack(!m_backStack.isEmpty());
    m_toolbar->setCanGoForward(!m_forwardStack.isEmpty());
}

#include "fileview.h"
#include "filefilter.h"
#include "theme.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QSet>
#include <QItemSelectionModel>

FileView::FileView(QFileSystemModel *model, QWidget *parent)
    : QWidget(parent), m_fsModel(model)
{
    m_proxy = new FileFilterProxy(this);
    m_proxy->setSourceFsModel(m_fsModel);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack);

    setupDetailsView();
    setupIconView();

    m_stack->addWidget(m_detailsView);
    m_stack->addWidget(m_iconView);
    m_stack->setCurrentWidget(m_detailsView);

    setStyleSheet(QString(
        "QTreeView, QListView {"
        "  background: %1;"
        "  color: %2;"
        "  border: none;"
        "  outline: none;"
        "  font-size: 13px;"
        "  alternate-background-color: %3;"
        "}"
        "QTreeView::item, QListView::item {"
        "  padding: 3px 4px;"
        "  min-height: 22px;"
        "}"
        "QTreeView::item:hover, QListView::item:hover {"
        "  background: %4;"
        "}"
        "QTreeView::item:selected, QListView::item:selected {"
        "  background: %5;"
        "  color: %6;"
        "}"
        "QHeaderView::section {"
        "  background: %3;"
        "  color: %6;"
        "  padding: 4px 8px;"
        "  border: none;"
        "  border-right: 1px solid %5;"
        "  border-bottom: 1px solid %5;"
        "  font-weight: 600;"
        "}"
    ).arg(Theme::BG, Theme::FG, Theme::BG2, Theme::HOVER, Theme::DIM, Theme::CYAN));
}

void FileView::setupDetailsView() {
    m_detailsView = new QTreeView(this);
    m_detailsView->setModel(m_proxy);
    m_detailsView->setRootIsDecorated(false);
    m_detailsView->setItemsExpandable(false);
    m_detailsView->setExpandsOnDoubleClick(false);
    m_detailsView->setUniformRowHeights(true);
    m_detailsView->setAlternatingRowColors(true);
    m_detailsView->setSortingEnabled(true);
    m_detailsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_detailsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_detailsView->setDragEnabled(true);
    m_detailsView->setAcceptDrops(true);
    m_detailsView->setDropIndicatorShown(true);
    m_detailsView->setDefaultDropAction(Qt::MoveAction);
    m_detailsView->setDragDropMode(QAbstractItemView::DragDrop);
    m_detailsView->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_detailsView->setAllColumnsShowFocus(true);
    m_detailsView->setIconSize(QSize(22, 22));

    auto *hdr = m_detailsView->header();
    hdr->setStretchLastSection(true);
    hdr->setSectionsClickable(true);
    hdr->setSortIndicatorShown(true);
    hdr->resizeSection(0, 320);
    hdr->resizeSection(1, 90);
    hdr->resizeSection(2, 120);
    hdr->setSortIndicator(0, Qt::AscendingOrder);

    m_proxy->sort(0, Qt::AscendingOrder);
    wireView(m_detailsView);
}

void FileView::setupIconView() {
    m_iconView = new QListView(this);
    m_iconView->setModel(m_proxy);
    m_iconView->setViewMode(QListView::IconMode);
    m_iconView->setIconSize(QSize(48, 48));
    m_iconView->setGridSize(QSize(100, 90));
    m_iconView->setSpacing(8);
    m_iconView->setResizeMode(QListView::Adjust);
    m_iconView->setMovement(QListView::Static);
    m_iconView->setWordWrap(true);
    m_iconView->setUniformItemSizes(true);
    m_iconView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_iconView->setDragEnabled(true);
    m_iconView->setAcceptDrops(true);
    m_iconView->setDropIndicatorShown(true);
    m_iconView->setDefaultDropAction(Qt::MoveAction);
    m_iconView->setDragDropMode(QAbstractItemView::DragDrop);
    m_iconView->setEditTriggers(QAbstractItemView::EditKeyPressed);
    wireView(m_iconView);
}

void FileView::wireView(QAbstractItemView *view) {
    connect(view, &QAbstractItemView::doubleClicked, this, &FileView::onDoubleClicked);
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &FileView::onSelectionChanged);
    view->viewport()->installEventFilter(this);
    view->installEventFilter(this);
}

QModelIndex FileView::toSource(const QModelIndex &proxyIndex) const {
    return m_proxy->mapToSource(proxyIndex);
}

void FileView::setRootIndex(const QModelIndex &sourceIndex) {
    QModelIndex proxyRoot = m_proxy->mapFromSource(sourceIndex);
    m_detailsView->setRootIndex(proxyRoot);
    m_iconView->setRootIndex(proxyRoot);
    clearSelection();
}

void FileView::setDetailsMode(bool details) {
    m_detailsMode = details;
    QStringList paths = selectedPaths();
    m_stack->setCurrentWidget(details ? static_cast<QWidget*>(m_detailsView)
                                      : static_cast<QWidget*>(m_iconView));
    clearSelection();
    auto *sel = currentView()->selectionModel();
    for (const auto &p : paths) {
        QModelIndex src = m_fsModel->index(p);
        QModelIndex prox = m_proxy->mapFromSource(src);
        if (prox.isValid())
            sel->select(prox, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}

void FileView::selectAll() {
    currentView()->selectAll();
}

void FileView::clearSelection() {
    currentView()->clearSelection();
}

QStringList FileView::selectedPaths() const {
    QStringList paths;
    QSet<QString> seen;
    auto *view = currentView();
    for (const auto &idx : view->selectionModel()->selectedIndexes()) {
        if (idx.column() != 0) continue;
        QString p = m_proxy->filePath(idx);
        if (!p.isEmpty() && !seen.contains(p)) {
            seen.insert(p);
            paths.append(p);
        }
    }
    return paths;
}

QModelIndex FileView::currentIndex() const {
    return toSource(currentView()->currentIndex());
}

QAbstractItemView *FileView::currentView() const {
    return m_detailsMode ? static_cast<QAbstractItemView*>(m_detailsView)
                         : static_cast<QAbstractItemView*>(m_iconView);
}

void FileView::onDoubleClicked(const QModelIndex &proxyIndex) {
    emit fileActivated(toSource(proxyIndex));
}

void FileView::onSelectionChanged() {
    emit selectionChanged();
}

bool FileView::eventFilter(QObject *obj, QEvent *event) {
    auto *view = currentView();
    if (obj == view || obj == view->viewport()) {
        if (event->type() == QEvent::ContextMenu) {
            auto *ce = static_cast<QContextMenuEvent*>(event);
            emit contextMenuRequested(ce->globalPos());
            return true;
        }
        if (event->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                QModelIndex idx = view->currentIndex();
                if (idx.isValid()) {
                    emit fileActivated(toSource(idx));
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

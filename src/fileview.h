#pragma once
#include <QWidget>
#include <QTreeView>
#include <QListView>
#include <QStackedWidget>
#include <QFileSystemModel>
#include <QAbstractItemView>
#include <QModelIndex>
#include <QStringList>
#include <QContextMenuEvent>

class FileFilterProxy;

class FileView : public QWidget {
    Q_OBJECT
public:
    explicit FileView(QFileSystemModel *model, QWidget *parent = nullptr);

    // Pass a source (QFileSystemModel) index
    void setRootIndex(const QModelIndex &sourceIndex);
    void setDetailsMode(bool details);
    bool isDetailsMode() const { return m_detailsMode; }
    void selectAll();
    void clearSelection();
    QStringList selectedPaths() const;
    QModelIndex currentIndex() const; // source index
    QAbstractItemView *currentView() const;

signals:
    void fileActivated(const QModelIndex &sourceIndex);
    void selectionChanged();
    void contextMenuRequested(const QPoint &globalPos);

private slots:
    void onDoubleClicked(const QModelIndex &proxyIndex);
    void onSelectionChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupDetailsView();
    void setupIconView();
    void wireView(QAbstractItemView *view);
    QModelIndex toSource(const QModelIndex &proxyIndex) const;

    QFileSystemModel *m_fsModel = nullptr;
    FileFilterProxy *m_proxy = nullptr;
    QStackedWidget *m_stack = nullptr;
    QTreeView *m_detailsView = nullptr;
    QListView *m_iconView = nullptr;
    bool m_detailsMode = true;
};

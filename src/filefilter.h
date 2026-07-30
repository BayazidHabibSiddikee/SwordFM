#pragma once
#include <QSortFilterProxyModel>
#include <QFileSystemModel>

class FileFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit FileFilterProxy(QObject *parent = nullptr);

    void setSourceFsModel(QFileSystemModel *model);
    QFileSystemModel *fsModel() const;

    QString filePath(const QModelIndex &proxyIndex) const;
    QFileInfo fileInfo(const QModelIndex &proxyIndex) const;
    bool isDir(const QModelIndex &proxyIndex) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    static bool isJunkName(const QString &name);
};

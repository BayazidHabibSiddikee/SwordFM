#include "filefilter.h"

#include <QFileInfo>
#include <QRegularExpression>

FileFilterProxy::FileFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void FileFilterProxy::setSourceFsModel(QFileSystemModel *model) {
    setSourceModel(model);
}

QFileSystemModel *FileFilterProxy::fsModel() const {
    return qobject_cast<QFileSystemModel*>(sourceModel());
}

QString FileFilterProxy::filePath(const QModelIndex &proxyIndex) const {
    auto *fs = fsModel();
    if (!fs || !proxyIndex.isValid()) return {};
    return fs->filePath(mapToSource(proxyIndex));
}

QFileInfo FileFilterProxy::fileInfo(const QModelIndex &proxyIndex) const {
    auto *fs = fsModel();
    if (!fs || !proxyIndex.isValid()) return {};
    return fs->fileInfo(mapToSource(proxyIndex));
}

bool FileFilterProxy::isDir(const QModelIndex &proxyIndex) const {
    auto *fs = fsModel();
    if (!fs || !proxyIndex.isValid()) return false;
    return fs->isDir(mapToSource(proxyIndex));
}

bool FileFilterProxy::isJunkName(const QString &name) {
    if (name.isEmpty()) return true;

    // Pure numeric names (e.g. "1000" from /run/user/1000 clutter)
    static const QRegularExpression pureNum(R"(^\d+$)");
    if (pureNum.match(name).hasMatch())
        return true;

    const QString lower = name.toLower();

    // libvirt / virt clutter
    if (lower.startsWith("libvirt") || lower.startsWith("libvirtd")
        || lower.contains("libvirt"))
        return true;

    // systemd unit / service files
    if (lower.endsWith(".service") || lower.endsWith(".socket")
        || lower.endsWith(".target") || lower.endsWith(".timer")
        || lower.endsWith(".mount") || lower.endsWith(".scope")
        || lower.endsWith(".slice") || lower.endsWith(".path")
        || lower.endsWith(".device") || lower.endsWith(".automount")
        || lower.endsWith(".swap"))
        return true;

    return false;
}

bool FileFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    auto *fs = fsModel();
    if (!fs) return true;

    QModelIndex idx = fs->index(sourceRow, 0, sourceParent);
    if (!idx.isValid()) return false;

    const QString name = fs->fileName(idx);
    if (isJunkName(name))
        return false;

    return true;
}

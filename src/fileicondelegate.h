#pragma once
#include <QStyledItemDelegate>
#include <QFileSystemModel>
#include <QPainter>
#include <QIcon>
#include <QMimeDatabase>
#include <QFileInfo>
#include "theme.h"

class FileIconDelegate : public QStyledItemDelegate {
public:
    explicit FileIconDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        painter->save();

        // Draw selection/hover background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor(Theme::DIM));
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor(Theme::HOVER));
        }

        // Get file info
        QString path = index.data(QFileSystemModel::FilePathRole).toString();
        QFileInfo fi(path);
        bool isDir = fi.isDir();

        // Get icon
        QIcon icon;
        if (isDir) {
            icon = QIcon::fromTheme("folder");
            if (icon.isNull()) {
                drawFolderIcon(painter, option.rect, index.data().toString());
                painter->restore();
                return;
            }
        } else {
            QMimeDatabase db;
            QString mime = db.mimeTypeForFile(fi).name();
            icon = QIcon::fromTheme(mime);
            if (icon.isNull()) icon = QIcon::fromTheme(fi.suffix());
            if (icon.isNull()) icon = QIcon::fromTheme("text-x-generic");
            if (icon.isNull()) {
                drawFileIcon(painter, option.rect, fi);
                painter->restore();
                return;
            }
        }

        // Draw icon
        int iconSize = 22;
        QRect iconRect = option.rect;
        iconRect.setWidth(iconSize);
        iconRect.setHeight(iconSize);
        iconRect.moveCenter(QPoint(option.rect.left() + iconSize/2 + 4, option.rect.center().y()));
        icon.paint(painter, iconRect);

        // Draw filename
        QFont font = painter->font();
        painter->setFont(font);
        painter->setPen(QColor(Theme::FG));

        QString name = index.data().toString();
        QRect textRect = option.rect;
        textRect.setLeft(iconRect.right() + 8);
        textRect.adjust(0, 0, -8, 0);

        QFontMetrics fm(font);
        QString elided = fm.elidedText(name, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(0, 26);
    }

private:
    void drawFolderIcon(QPainter *p, const QRect &rect, const QString &name) const {
        int iconSize = 20;
        QRect r = rect;
        r.setWidth(iconSize + 8);
        r.setHeight(iconSize);
        r.moveCenter(QPoint(rect.left() + iconSize/2 + 4, rect.center().y()));

        QColor folderColor(97, 175, 239);
        p->setBrush(folderColor);
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(r.x() + 2, r.y() + 5, r.width() - 4, r.height() - 5, 2, 2);
        p->drawRoundedRect(r.x() + 2, r.y() + 2, 8, 5, 2, 2);
    }

    void drawFileIcon(QPainter *p, const QRect &rect, const QFileInfo &fi) const {
        int iconSize = 20;
        QRect r = rect;
        r.setWidth(iconSize + 8);
        r.setHeight(iconSize);
        r.moveCenter(QPoint(rect.left() + iconSize/2 + 4, rect.center().y()));

        QString ext = fi.suffix().toLower();
        QColor color;
        if (ext.contains("py") || ext.contains("js") || ext.contains("ts"))
            color = QColor(229, 192, 123);
        else if (ext.contains("cpp") || ext.contains("c") || ext.contains("h") || ext.contains("rs"))
            color = QColor(97, 175, 239);
        else if (ext.contains("md") || ext.contains("txt"))
            color = QColor(152, 195, 121);
        else if (ext.contains("png") || ext.contains("jpg") || ext.contains("gif"))
            color = QColor(224, 108, 117);
        else if (ext.contains("json") || ext.contains("yaml") || ext.contains("toml"))
            color = QColor(171, 178, 191);
        else
            color = QColor(152, 195, 121);

        p->setBrush(color);
        p->setPen(Qt::NoPen);

        QRect fileRect = r.adjusted(2, 2, -2, -2);
        p->drawRoundedRect(fileRect, 2, 2);

        QPolygon fold;
        fold << QPoint(fileRect.right() - 4, fileRect.top())
             << QPoint(fileRect.right(), fileRect.top() + 4)
             << QPoint(fileRect.right() - 4, fileRect.top() + 4);
        p->setBrush(QColor(Theme::BG2));
        p->drawPolygon(fold);
    }
};

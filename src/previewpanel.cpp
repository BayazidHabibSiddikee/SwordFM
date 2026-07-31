#include "previewpanel.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QPixmap>
#include <QFont>
#include <QFontDatabase>
#include <QImageReader>
#include <QTextOption>
#include <QTextCursor>
#include <QProcess>
#include <QDir>

static const qint64 kMaxTextBytes = 2 * 1024 * 1024; // 2 MB

static bool isImagePath(const QFileInfo &fi, const QString &mime) {
    if (mime.startsWith("image/"))
        return true;
    static const QStringList imgExt = {
        "png", "jpg", "jpeg", "gif", "webp", "bmp", "svg", "ico",
        "tif", "tiff", "avif", "jxl", "heic"
    };
    return imgExt.contains(fi.suffix().toLower());
}

static bool isTextish(const QFileInfo &fi, const QString &mime) {
    if (mime.startsWith("text/"))
        return true;
    static const QStringList mimeOk = {
        "application/json", "application/xml", "application/javascript",
        "application/x-shellscript", "application/x-desktop",
        "application/x-yaml", "inode/x-empty",
    };
    if (mimeOk.contains(mime))
        return true;
    static const QStringList textExt = {
        "md", "txt", "log", "conf", "cfg", "ini", "toml", "yaml", "yml",
        "json", "xml", "csv", "rs", "py", "cpp", "h", "hpp", "c", "cc",
        "js", "ts", "tsx", "jsx", "css", "html", "htm", "sh", "bash", "zsh",
        "go", "java", "kt", "lua", "rb", "php", "sql", "vim", "nix",
        "cmake", "makefile", "mk", "gradle", "dart", "swift", "r", "jl",
    };
    return textExt.contains(fi.suffix().toLower())
        || fi.fileName().compare("Makefile", Qt::CaseInsensitive) == 0
        || fi.fileName().compare("CMakeLists.txt", Qt::CaseInsensitive) == 0;
}

PreviewPanel::PreviewPanel(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(280);
    setStyleSheet(QString(
        "PreviewPanel { background: %1; border-left: 1px solid %2; }"
        "QLabel#previewTitle { color: %3; font-weight: 600; font-size: 12px; padding: 0 4px; }"
        "QLabel#previewEmpty { color: %4; font-size: 13px; }"
        "QLabel#previewStub { color: %4; font-size: 13px; padding: 16px; }"
        "QPlainTextEdit {"
        "  background: %1; color: %5; border: none;"
        "  selection-background-color: %2; selection-color: %3;"
        "}"
        "QToolButton {"
        "  background: transparent; border: none; color: %4; border-radius: 4px; padding: 2px;"
        "}"
        "QToolButton:hover { background: %2; color: %3; }"
        "QScrollArea { background: %1; border: none; }"
    ).arg(Theme::BG, Theme::DIM, Theme::CYAN, Theme::FG_DIM, Theme::FG));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *header = new QWidget(this);
    header->setFixedHeight(34);
    header->setStyleSheet(QString("background: %1; border-bottom: 1px solid %2;")
                              .arg(Theme::BG2, Theme::BORDER));
    auto *hdrLay = new QHBoxLayout(header);
    hdrLay->setContentsMargins(10, 0, 6, 0);

    m_title = new QLabel("PREVIEW", header);
    m_title->setObjectName("previewTitle");

    m_closeBtn = new QToolButton(header);
    m_closeBtn->setText(QStringLiteral("✕"));
    m_closeBtn->setToolTip("Close preview");
    m_closeBtn->setFixedSize(24, 24);
    connect(m_closeBtn, &QToolButton::clicked, this, [this]() {
        clearPreview();
        emit closeRequested();
    });

    hdrLay->addWidget(m_title, 1);
    hdrLay->addWidget(m_closeBtn);
    root->addWidget(header);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack, 1);

    m_emptyPage = new QWidget(this);
    auto *emptyLay = new QVBoxLayout(m_emptyPage);
    auto *emptyLbl = new QLabel("Select a file to preview\ntext · code · images", m_emptyPage);
    emptyLbl->setObjectName("previewEmpty");
    emptyLbl->setAlignment(Qt::AlignCenter);
    emptyLay->addStretch();
    emptyLay->addWidget(emptyLbl);
    emptyLay->addStretch();
    m_stack->addWidget(m_emptyPage);

    m_textView = new QPlainTextEdit(this);
    m_textView->setReadOnly(true);
    m_textView->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_textView->setWordWrapMode(QTextOption::NoWrap);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(11);
    m_textView->setFont(mono);
    m_stack->addWidget(m_textView);

    m_imageScroll = new QScrollArea(this);
    m_imageScroll->setWidgetResizable(true);
    m_imageScroll->setAlignment(Qt::AlignCenter);
    m_imageLabel = new QLabel;
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet(QString("background: %1;").arg(Theme::BG));
    m_imageScroll->setWidget(m_imageLabel);
    m_stack->addWidget(m_imageScroll);

    m_stubLabel = new QLabel(this);
    m_stubLabel->setObjectName("previewStub");
    m_stubLabel->setAlignment(Qt::AlignCenter);
    m_stubLabel->setWordWrap(true);
    m_stack->addWidget(m_stubLabel);

    showEmpty();
}

void PreviewPanel::showEmpty() {
    m_path.clear();
    m_title->setText("PREVIEW");
    m_stack->setCurrentWidget(m_emptyPage);
    m_textView->clear();
    m_imageLabel->clear();
}

void PreviewPanel::clearPreview() {
    showEmpty();
}

bool PreviewPanel::loadTextFile(const QString &path, QString *out, QString *error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        *error = "Cannot open file";
        return false;
    }
    if (f.size() > kMaxTextBytes) {
        *error = QString("File too large to preview (%1 MB)")
                     .arg(f.size() / (1024.0 * 1024), 0, 'f', 1);
        return false;
    }
    QByteArray data = f.readAll();
    int nulls = 0;
    const int check = qMin(data.size(), 8192);
    for (int i = 0; i < check; ++i) {
        if (data.at(i) == '\0')
            ++nulls;
    }
    if (nulls > 0) {
        *error = "Binary file";
        return false;
    }
    *out = QString::fromUtf8(data);
    return true;
}

void PreviewPanel::showText(const QString &path) {
    QString text, err;
    if (!loadTextFile(path, &text, &err)) {
        showBinaryStub(path, err);
        return;
    }
    m_textView->setPlainText(text);
    m_textView->moveCursor(QTextCursor::Start);
    m_stack->setCurrentWidget(m_textView);
}

void PreviewPanel::showImage(const QString &path) {
    QPixmap px;
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage img = reader.read();
    if (!img.isNull())
        px = QPixmap::fromImage(img);
    else
        px = QPixmap(path);

    if (px.isNull()) {
        showBinaryStub(path, "Could not load image");
        return;
    }

    int maxW = qMax(200, width() - 24);
    int maxH = qMax(200, height() - 60);
    if (px.width() > maxW || px.height() > maxH)
        px = px.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    m_imageLabel->setPixmap(px);
    m_imageLabel->adjustSize();
    m_stack->setCurrentWidget(m_imageScroll);
}

void PreviewPanel::showBinaryStub(const QString &path, const QString &reason) {
    QFileInfo fi(path);
    m_stubLabel->setText(QString("%1\n\n%2\n%3 bytes")
                             .arg(fi.fileName(), reason)
                             .arg(fi.size()));
    m_stack->setCurrentWidget(m_stubLabel);
}

void PreviewPanel::previewFile(const QString &path) {
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) {
        showEmpty();
        return;
    }

    m_path = fi.absoluteFilePath();
    m_title->setText(fi.fileName());

    QMimeDatabase db;
    const QString mime = db.mimeTypeForFile(fi).name();

    if (isImagePath(fi, mime)) {
        showImage(path);
        return;
    }
    if (isTextish(fi, mime)) {
        showText(path);
        return;
    }

    QString text, err;
    if (fi.size() <= kMaxTextBytes && loadTextFile(path, &text, &err)) {
        m_textView->setPlainText(text);
        m_stack->setCurrentWidget(m_textView);
        return;
    }

    showBinaryStub(path, err.isEmpty() ? "No preview available" : err);
}

void PreviewPanel::previewFolderGraph(const QString &folderPath) {
    QFileInfo fi(folderPath);
    if (!fi.exists() || !fi.isDir()) {
        showEmpty();
        return;
    }

    m_path = folderPath;
    m_title->setText("GRAPH: " + fi.fileName());

    // Generate graph data
    QStringList nodes;
    QStringList edges;
    int nodeId = 0;

    // Root folder node
    nodes.append(QString("  %1 [label=\"%2\", shape=box, style=filled, "
                         "fillcolor=\"#61afef25\", color=\"#61afef\"];")
                     .arg(nodeId).arg(fi.fileName()));
    int rootId = nodeId;
    nodeId++;

    // Scan folder (max 2 levels deep)
    QDir dir(folderPath);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name | QDir::DirsFirst);

    QFileInfoList entries = dir.entryInfoList();
    int maxEntries = qMin(entries.size(), 50);

    for (int i = 0; i < maxEntries; ++i) {
        const QFileInfo &entry = entries[i];
        if (entry.fileName().startsWith('.') && entry.fileName() != ".git")
            continue;

        QString ext = entry.suffix().toLower();
        QString color;
        if (entry.isDir())
            color = "#61afef";
        else if (ext == "py" || ext == "js" || ext == "ts")
            color = "#e5c07b";
        else if (ext == "cpp" || ext == "c" || ext == "h" || ext == "rs")
            color = "#61afef";
        else if (ext == "md" || ext == "txt")
            color = "#98c379";
        else if (ext == "png" || ext == "jpg" || ext == "gif")
            color = "#e06c75";
        else
            color = "#abb2bf";

        QString label = entry.fileName();
        if (label.length() > 15)
            label = label.left(12) + "...";

        QString shape = entry.isDir() ? "box" : "ellipse";
        QString fill = entry.isDir() ? "#61afef25" : color + "25";

        nodes.append(QString("  %1 [label=\"%2\", shape=%3, style=filled, "
                             "fillcolor=\"%4\", color=\"%5\"];")
                         .arg(nodeId).arg(label.toHtmlEscaped())
                         .arg(shape).arg(fill).arg(color));

        edges.append(QString("  %1 -> %2;").arg(rootId).arg(nodeId));
        nodeId++;

        // Scan subdirectories one level
        if (entry.isDir()) {
            QDir subDir(entry.absoluteFilePath());
            subDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
            subDir.setSorting(QDir::Name);
            QFileInfoList subEntries = subDir.entryInfoList();
            int subMax = qMin(subEntries.size(), 10);
            for (int j = 0; j < subMax; ++j) {
                const QFileInfo &sub = subEntries[j];
                QString subExt = sub.suffix().toLower();
                QString subColor;
                if (subExt == "py" || subExt == "js") subColor = "#e5c07b";
                else if (subExt == "cpp" || subExt == "h") subColor = "#61afef";
                else if (subExt == "md" || subExt == "txt") subColor = "#98c379";
                else subColor = "#abb2bf";

                QString subLabel = sub.fileName();
                if (subLabel.length() > 12) subLabel = subLabel.left(9) + "...";

                nodes.append(QString("  %1 [label=\"%2\", shape=ellipse, style=filled, "
                                     "fillcolor=\"%325\", color=\"%3\"];")
                                 .arg(nodeId).arg(subLabel.toHtmlEscaped())
                                 .arg(subColor));
                edges.append(QString("  %1 -> %2;").arg(nodeId - 1).arg(nodeId));
                nodeId++;
            }
        }
    }

    // Build Graphviz dot
    QString dot = "digraph G {\n"
                  "  layout=neato;\n"
                  "  overlap=false;\n"
                  "  splines=ortho;\n"
                  "  bgcolor=\"transparent\";\n"
                  "  ratio=fill;\n"
                  "  size=\"4,4\";\n"
                  "  node [fontname=\"DejaVu Sans Mono\", fontsize=10];\n"
                  "  edge [color=\"#3e4451\", arrowsize=0.5];\n\n";

    for (const auto &n : nodes)
        dot += n + "\n";
    dot += "\n";
    for (const auto &e : edges)
        dot += e + "\n";
    dot += "}\n";

    // Save and render
    QString tmpDir = QDir::tempPath();
    QString dotPath = tmpDir + "/swordfm_graph.dot";
    QString pngPath = tmpDir + "/swordfm_graph.png";

    QFile dotFile(dotPath);
    if (dotFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        dotFile.write(dot.toUtf8());
        dotFile.close();
    }

    // Render with Graphviz
    QProcess proc;
    proc.start("neato", {"-Tpng", "-o", pngPath, dotPath});
    proc.waitForFinished(5000);

    if (QFileInfo::exists(pngPath)) {
        QPixmap px(pngPath);
        if (!px.isNull()) {
            int maxW = qMax(200, width() - 24);
            int maxH = qMax(200, height() - 60);
            if (px.width() > maxW || px.height() > maxH)
                px = px.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            m_imageLabel->setPixmap(px);
            m_imageLabel->adjustSize();
            m_stack->setCurrentWidget(m_imageScroll);

            // Clean up
            QFile::remove(dotPath);
            QFile::remove(pngPath);
            return;
        }
    }

    // Fallback: show dot source
    m_textView->setPlainText(dot);
    m_stack->setCurrentWidget(m_textView);

    QFile::remove(dotPath);
    QFile::remove(pngPath);
}

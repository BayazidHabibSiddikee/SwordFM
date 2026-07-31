#include "openwith.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcess>
#include <QStandardPaths>
#include <QSettings>
#include <QSet>
#include <QRegularExpression>
#include <QUrl>
#include <QIcon>
#include <QApplication>
#include <QStyle>

static QStringList desktopSearchPaths() {
    QStringList paths;
    paths << QDir::homePath() + "/.local/share/applications";
    const auto system = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const auto &p : system) {
        if (!paths.contains(p))
            paths << p;
    }
    paths << "/usr/share/applications";
    paths << "/usr/local/share/applications";
    return paths;
}

static QString findDesktopFile(const QString &desktopId) {
    QString id = desktopId;
    if (!id.endsWith(".desktop"))
        id += ".desktop";
    for (const auto &dir : desktopSearchPaths()) {
        QString path = dir + "/" + id;
        if (QFileInfo::exists(path))
            return path;
        // Flatpak-style nested ids: foo/bar.desktop
        if (id.contains('-')) {
            // try as-is only
        }
    }
    // Recursive-ish: also check subdirs one level (flatpak exports)
    for (const auto &dir : desktopSearchPaths()) {
        QDir d(dir);
        const auto entries = d.entryList({"*.desktop"}, QDir::Files);
        for (const auto &e : entries) {
            if (e.compare(id, Qt::CaseInsensitive) == 0)
                return d.absoluteFilePath(e);
        }
    }
    return {};
}

static AppHandler parseDesktopFile(const QString &path, const QString &desktopId) {
    AppHandler app;
    app.desktopId = desktopId.contains('.') ? desktopId : QFileInfo(path).fileName();
    if (!app.desktopId.endsWith(".desktop"))
        app.desktopId += ".desktop";

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return app;

    bool inDesktop = false;
    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.startsWith('[')) {
            inDesktop = (line == "[Desktop Entry]");
            continue;
        }
        if (!inDesktop || line.isEmpty() || line.startsWith('#'))
            continue;
        const int eq = line.indexOf('=');
        if (eq < 0) continue;
        const QString key = line.left(eq);
        const QString val = line.mid(eq + 1);
        if (key == "Name" && app.name.isEmpty())
            app.name = val;
        else if (key == "Exec" && app.exec.isEmpty())
            app.exec = val;
        else if (key == "Icon" && app.iconName.isEmpty())
            app.iconName = val;
        else if (key == "NoDisplay" && val.toLower() == "true")
            app.name.clear(); // mark invalid-ish
        else if (key == "Hidden" && val.toLower() == "true")
            app.name.clear();
    }
    if (app.name.isEmpty() && !app.exec.isEmpty())
        app.name = QFileInfo(app.desktopId).completeBaseName();
    return app;
}

static QStringList mimeappsDefaults(const QString &mime) {
    QStringList result;
    const QStringList files = {
        QDir::homePath() + "/.config/mimeapps.list",
        QDir::homePath() + "/.local/share/applications/mimeapps.list",
        "/usr/share/applications/mimeapps.list",
    };
    for (const auto &file : files) {
        if (!QFileInfo::exists(file)) continue;
        QSettings ini(file, QSettings::IniFormat);
        ini.beginGroup("Default Applications");
        QString v = ini.value(mime).toString();
        ini.endGroup();
        if (!v.isEmpty()) {
            for (const auto &part : v.split(';', Qt::SkipEmptyParts))
                result.append(part.trimmed());
        }
        ini.beginGroup("Added Associations");
        v = ini.value(mime).toString();
        ini.endGroup();
        if (!v.isEmpty()) {
            for (const auto &part : v.split(';', Qt::SkipEmptyParts)) {
                const QString id = part.trimmed();
                if (!result.contains(id))
                    result.append(id);
            }
        }
    }
    return result;
}

static QStringList mimeinfoCacheHandlers(const QString &mime) {
    QStringList result;
    for (const auto &dir : desktopSearchPaths()) {
        QFile f(dir + "/mimeinfo.cache");
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        while (!f.atEnd()) {
            QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (!line.startsWith(mime + "="))
                continue;
            const QString apps = line.mid(mime.size() + 1);
            for (const auto &part : apps.split(';', Qt::SkipEmptyParts)) {
                const QString id = part.trimmed();
                if (!result.contains(id))
                    result.append(id);
            }
            break;
        }
    }
    return result;
}

static bool desktopHandlesMime(const QString &desktopPath, const QString &mime) {
    QFile f(desktopPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    bool inDesktop = false;
    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.startsWith('[')) {
            inDesktop = (line == "[Desktop Entry]");
            continue;
        }
        if (!inDesktop) continue;
        if (line.startsWith("MimeType=")) {
            const QStringList types = line.mid(9).split(';', Qt::SkipEmptyParts);
            for (const auto &t : types) {
                if (t == mime) return true;
                // video/* style
                if (t.endsWith("/*") && mime.startsWith(t.left(t.size() - 1)))
                    return true;
            }
            return false;
        }
    }
    return false;
}

QStringList preferredVideoPlayers() {
    return {
        "mpv.desktop",
        "vlc.desktop",
        "celluloid.desktop",
        "io.github.celluloid_player.Celluloid.desktop",
        "smplayer.desktop",
        "org.gnome.Totem.desktop",
        "totem.desktop",
        "com.github.rafostar.Clapper.desktop",
        "org.kde.haruna.desktop",
        "haruna.desktop",
        "org.kde.dragonplayer.desktop",
        "dragon.desktop",
        "mplayer.desktop",
        "ffplay.desktop",
    };
}

QStringList preferredImageViewers() {
    return {
        "eog.desktop",
        "org.gnome.eog.desktop",
        "feh.desktop",
        "sxiv.desktop",
        "imv.desktop",
        "nsxiv.desktop",
        "display.desktop",
        "gwenview.desktop",
        "org.kde.gwenview.desktop",
        "ristretto.desktop",
        "nomacs.desktop",
    };
}

QStringList preferredTextEditors() {
    return {
        "org.gnome.TextEditor.desktop",
        "org.gnome.gedit.desktop",
        "gedit.desktop",
        "kate.desktop",
        "org.kde.kate.desktop",
        "kwrite.desktop",
        "org.kde.kwrite.desktop",
        "xfce4-terminal.desktop",
        "org.xfce.Terminal.desktop",
        "konsole.desktop",
        "org.kde.konsole.desktop",
        "io.github.nickvision.paragraph.desktop",
        "notepadqq.desktop",
        "sublime_text.desktop",
        "code.desktop",
        "codium.desktop",
        "vscodium.desktop",
        "helix.desktop",
    };
}

QStringList preferredAudioPlayers() {
    return {
        "audacious.desktop",
        "rhythmbox.desktop",
        "org.gnome.Rhythmbox.desktop",
        "lollypop.desktop",
        "org.gnome.Lollypop.desktop",
        "playerctl.desktop",
        "cmus.desktop",
        "spotify.desktop",
        "io.github.quodlibet.QuodLibet.desktop",
    };
}

QStringList preferredPdfReaders() {
    return {
        "evince.desktop",
        "org.gnome.Evince.desktop",
        "okular.desktop",
        "org.kde.okular.desktop",
        "zathura.desktop",
        "mupdf.desktop",
        "atril.desktop",
        "xreader.desktop",
    };
}

QStringList preferredArchivers() {
    return {
        "file-roller.desktop",
        "org.gnome.FileRoller.desktop",
        "engrampa.desktop",
        "org.mate.FileRoller.desktop",
        "ark.desktop",
        "org.kde.ark.desktop",
        "xarchiver.desktop",
    };
}

QList<AppHandler> allInstalledApps() {
    QList<AppHandler> apps;
    QSet<QString> seen;

    for (const auto &dir : desktopSearchPaths()) {
        QDir d(dir);
        const auto entries = d.entryList({"*.desktop"}, QDir::Files);
        for (const auto &e : entries) {
            if (seen.contains(e)) continue;
            const QString full = d.absoluteFilePath(e);
            AppHandler app = parseDesktopFile(full, e);
            if (app.name.isEmpty() || app.exec.isEmpty()) continue;
            // Skip hidden/no-display/terminal apps
            QFile f(full);
            bool skip = false;
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                bool inDesktop = false;
                while (!f.atEnd()) {
                    QString line = QString::fromUtf8(f.readLine()).trimmed();
                    if (line.startsWith('[')) {
                        inDesktop = (line == "[Desktop Entry]");
                        continue;
                    }
                    if (!inDesktop) continue;
                    if (line == "NoDisplay=true" || line == "Hidden=true"
                        || line == "Terminal=true") {
                        skip = true;
                        break;
                    }
                }
            }
            if (skip) continue;
            seen.insert(e);
            apps.append(app);
        }
    }

    // Sort by name
    std::sort(apps.begin(), apps.end(), [](const AppHandler &a, const AppHandler &b) {
        return a.name.toLower() < b.name.toLower();
    });

    return apps;
}

static bool looksLikeBrowser(const QString &desktopId) {
    const QString id = desktopId.toLower();
    return id.contains("firefox") || id.contains("chrome") || id.contains("chromium")
        || id.contains("brave") || id.contains("zen") || id.contains("edge")
        || id.contains("opera") || id.contains("vivaldi") || id.contains("librewolf")
        || id.contains("epiphany") || id.contains("brows");
}

QIcon AppHandler::icon() const {
    if (!iconName.isEmpty()) {
        QIcon ic = QIcon::fromTheme(iconName);
        if (!ic.isNull()) return ic;
        if (QFileInfo::exists(iconName))
            return QIcon(iconName);
    }
    return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
}

QList<AppHandler> appsForFile(const QString &path) {
    QList<AppHandler> apps;
    QSet<QString> seen;

    QMimeDatabase db;
    const QString mime = db.mimeTypeForFile(path).name();
    const bool isVideo = mime.startsWith("video/");
    const bool isImage = mime.startsWith("image/");
    const bool isAudio = mime.startsWith("audio/");
    const bool isText = mime.startsWith("text/") || mime == "application/json"
        || mime == "application/xml" || mime == "application/javascript"
        || mime == "application/x-shellscript" || mime == "application/x-perl"
        || mime == "application/x-python" || mime == "application/x-c++"
        || mime == "application/x-java" || mime == "application/x-rust"
        || mime == "application/x-go" || mime == "application/xmake"
        || mime == "application/x-cmake" || mime == "application/toml"
        || mime == "application/x-yaml" || mime == "application/x-toml";
    const bool isPdf = mime == "application/pdf";
    const bool isArchive = mime.contains("zip") || mime.contains("tar")
        || mime.contains("gzip") || mime.contains("bzip2")
        || mime.contains("x-xz") || mime.contains("x-7z")
        || mime.contains("x-rar") || mime.contains("x-cpio")
        || mime.contains("x-deb") || mime.contains("x-rpm");

    auto addId = [&](const QString &desktopId, bool asDefault = false) {
        if (desktopId.isEmpty() || seen.contains(desktopId)) {
            if (asDefault) {
                for (auto &a : apps) {
                    if (a.desktopId == desktopId)
                        a.isDefault = true;
                }
            }
            return;
        }
        const QString file = findDesktopFile(desktopId);
        if (file.isEmpty()) return;
        AppHandler app = parseDesktopFile(file, desktopId);
        if (app.exec.isEmpty()) return;
        app.isDefault = asDefault;
        seen.insert(app.desktopId);
        apps.append(app);
    };

    // 1) mimeapps defaults / associations
    const QStringList fromMimeapps = mimeappsDefaults(mime);
    for (int i = 0; i < fromMimeapps.size(); ++i)
        addId(fromMimeapps[i], i == 0);

    // 2) Inject known apps by category near the top
    if (isVideo) {
        for (const auto &id : preferredVideoPlayers())
            addId(id, false);
    } else if (isImage) {
        for (const auto &id : preferredImageViewers())
            addId(id, false);
    } else if (isAudio) {
        for (const auto &id : preferredAudioPlayers())
            addId(id, false);
    } else if (isText) {
        for (const auto &id : preferredTextEditors())
            addId(id, false);
    } else if (isPdf) {
        for (const auto &id : preferredPdfReaders())
            addId(id, false);
    } else if (isArchive) {
        for (const auto &id : preferredArchivers())
            addId(id, false);
    }

    // 3) mimeinfo.cache
    for (const auto &id : mimeinfoCacheHandlers(mime))
        addId(id, false);

    // 4) Parent MIME (e.g. video/mp4 -> also check nothing extra; scan players' desktop MimeType)
    if (isVideo) {
        for (const auto &dir : desktopSearchPaths()) {
            QDir d(dir);
            for (const auto &e : d.entryList({"*.desktop"}, QDir::Files)) {
                const QString full = d.absoluteFilePath(e);
                if (desktopHandlesMime(full, mime) || desktopHandlesMime(full, "video/mp4")
                    || desktopHandlesMime(full, "video/x-matroska")
                    || desktopHandlesMime(full, "video/*"))
                    addId(e, false);
            }
        }
    }

    // Prefer non-browser as default for videos if current default is a browser
    if (isVideo && !apps.isEmpty() && looksLikeBrowser(apps.first().desktopId)) {
        for (int i = 1; i < apps.size(); ++i) {
            if (!looksLikeBrowser(apps[i].desktopId)) {
                apps.move(i, 0);
                break;
            }
        }
        for (int i = 0; i < apps.size(); ++i)
            apps[i].isDefault = (i == 0);
    } else if (!apps.isEmpty()) {
        // Ensure exactly one default flag
        bool any = false;
        for (auto &a : apps) {
            if (a.isDefault) { any = true; break; }
        }
        if (!any)
            apps[0].isDefault = true;
    }

    return apps;
}

AppHandler defaultAppForFile(const QString &path) {
    const auto apps = appsForFile(path);
    for (const auto &a : apps) {
        if (a.isDefault)
            return a;
    }
    if (!apps.isEmpty())
        return apps.first();
    return {};
}

static QStringList splitExec(const QString &exec) {
    // Simple shell-ish split respecting single/double quotes
    QStringList out;
    QString cur;
    QChar quote;
    for (int i = 0; i < exec.size(); ++i) {
        QChar c = exec[i];
        if (!quote.isNull()) {
            if (c == quote) quote = QChar();
            else cur += c;
            continue;
        }
        if (c == '"' || c == '\'') {
            quote = c;
            continue;
        }
        if (c.isSpace()) {
            if (!cur.isEmpty()) {
                out << cur;
                cur.clear();
            }
            continue;
        }
        cur += c;
    }
    if (!cur.isEmpty())
        out << cur;
    return out;
}

bool openWithApp(const AppHandler &app, const QString &path) {
    if (app.exec.isEmpty() || path.isEmpty())
        return false;

    const QString abs = QFileInfo(path).absoluteFilePath();
    const QString uri = QUrl::fromLocalFile(abs).toString();

    QString exec = app.exec;
    // Strip field codes we don't support after expansion
    bool hasFileCode = exec.contains("%f") || exec.contains("%F")
        || exec.contains("%u") || exec.contains("%U");

    exec.replace("%F", abs);
    exec.replace("%f", abs);
    exec.replace("%U", uri);
    exec.replace("%u", uri);
    exec.replace("%c", app.name);
    exec.replace("%k", app.desktopId);
    exec.replace("%%", "%");
    // Remove remaining lone codes
    exec.replace(QRegularExpression("%[a-zA-Z]"), "");

    QStringList parts = splitExec(exec.trimmed());
    if (parts.isEmpty())
        return false;

    const QString cmd = parts.takeFirst();
    if (!hasFileCode)
        parts.append(abs);

    // Try PATH + absolute
    if (QProcess::startDetached(cmd, parts))
        return true;

    // Fallback: gtk-launch
    if (QProcess::startDetached("gtk-launch", {app.desktopId, abs}))
        return true;

    return false;
}

bool openWithDefault(const QString &path) {
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile())
        return false;

    AppHandler app = defaultAppForFile(path);
    if (!app.exec.isEmpty() && openWithApp(app, path))
        return true;

    // Direct player fallbacks for video
    QMimeDatabase db;
    if (db.mimeTypeForFile(fi).name().startsWith("video/")) {
        for (const auto &id : preferredVideoPlayers()) {
            const QString file = findDesktopFile(id);
            if (file.isEmpty()) continue;
            AppHandler a = parseDesktopFile(file, id);
            if (!a.exec.isEmpty() && openWithApp(a, path))
                return true;
        }
        // Last resort binaries
        for (const char *bin : {"mpv", "vlc", "celluloid", "ffplay"}) {
            if (QProcess::startDetached(bin, {fi.absoluteFilePath()}))
                return true;
        }
    }

    // Generic fallback
    if (QProcess::startDetached("xdg-open", {fi.absoluteFilePath()}))
        return true;
    if (QProcess::startDetached("gio", {"open", fi.absoluteFilePath()}))
        return true;

    return false;
}

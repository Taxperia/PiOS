// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAbstractListModel>
#include <QFileInfo>
#include <QVariantMap>
#include <QFutureWatcher>
#include <QFileSystemWatcher>
#include <QUrl>
#include <functional>

class FilesModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
    Q_PROPERTY(bool canPaste READ canPaste NOTIFY clipboardChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool inTrash READ inTrash NOTIFY pathChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY optionsChanged)
    Q_PROPERTY(QString sortBy READ sortBy WRITE setSortBy NOTIFY optionsChanged)
    Q_PROPERTY(QString operationStatus READ operationStatus NOTIFY busyChanged)

public:
    enum Role { NameRole = Qt::UserRole + 1, PathRole, DirectoryRole, SizeRole, IconRole };

    explicit FilesModel(const QString &initialPath = {}, QObject *parent = nullptr);
    ~FilesModel() override;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString path() const { return m_path; }
    QString filter() const { return m_filter; }
    QString errorText() const { return m_error; }
    bool canGoBack() const { return m_historyIndex > 0; }
    bool canGoForward() const { return m_historyIndex + 1 < m_history.size(); }
    bool canPaste() const;
    bool busy() const { return m_busy; }
    bool inTrash() const { return m_path == placePath(QStringLiteral("trash")); }
    bool showHidden() const { return m_showHidden; }
    QString sortBy() const { return m_sortBy; }
    QString operationStatus() const { return m_operationStatus; }
    void setShowHidden(bool value);
    void setSortBy(const QString &value);
    Q_INVOKABLE void copyEntries(const QVariantList &rows, bool cut = false);
    Q_INVOKABLE void paste();
    Q_INVOKABLE void refreshClipboard();
    Q_INVOKABLE void dropUrls(const QList<QUrl> &urls, const QString &destination, bool move);
    Q_INVOKABLE QList<QUrl> entryUrls(const QVariantList &rows) const;
    Q_INVOKABLE void trashEntries(const QVariantList &rows, bool permanently = false);
    Q_INVOKABLE void restoreEntries(const QVariantList &rows);
    Q_INVOKABLE QVariantMap properties(int row) const;

    void setFilter(const QString &value);
    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE void setPath(const QString &path);
    Q_INVOKABLE void back();
    Q_INVOKABLE void forward();
    Q_INVOKABLE void up();
    Q_INVOKABLE void home();
    Q_INVOKABLE QString placePath(const QString &place) const;
    Q_INVOKABLE bool openPlace(const QString &place);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString entryName(int row) const;
    Q_INVOKABLE bool createFolder(const QString &name);
    Q_INVOKABLE bool createFile(const QString &name);
    Q_INVOKABLE bool renameEntry(int row, const QString &newName);
    Q_INVOKABLE bool moveToTrash(int row);
    Q_INVOKABLE bool deletePermanently(int row);
    Q_INVOKABLE bool openTerminalHere();

signals:
    void pathChanged();
    void filterChanged();
    void errorTextChanged();
    void historyChanged();
    void clipboardChanged();
    void busyChanged();
    void optionsChanged();

private:
    bool validName(const QString &name) const;
    bool validRow(int row) const;
    bool load(const QString &path, bool record);
    void setError(const QString &message);
    void transfer(const QStringList &sources, const QString &destination, bool move, bool clipboard);
    void runOperation(const std::function<QString()> &operation, const QString &status, bool clearClipboard = false);

    QList<QFileInfo> m_all;
    QList<QFileInfo> m_visible;
    QString m_path;
    QString m_filter;
    QString m_error;
    QStringList m_history;
    int m_historyIndex = -1;
    bool m_showHidden = false;
    QString m_sortBy = QStringLiteral("name");
    QString m_operationStatus;
    QFutureWatcher<QString> m_transfer;
    QFileSystemWatcher m_watcher;
    bool m_busy = false;
    QFutureWatcher<QStringList> m_clipboardReader;
    QList<QUrl> m_clipboardUrls;
    bool m_clipboardCut = false;
    bool m_externalClipboard = false;
};

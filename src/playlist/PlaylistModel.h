#pragma once

#include <QAbstractListModel>

class PlaylistModel final : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { PathRole = Qt::UserRole + 1 };
    explicit PlaylistModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString pathAt(int row) const;
    QStringList paths() const { return m_paths; }
    int currentRow() const { return m_current; }
    void addFiles(const QStringList &files);
    void removeRowAt(int row);
    void clear();
    void setCurrentRow(int row);
    int nextRow() const;
    int previousRow() const;
private:
    QStringList m_paths;
    int m_current = -1;
};

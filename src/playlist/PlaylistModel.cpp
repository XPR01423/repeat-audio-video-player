#include "PlaylistModel.h"
#include <QFileInfo>

PlaylistModel::PlaylistModel(QObject *parent) : QAbstractListModel(parent) {}
int PlaylistModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : m_paths.size(); }
QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_paths.size()) return {};
    const QString &path = m_paths.at(index.row());
    if (role == Qt::DisplayRole) {
        const QString title = QFileInfo(path).fileName();
        return index.row() == m_current ? QStringLiteral("▶  %1").arg(title) : title;
    }
    if (role == PathRole) return path;
    if (role == Qt::ToolTipRole) return path;
    return {};
}
QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames(); roles[PathRole] = "path"; return roles;
}
QString PlaylistModel::pathAt(int row) const { return row >= 0 && row < m_paths.size() ? m_paths.at(row) : QString(); }
void PlaylistModel::addFiles(const QStringList &files)
{
    QStringList accepted;
    for (const QString &f : files) if (!f.isEmpty() && !m_paths.contains(f)) accepted << f;
    if (accepted.isEmpty()) return;
    const int first = m_paths.size();
    beginInsertRows({}, first, first + accepted.size() - 1); m_paths.append(accepted); endInsertRows();
    if (m_current < 0) setCurrentRow(0);
}
void PlaylistModel::removeRowAt(int row)
{
    if (row < 0 || row >= m_paths.size()) return;
    beginRemoveRows({}, row, row); m_paths.removeAt(row); endRemoveRows();
    if (m_paths.isEmpty()) m_current = -1;
    else if (m_current == row) m_current = qMin(row, m_paths.size() - 1);
    else if (m_current > row) --m_current;
    if (!m_paths.isEmpty()) emit dataChanged(index(0), index(m_paths.size() - 1), {Qt::DisplayRole});
}
void PlaylistModel::clear()
{
    if (m_paths.isEmpty()) return;
    beginResetModel(); m_paths.clear(); m_current = -1; endResetModel();
}
void PlaylistModel::setCurrentRow(int row)
{
    if (row < 0 || row >= m_paths.size() || row == m_current) return;
    const int old = m_current; m_current = row;
    if (old >= 0) emit dataChanged(index(old), index(old), {Qt::DisplayRole});
    emit dataChanged(index(row), index(row), {Qt::DisplayRole});
}
int PlaylistModel::nextRow() const { return m_paths.isEmpty() ? -1 : (m_current + 1) % m_paths.size(); }
int PlaylistModel::previousRow() const { return m_paths.isEmpty() ? -1 : (m_current <= 0 ? m_paths.size() - 1 : m_current - 1); }

#include "DragStringListModel.h"

DragStringListModel::DragStringListModel(const QStringList& strs, QObject* parent)
    :
    QStringListModel(strs, parent)
{
}

Qt::ItemFlags DragStringListModel::flags(const QModelIndex& idx) const
{
    Qt::ItemFlags flags; //= QStringListModel::flags(index);

    if (idx.isValid()) {
        flags =  Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
    }
    else {
        flags =  Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled  | Qt::ItemIsEnabled;
    }

    return flags;
}

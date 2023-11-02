#include <QtWidgets>

class DragStringListModel : public QStringListModel
{
public:
    DragStringListModel(const QStringList& strs, QObject* parent = nullptr);
    Qt::ItemFlags flags(const QModelIndex& idx) const;
};

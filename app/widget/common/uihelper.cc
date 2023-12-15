#include "uihelper.h"

QGridLayout* uihelper::InitDialogGridLayout(const QList<QPair<QWidget*, QWidget*>>& widget_pair_list)
{
    auto grid_layout = new QGridLayout;
    grid_layout->setContentsMargins(12, 8, 12, 8);

    int row = 0;
    for (auto pair : widget_pair_list) {
        int rol = 0;
        grid_layout->addWidget(pair.first, row, rol++, Qt::AlignLeft);
        grid_layout->addWidget(pair.second, row++, rol);
    }

    return grid_layout;
}

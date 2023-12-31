#ifndef UIHELPER_H_
#define UIHELPER_H_

#include "fast_layout.h"

namespace uihelper {
QGridLayout* InitDialogGridLayout(const QList<QPair<QWidget*, QWidget*>>& widget_pair_list);

}; // namespace uihelper

#endif

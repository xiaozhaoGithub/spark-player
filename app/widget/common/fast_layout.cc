#include "fast_layout.h"

CHBoxLayout::CHBoxLayout(QWidget* parent /*=nullptr*/, int margin /*=0*/, int spacing /*=0*/)
    : QHBoxLayout(parent)
{
    setMargin(margin);
    setSpacing(spacing);
}

CVBoxLayout::CVBoxLayout(QWidget* parent /*=nullptr*/, int margin /*=0*/, int spacing /*=0*/)
    : QVBoxLayout(parent)
{
    setMargin(margin);
    setSpacing(spacing);
}

CGridLayout::CGridLayout(QWidget* parent /*=nullptr*/, int margin /*=0*/, int spacing /*=0*/)
    : QGridLayout(parent)
{
    setMargin(margin);
    setSpacing(spacing);
}

#include "dndwidget.h"
#include <QMouseEvent>
#include <QLayout>
#include <QVBoxLayout>

#include <algorithm>

DnDWidget::DnDWidget(QWidget* parent):
    QWidget (parent)
{

}

DnDWidget::~DnDWidget()
{
}

QLayout *DnDWidget::getParentLayout()
{
    auto ptr = qobject_cast<QWidget*>(parent());
    if (ptr)
    {
        return ptr->layout();
    }

    return nullptr;
}

void DnDWidget::moveVerticalToNewPos(const QPoint &pos)
{
    int dest = pos.y();
    int y = oldPos.y();
    int sign = (dest > y)?1:-1;
    int h = height();

    auto vl = qobject_cast<QVBoxLayout*>(getParentLayout());
    if (vl)
    {
        if (std::abs(dest - y ) > h)
        {
            int index = vl->indexOf(this);
            while (true)
            {
                if (y >= dest && y <= dest + h)
                    break;

                index += sign;
                y     += sign * h;

                if (index < 0 || index >= vl->count())
                    break;

                vl->removeWidget(this);
                vl->insertWidget(index , this);
            }
        }
        vl->update();
    }
}

QPoint DnDWidget::getCurrentPos()
{
    return QPoint(geometry().x(), geometry().y());
}

void DnDWidget::mousePressEvent(QMouseEvent *event)
{
    clickOffset = event->pos();
    oldPos = getCurrentPos();
}

void DnDWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton)
    {
        if (std::abs(clickOffset.y() - event->pos().y()) > 15 || hadDrag) //antishake
        {
            hadDrag = true;
            this->move(mapToParent(event->pos() - clickOffset));
        }
    }
}

void DnDWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (hadDrag)
    {
        hadDrag = false;
        moveVerticalToNewPos(getCurrentPos());
    }
}

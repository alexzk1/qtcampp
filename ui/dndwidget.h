#ifndef DNDWIDGET_H
#define DNDWIDGET_H

#include <QObject>
#include <QWidget>
#include <QPoint>

//it is movable by mouse widget

class DnDWidget : public QWidget
{
    Q_OBJECT
public:
    DnDWidget() = delete;
    explicit DnDWidget(QWidget* parent = nullptr);
    virtual ~DnDWidget();


private:
    QPoint clickOffset;
    QPoint oldPos;
    bool   hadDrag;
    QLayout* getParentLayout();

    void moveVerticalToNewPos(const QPoint& pos);
    QPoint getCurrentPos();
protected:

    void virtual mousePressEvent(QMouseEvent *event) override;
    void virtual mouseMoveEvent(QMouseEvent *event) override;
    void virtual mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // DNDWIDGET_H

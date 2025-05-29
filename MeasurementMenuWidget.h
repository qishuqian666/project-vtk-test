#pragma once

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QEvent>
#include <QMoveEvent>

class MeasurementMenuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MeasurementMenuWidget(QWidget *parent = nullptr);
    void showMenu(const QPoint &position); // 显示菜单
    void hideMenu();                       // 隐藏菜单

protected:
    bool eventFilter(QObject *watched, QEvent *event);

signals:
    void pointMeasureRequested();
    void lineMeasureRequested();
    void triangleMeasureRequested();
    void closeMeasureRequested();

private:
    QPushButton *pointBtn_;
    QPushButton *lineBtn_;
    QPushButton *triangleBtn_;
    QPushButton *closeBtn_;
    QPoint anchorOffset_; // 相对于父窗口的位置偏移

    void updateHighlight(QPushButton *activeBtn);
};

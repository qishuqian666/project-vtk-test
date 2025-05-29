#include "MeasurementMenuWidget.h"

MeasurementMenuWidget::MeasurementMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    if (parent)
        parent->installEventFilter(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool); // 无边框悬浮窗
    setAttribute(Qt::WA_ShowWithoutActivating);         // 不抢焦点
    setStyleSheet("QPushButton { min-width: 80px; }");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    pointBtn_ = new QPushButton("dian");
    lineBtn_ = new QPushButton("xian");
    triangleBtn_ = new QPushButton("mian");
    closeBtn_ = new QPushButton("close");

    layout->addWidget(pointBtn_);
    layout->addWidget(lineBtn_);
    layout->addWidget(triangleBtn_);
    layout->addWidget(closeBtn_);

    connect(pointBtn_, &QPushButton::clicked, this, [=]()
            {
        emit pointMeasureRequested();
        updateHighlight(pointBtn_); });
    connect(lineBtn_, &QPushButton::clicked, this, [=]()
            {
        emit lineMeasureRequested();
        updateHighlight(lineBtn_); });
    connect(triangleBtn_, &QPushButton::clicked, this, [=]()
            {
        emit triangleMeasureRequested();
        updateHighlight(triangleBtn_); });
    connect(closeBtn_, &QPushButton::clicked, this, [=]()
            {
        emit closeMeasureRequested();
        hideMenu(); });
}

void MeasurementMenuWidget::showMenu(const QPoint &position)
{
    if (parentWidget())
        anchorOffset_ = position - parentWidget()->mapToGlobal(QPoint(0, 0));
    move(position);
    show();
}

void MeasurementMenuWidget::hideMenu()
{
    hide();
}

bool MeasurementMenuWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Move)
    {
        if (isVisible())
        {
            QPoint newGlobalPos = parentWidget()->mapToGlobal(QPoint(0, 0)) + anchorOffset_;
            move(newGlobalPos);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MeasurementMenuWidget::updateHighlight(QPushButton *activeBtn)
{
    QList<QPushButton *> buttons = {pointBtn_, lineBtn_, triangleBtn_};
    for (auto btn : buttons)
        btn->setStyleSheet(btn == activeBtn ? "background-color: lightblue;" : "");
}

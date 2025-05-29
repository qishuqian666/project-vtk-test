#pragma once

#include <QObject>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTextActor.h>
#include <vtkActor.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>
#include <vtkCaptionActor2D.h>
#include <vector>
#include <array>

enum class MeasurementMode
{
    None,
    Point,
    Line,
    Triangle
};

// 测量控制器类，负责处理点、线、面的选择和测量显示
class MeasurementController : public QObject
{
    Q_OBJECT

public:
    MeasurementController(vtkRenderer *renderer, vtkRenderWindowInteractor *interactor);
    void setMode(MeasurementMode mode); // 设置当前测量模式
    void clearMeasurements();           // 清除当前所有测量
    void onLeftButtonPressed();         // 鼠标左键点击事件响应（需外部连接）
    // 重新添加文本框到场景中
    void ReAddActorsToRenderer();

private:
    void addPointMarker(const double pos[3]);                                         // 添加球体标记点
    void updateMeasurementDisplay();                                                  // 根据点数更新测量图形与文字
    void renderLine(const double p1[3], const double p2[3]);                          // 渲染一条直线
    void renderTriangle(const double p1[3], const double p2[3], const double p3[3]);  // 渲染一个三角形
    QString formatDistanceInfo(const double *p1, const double *p2);                   // 格式化距离测量文本
    QString formatTriangleInfo(const double *p1, const double *p2, const double *p3); // 格式化三角形测量文本
    double computeDistance(const double *p1, const double *p2);                       // 计算两点距离
    double computeArea(const double *A, const double *B, const double *C);            // 计算三角形面积
    double computeAngleDeg(const double *A, const double *B, const double *C);        // 计算角度
    void render();                                                                    // 请求刷新渲染窗口
    void updateTextActor();                                                           // 更新文本信息框
    // 清除所有标记点和测量图形
    void clearAllMarkers();

    vtkRenderer *renderer_;
    vtkRenderWindowInteractor *interactor_;
    MeasurementMode mode_ = MeasurementMode::None;
    std::vector<std::array<double, 3>> pickedPoints_;     // 已选的测量点
    std::vector<vtkSmartPointer<vtkActor>> pointMarkers_; // 所有绘制的 actor（点、线）
    vtkSmartPointer<vtkTextActor> textActor_;             // 文本显示 actor
};

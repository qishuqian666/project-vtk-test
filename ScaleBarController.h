#ifndef SCALEBARCONTROLLER_H
#define SCALEBARCONTROLLER_H

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTextActor.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkActor2D.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCallbackCommand.h>

class ScaleBarController
{
public:
    ScaleBarController(vtkSmartPointer<vtkRenderer> renderer,
                       vtkSmartPointer<vtkRenderWindow> renderWindow,
                       vtkSmartPointer<vtkRenderWindowInteractor> interactor);

    void UpdateScaleBar(); // 根据当前缩放更新比例尺
    void ReAddToRenderer();

private:
    static void OnInteractionEvent(vtkObject *caller, unsigned long eid,
                                   void *clientdata, void *calldata);

    void CreateScaleBarActors();
    double GetCurrentScaleFactor();             // 获取 world units / pixel 比例
    double ComputeRoundedLength(double length); // 跳变策略

    vtkSmartPointer<vtkRenderer> renderer_;
    vtkSmartPointer<vtkRenderWindow> renderWindow_;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor_;

    vtkSmartPointer<vtkLineSource> lineSource_;
    vtkSmartPointer<vtkPolyDataMapper2D> lineMapper_;
    vtkSmartPointer<vtkActor2D> lineActor_;

    vtkSmartPointer<vtkTextActor> scaleText_;
    vtkSmartPointer<vtkCallbackCommand> interactionCallback_;

    const int pixelLength_ = 200;         // 比例尺显示长度（像素）
    double lastValidParallelScale_ = 1.0; // 上一次合法的 ParallelScale 值
    double lastValidCameraDistance_ = 1.0;
};

#endif // SCALEBARCONTROLLER_H

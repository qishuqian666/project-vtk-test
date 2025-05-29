/* 三维地形数据显示 */
#ifndef CDS_FRONTEND_THREE_DIMENSIONAL_DISPLAY_PAGE_H__
#define CDS_FRONTEND_THREE_DIMENSIONAL_DISPLAY_PAGE_H__
#include "ScaleBarController.h"
#include "MeshSliceController.h"
#include "BoxClipperController.h"
#include "ModelPinelineBuilder.h"
#include "MeasurementController.h"
#include "MeasurementMenuWidget.h"
// #include "OverlayLineRenderer.h"

#include <QWidget>
#include <QEvent>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVTKOpenGLWidget.h>
#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataMapper.h>
#include <vtkTextActor.h>
#include <vtkAxesActor.h>
#include <vtkLookupTable.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkTextMapper.h>
#include <vtkScalarBarActor.h>
#include <vtkColorTransferFunction.h>

class ThreeDimensionalDisplayPage : public QWidget
{
    Q_OBJECT

public:
    explicit ThreeDimensionalDisplayPage(QWidget *parent = nullptr);
    ~ThreeDimensionalDisplayPage();

private:
    // 初始化读取文件路径控件
    void initSelectFilePath();
    // 初始化控制按钮
    void initControlBtn();
    // 初始化测量菜单
    void initMeasurementMenu();
    // vtk加载文件
    void loadModelByExtension(const QString &filePath);
    // 加载坐标轴
    void addCoordinateAxes();
    // 标量颜色图例
    void addScalarColorLegend();
    // 添加addBoundingBox
    void addBoundingBox(vtkSmartPointer<vtkPolyData> polyData);
    // 点击按钮之后的槽函数
    void OnBoundingBoxButtonClicked();
    // 控制面显示槽函数
    void toggleSurfaceVisibility();
    // 控制线显示槽函数
    void toggleWireframeVisibility();
    // 控制点显示槽函数
    void togglePointsVisibility();
    // 设置点大小
    void setPointSize();
    // 渲染数据为jet风格
    vtkSmartPointer<vtkLookupTable> createJetLookupTable(double minValue, double maxValue, double gamma = 0.7);
    vtkSmartPointer<vtkLookupTable> createViridisLookupTable(double minValue, double maxValue);
    vtkSmartPointer<vtkLookupTable> createCoolToWarmLookupTable(double minValue, double maxValue);
    vtkSmartPointer<vtkLookupTable> createGrayscaleLookupTable(double minValue, double maxValue);
    vtkSmartPointer<vtkLookupTable> createRainbowLookupTable(double minValue, double maxValue);
    void updateColorStyle(int style); // 颜色风格切换函数
    // 设置Z轴拉伸
    void setZAxisStretching();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void SlotFileSelectBtnClicked();
    // 用于测试渲染效果
    void updateLUTWithGamma(double gamma);
    // 点击箱体切割器按钮槽函数
    void SlotCilckedCrossSectionBtn();

private:
    QVBoxLayout *main_layout_;  // 主布局
    QLineEdit *file_path_edit_; // 文件路径

    std::unique_ptr<ModelPipelineBuilder> model_pinpeline_builder_; // 模型构建器;

    // 显示场景
    QVTKOpenGLWidget *m_pScene;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow_;
    vtkSmartPointer<vtkRenderer> renderer_;
    // 创建交互器
    vtkSmartPointer<vtkRenderWindowInteractor> interactor_;
    // 边框显示
    vtkSmartPointer<vtkActor> boundingBoxActor_; // 用于存储BoundingBox的Actor
    bool isBoundingBoxVisible_;                  // 控制BoundingBox的显隐状态
    QPushButton *bounding_box_control_btn_;
    //  坐标轴显示
    vtkSmartPointer<vtkAxesActor> axes_;
    vtkSmartPointer<vtkOrientationMarkerWidget> marker_;
    // 控制ply点大小
    QLineEdit *point_size_edit_;
    vtkSmartPointer<vtkPolyData> ply_poly_data_;
    vtkSmartPointer<vtkActor> ply_point_actor_; // 点云Actor
    // 控制obj点线面的显隐
    vtkSmartPointer<vtkActor> surfaceActor_;
    vtkSmartPointer<vtkActor> wireframeActor_;
    vtkSmartPointer<vtkActor> pointsActor_;
    // 控制obj点线面显隐的按钮
    QPushButton *surfaceToggleButton_;
    QPushButton *wireframeToggleButton_;
    QPushButton *pointsToggleButton_;
    bool is_surface_visible_;       // 控制面的显隐状态
    bool is_wireframe_visible_;     // 控制边的显隐状态
    bool is_points_visible_;        // 控制点的显隐状态
    int current_color_style;        // 0:Jet, 1:Viridis, 2:CoolWarm, 3:Grayscale, 4:Rainbow
    double current_scalar_range[2]; // 保存当前标量范围
    // 添加颜色图例
    vtkNew<vtkScalarBarActor> scalarBar;
    vtkNew<vtkColorTransferFunction> colorTF;
    // 比例尺
    std::unique_ptr<ScaleBarController> scaleBarController_;
    // 切面图
    std::unique_ptr<MeshSliceController> meshSliceController_;
    // 箱形剪控制器
    std::unique_ptr<BoxClipperController> boxClipper_;
    bool boxClipper_enabled_;
    // z轴拉伸
    QLineEdit *zaxis_stretching_edit_;
    // vtkSmartPointer<vtkTransform> zaxis_transform_;
    //  测量功能
    MeasurementMenuWidget *measurementMenuWidget_;
    std::unique_ptr<MeasurementController> measurementController_;
    QPushButton *measurement_btn_;
    // std::unique_ptr<OverlayLineRenderer> overlayLineRenderer_;
};

#endif

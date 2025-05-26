#include "ThreeDViewWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <qDebug>
#include <QDoubleValidator>
#include <QSlider>
#include <iostream>
#include <sstream>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCylinderSource.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkElevationFilter.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <array>
#include <vtkPLYReader.h>
#include <vtkOBJReader.h>
#include <vtkTIFFReader.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkTransform.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkFeatureEdges.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkButtonWidget.h>
#include <vtkTextProperty.h>
#include <vtkTextMapper.h>
#include <vtkActor2D.h>
#include <vtkTextRepresentation.h>
#include <vtkTexturedButtonRepresentation2D.h>
#include <vtkCallbackCommand.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkTransformFilter.h>
#include <vtkCubeSource.h>

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);
VTK_MODULE_INIT(vtkRenderingContextOpenGL2);

// 定义静态回调函数
// static void ScaleBarInteractionCallback(vtkObject *caller, unsigned long eventId, void *clientData, void *callData)
// {
//     ScaleBarController *scaleBar = static_cast<ScaleBarController *>(clientData);
//     if (scaleBar)
//     {
//         scaleBar->onInteraction();
//     }
// }

ThreeDimensionalDisplayPage::ThreeDimensionalDisplayPage(QWidget *parent)
{
    /**
     * @brief 设置widget属性和布局
     *
     */
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground);
    setObjectName("ThreeDimensionalDisplayPage");

    isBoundingBoxVisible_ = true;
    is_surface_visible_ = true;
    is_wireframe_visible_ = true;
    is_points_visible_ = true;
    current_color_style = 0;       // 0:Jet, 1:Viridis, 2:CoolWarm, 3:Grayscale, 4:Rainbow
    current_scalar_range[0] = 0.0; // 最小值
    current_scalar_range[1] = 1.0; // 最大值

    model_pinpeline_builder_ = std::make_unique<ModelPipelineBuilder>();

    main_layout_ = new QVBoxLayout();
    this->setLayout(main_layout_);
    main_layout_->setContentsMargins(20, 20, 20, 20);
    main_layout_->setSpacing(5);
    setLayout(main_layout_);
    setMinimumSize(800, 600);

    m_pScene = new QVTKOpenGLWidget();
    main_layout_->addWidget(m_pScene);

    // ------------------------------
    // 用于测试渲染是否正常：Cylinder
    // ------------------------------
    vtkNew<vtkCylinderSource> cylinderSource;
    cylinderSource->SetHeight(10.0);
    cylinderSource->SetRadius(5.0);
    cylinderSource->SetResolution(50);
    cylinderSource->Update();

    vtkNew<vtkPolyDataMapper> testMapper;
    testMapper->SetInputConnection(cylinderSource->GetOutputPort());

    vtkNew<vtkActor> testActor;
    testActor->SetMapper(testMapper);
    testActor->GetProperty()->SetColor(1.0, 0.0, 0.0); // 红色

    vtkNew<vtkNamedColors> colors;
    // 创建渲染器和渲染窗口
    renderer_ = vtkSmartPointer<vtkRenderer>::New();
    renderer_->SetBackground(colors->GetColor3d("DarkSlateGray").GetData());
    renderer_->AddActor(testActor);
    renderer_->ResetCamera();
    renderWindow_ = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    renderWindow_->AddRenderer(renderer_);
    m_pScene->SetRenderWindow(renderWindow_);
    interactor_ = m_pScene->GetInteractor();
    renderWindow_->SetInteractor(interactor_);
    addCoordinateAxes();

    // 创建比例尺控制器
    scaleBarController_ = std::make_unique<ScaleBarController>(renderer_, renderWindow_, interactor_);
    // 启动交互器
    interactor_->Initialize();
    // 切面图
    meshSliceController_ = std::make_unique<MeshSliceController>(renderer_);
    meshSliceController_->SetOriginalActor(testActor);
    meshSliceController_->UpdatePolyData(cylinderSource->GetOutput()); // 传入最终用于渲染的 polydata

    boxClipper_ = std::make_unique<BoxClipperController>(interactor_, renderer_);
    boxClipper_enabled_ = false;
    boxClipper_->SetInputDataAndReplaceOriginal(cylinderSource->GetOutput(), testActor);
    initSelectFilePath();
    initControlBtn();
    renderWindow_->Render();
}

ThreeDimensionalDisplayPage::~ThreeDimensionalDisplayPage()
{
}

void ThreeDimensionalDisplayPage::initSelectFilePath()
{
    QHBoxLayout *select_file_path_layout = new QHBoxLayout();
    select_file_path_layout->setContentsMargins(10, 10, 10, 10);
    select_file_path_layout->setSpacing(20);
    QLabel *select_file_path_label = new QLabel("Select file path:"); // 原：请选择读取文件路径:
    select_file_path_layout->addWidget(select_file_path_label);

    QPushButton *file_select_button = new QPushButton("Select");
    file_select_button->setObjectName("file_select_button");
    file_select_button->setToolTip("Select file path"); // 原：选择文件路径
    select_file_path_layout->addWidget(file_select_button);

    file_path_edit_ = new QLineEdit();
    file_path_edit_->setPlaceholderText("Select file to load"); // 原：请选择加载文件路径
    select_file_path_layout->addWidget(file_path_edit_);
    main_layout_->addLayout(select_file_path_layout);

    connect(file_select_button, &QPushButton::clicked, this, &ThreeDimensionalDisplayPage::SlotFileSelectBtnClicked);
}

void ThreeDimensionalDisplayPage::initControlBtn()
{
    QHBoxLayout *control_btn_layout = new QHBoxLayout();
    main_layout_->addLayout(control_btn_layout);
    QLabel *point_size_label = new QLabel("Set Point Size"); // 原：设置点大小1
    control_btn_layout->addWidget(point_size_label);
    point_size_edit_ = new QLineEdit();
    point_size_edit_->setText("1.0");
    // 限制只能输入 0.1 到 100.0 之间的小数（支持最多两位小数）
    auto *validator = new QDoubleValidator(0.1, 100.0, 2, point_size_edit_);
    validator->setNotation(QDoubleValidator::StandardNotation);
    point_size_edit_->setFixedWidth(100);
    point_size_edit_->setValidator(validator);
    control_btn_layout->addWidget(point_size_edit_);
    QPushButton *point_size_btn_ = new QPushButton("OK"); // 原：确定
    control_btn_layout->addWidget(point_size_btn_);

    bounding_box_control_btn_ = new QPushButton("Hide Bounding Box"); // 原：隐藏边框
    control_btn_layout->addWidget(bounding_box_control_btn_);
    surfaceToggleButton_ = new QPushButton("Hide Surface"); // 原：隐藏面
    control_btn_layout->addWidget(surfaceToggleButton_);
    wireframeToggleButton_ = new QPushButton("Hide Wireframe"); // 原：隐藏边
    control_btn_layout->addWidget(wireframeToggleButton_);
    pointsToggleButton_ = new QPushButton("Hide Points"); // 原：隐藏点
    control_btn_layout->addWidget(pointsToggleButton_);

    // 用于测试渲染效果 设置渲染颜色范围滑块
    // 创建滑块控制 gamma
    // QSlider *gamma_slider = new QSlider(Qt::Horizontal);
    // gamma_slider->setRange(10, 100); // 映射到 0.1 ~ 1.0
    // gamma_slider->setValue(70);      // 初始值为 0.7
    // connect(gamma_slider, &QSlider::valueChanged, this, [this](int value)
    //         {
    // double gamma = value / 100.0;
    // updateLUTWithGamma(gamma); });
    // control_btn_layout->addWidget(new QLabel("Gamma (颜色分布)"));
    // control_btn_layout->addWidget(gamma_slider);

    // 新增颜色风格按钮组
    QLabel *color_style_label = new QLabel("Color Style:");
    control_btn_layout->addWidget(color_style_label);

    QPushButton *jet_btn = new QPushButton("Jet");
    control_btn_layout->addWidget(jet_btn);
    QPushButton *viridis_btn = new QPushButton("Viridis");
    control_btn_layout->addWidget(viridis_btn);
    QPushButton *coolwarm_btn = new QPushButton("CoolWarm");
    control_btn_layout->addWidget(coolwarm_btn);
    QPushButton *grayscale_btn = new QPushButton("Grayscale");
    control_btn_layout->addWidget(grayscale_btn);
    QPushButton *rainbow_btn = new QPushButton("Rainbow");
    control_btn_layout->addWidget(rainbow_btn);

    // 第二排按钮 控制切面图显示
    QHBoxLayout *control_btn_layout_2 = new QHBoxLayout();
    main_layout_->addLayout(control_btn_layout_2);
    QPushButton *btnSliceX = new QPushButton("SliceX");
    control_btn_layout_2->addWidget(btnSliceX);
    QPushButton *btnSliceY = new QPushButton("btnSliceY");
    control_btn_layout_2->addWidget(btnSliceY);
    QPushButton *btnSliceZ = new QPushButton("btnSliceZ");
    control_btn_layout_2->addWidget(btnSliceZ);
    QPushButton *hideSlice = new QPushButton("hideSlice");
    control_btn_layout_2->addWidget(hideSlice);
    QPushButton *cross_section = new QPushButton("cross section");
    control_btn_layout_2->addWidget(cross_section);

    // z轴拉伸
    QLabel *zaxis_stretching_label = new QLabel("Z-axis stretching"); // 原：设置点大小1
    control_btn_layout_2->addWidget(zaxis_stretching_label);
    zaxis_stretching_edit_ = new QLineEdit();
    zaxis_stretching_edit_->setText("1.0");
    // 限制只能输入 0.1 到 100.0 之间的小数（支持最多两位小数）
    zaxis_stretching_edit_->setFixedWidth(100);
    zaxis_stretching_edit_->setValidator(validator);
    control_btn_layout_2->addWidget(zaxis_stretching_edit_);
    QPushButton *zaxis_stretching_btn_ = new QPushButton("OK"); // 原：确定
    control_btn_layout_2->addWidget(zaxis_stretching_btn_);

    connect(btnSliceX, &QPushButton::clicked, this, [=]()
            { meshSliceController_->ShowSlice(SLICE_X); renderWindow_->Render(); });

    connect(btnSliceY, &QPushButton::clicked, this, [=]()
            { meshSliceController_->ShowSlice(SLICE_Y); renderWindow_->Render(); });

    connect(btnSliceZ, &QPushButton::clicked, this, [=]()
            { meshSliceController_->ShowSlice(SLICE_Z); renderWindow_->Render(); });

    connect(hideSlice, &QPushButton::clicked, this, [=]()
            {
    if (meshSliceController_)
        meshSliceController_->HideSlice(); 
        renderWindow_->Render(); });

    connect(cross_section, &QPushButton::clicked, this, &ThreeDimensionalDisplayPage::SlotCilckedCrossSectionBtn);

    // 连接按钮点击信号到颜色切换槽函数
    connect(jet_btn, &QPushButton::clicked, this, [this]()
            { updateColorStyle(0); });
    connect(viridis_btn, &QPushButton::clicked, this, [this]()
            { updateColorStyle(1); });
    connect(coolwarm_btn, &QPushButton::clicked, this, [this]()
            { updateColorStyle(2); });
    connect(grayscale_btn, &QPushButton::clicked, this, [this]()
            { updateColorStyle(3); });
    connect(rainbow_btn, &QPushButton::clicked, this, [this]()
            { updateColorStyle(4); });

    connect(bounding_box_control_btn_, &QPushButton::clicked, this, &ThreeDimensionalDisplayPage::OnBoundingBoxButtonClicked);
    connect(surfaceToggleButton_, &QPushButton::clicked, this, &ThreeDimensionalDisplayPage::toggleSurfaceVisibility);
    connect(wireframeToggleButton_, &QPushButton::clicked, this, &ThreeDimensionalDisplayPage::toggleWireframeVisibility);
    connect(pointsToggleButton_, &QPushButton::clicked, this, &ThreeDimensionalDisplayPage::togglePointsVisibility);
    connect(point_size_btn_, &QPushButton::clicked, this, &ThreeDimensionalDisplayPage::setPointSize);
    connect(zaxis_stretching_btn_, &QPushButton::clicked, this, &ThreeDimensionalDisplayPage::setZAxisStretching);
}

void ThreeDimensionalDisplayPage::loadModelByExtension(const QString &filePath)
{
    if (!QFileInfo::exists(filePath))
    {
        qDebug() << "File does not exist: " << filePath;
        return;
    }
    if (!model_pinpeline_builder_->loadModel(filePath))
    {
        qDebug() << "Failed to load model: " << filePath;
        return;
    }

    renderer_->RemoveAllViewProps();
    renderer_->SetBackground(0.5, 0.5, 0.5); // 可选：统一背景色
    if (model_pinpeline_builder_->getModelType() == ModelPipelineBuilder::ModelType::PLY)
    {
        ply_point_actor_ = model_pinpeline_builder_->getActor();
        renderer_->AddActor(ply_point_actor_);
        // 设置切面控制器
        meshSliceController_->SetOriginalActor(ply_point_actor_);
        meshSliceController_->UpdatePolyData(model_pinpeline_builder_->getProcessedPolyData());
        // BoxClipper 设置
        boxClipper_->SetInputDataAndReplaceOriginal(model_pinpeline_builder_->getProcessedPolyData(),
                                                    ply_point_actor_);
    }
    else if (model_pinpeline_builder_->getModelType() == ModelPipelineBuilder::ModelType::OBJ)
    {
        // 添加 OBJ 专属的线框、点、面 actor
        surfaceActor_ = model_pinpeline_builder_->getSurfaceActor();
        wireframeActor_ = model_pinpeline_builder_->getWireframeActor();
        pointsActor_ = model_pinpeline_builder_->getPointsActor();

        if (surfaceActor_)
        {
            surfaceActor_->SetVisibility(is_surface_visible_);
            renderer_->AddActor(surfaceActor_);
        }
        if (wireframeActor_)
        {
            wireframeActor_->SetVisibility(is_wireframe_visible_);
            renderer_->AddActor(wireframeActor_);
        }
        if (pointsActor_)
        {
            pointsActor_->SetVisibility(is_points_visible_);
            renderer_->AddActor(pointsActor_);
        }
        meshSliceController_->SetOriginalActor(surfaceActor_);
        meshSliceController_->UpdatePolyData(model_pinpeline_builder_->getProcessedPolyData());
        // BoxClipper 设置
        boxClipper_->SetInputDataAndReplaceOriginal(model_pinpeline_builder_->getProcessedPolyData(),
                                                    surfaceActor_);
    }
    // 1. 添加 BoundingBox
    addBoundingBox(model_pinpeline_builder_->getProcessedPolyData());
    boxClipper_enabled_ = false;

    // 4. 重设相机、刷新渲染器
    renderer_->ResetCamera();
    renderWindow_->Render();

    // 5. 比例尺处理
    if (scaleBarController_)
    {
        scaleBarController_->ReAddToRenderer();
        scaleBarController_->UpdateScaleBar(); // 主动触发更新比例尺显示
    }

    m_pScene->update(); // 最后刷新界面
}

void ThreeDimensionalDisplayPage::addCoordinateAxes()
{
    if (!axes_)
        axes_ = vtkSmartPointer<vtkAxesActor>::New();
    axes_->SetTotalLength(10.0, 10.0, 10.0);

    if (!marker_)
        marker_ = vtkSmartPointer<vtkOrientationMarkerWidget>::New();

    marker_->SetOrientationMarker(axes_);
    marker_->SetOutlineColor(0.9300, 0.5700, 0.1300);
    marker_->SetInteractor(m_pScene->GetInteractor());
    marker_->SetDefaultRenderer(renderer_);
    marker_->SetViewport(0.8, 0.0, 1.0, 0.2);
    marker_->SetEnabled(1);
    marker_->InteractiveOff(); // 坐标轴 不可拖动（禁用交互）
}

void ThreeDimensionalDisplayPage::addScalarColorLegend()
{
    colorTF->SetColorSpaceToHSV();
    colorTF->AddRGBPoint(0.0, 0.0, 0.0, 1.0); // 蓝色
    colorTF->AddRGBPoint(0.5, 0.0, 1.0, 0.0); // 绿色
    colorTF->AddRGBPoint(1.0, 1.0, 0.0, 0.0); // 红色

    scalarBar->SetLookupTable(colorTF);
    scalarBar->SetTitle("Scale");
    scalarBar->UnconstrainedFontSizeOn();
    scalarBar->SetNumberOfLabels(5);
    scalarBar->SetPosition(0.6, 0.05); // 调整位置到右下角坐标轴左侧
    scalarBar->SetWidth(0.15);
    scalarBar->SetHeight(0.2);

    // 设置比例尺文字和刻度线为黑色
    vtkTextProperty *textProp = scalarBar->GetLabelTextProperty();
    textProp->SetColor(0.0, 0.0, 0.0); // 黑色
    textProp->SetFontSize(12);
    textProp->BoldOff();
    textProp->ItalicOff();
    textProp->ShadowOff();

    vtkTextProperty *titleProp = scalarBar->GetTitleTextProperty();
    titleProp->SetColor(0.0, 0.0, 0.0); // 黑色
    titleProp->SetFontSize(14);
    titleProp->BoldOff();
    titleProp->ItalicOff();
    titleProp->ShadowOff();

    // 设置刻度线颜色为黑色
    scalarBar->SetDrawTickLabels(1);
    scalarBar->GetLabelTextProperty()->SetColor(0.0, 0.0, 0.0);

    renderer_->AddActor2D(scalarBar);
}

void ThreeDimensionalDisplayPage::addBoundingBox(vtkSmartPointer<vtkPolyData> polyData)
{
    if (boundingBoxActor_)
    {
        renderer_->RemoveActor(boundingBoxActor_);
        boundingBoxActor_ = nullptr;
    }

    vtkNew<vtkOutlineFilter> outlineFilter;
    outlineFilter->SetInputData(polyData);
    outlineFilter->Update();

    vtkNew<vtkPolyDataMapper> outlineMapper;
    outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());

    boundingBoxActor_ = vtkSmartPointer<vtkActor>::New();
    boundingBoxActor_->SetMapper(outlineMapper);
    boundingBoxActor_->GetProperty()->SetColor(1.0, 0.0, 0.0);
    boundingBoxActor_->GetProperty()->SetLineWidth(2.0);
    boundingBoxActor_->SetVisibility(isBoundingBoxVisible_); // 根据状态决定是否显示

    renderer_->AddActor(boundingBoxActor_);
}

void ThreeDimensionalDisplayPage::OnBoundingBoxButtonClicked()
{
    if (!boundingBoxActor_)
        return;

    isBoundingBoxVisible_ = !isBoundingBoxVisible_;
    boundingBoxActor_->SetVisibility(isBoundingBoxVisible_);

    if (bounding_box_control_btn_)
    {
        bounding_box_control_btn_->setText(isBoundingBoxVisible_ ? "Hide Bounding Box" : "Show Bounding Box"); // 原：隐藏边框/显示边框
    }

    renderWindow_->Render();
}

void ThreeDimensionalDisplayPage::toggleSurfaceVisibility()
{
    if (surfaceActor_)
    {
        is_surface_visible_ = !is_surface_visible_;
        surfaceActor_->SetVisibility(is_surface_visible_);
        surfaceToggleButton_->setText(is_surface_visible_ ? "Hide Surface" : "Show Surface"); // 原：隐藏面/显示面
        renderWindow_->Render();
    }
}

void ThreeDimensionalDisplayPage::toggleWireframeVisibility()
{
    if (wireframeActor_)
    {
        is_wireframe_visible_ = !is_wireframe_visible_;
        wireframeActor_->SetVisibility(is_wireframe_visible_);
        wireframeToggleButton_->setText(is_wireframe_visible_ ? "Hide Wireframe" : "Show Wireframe"); // 原：隐藏边/显示边
        renderWindow_->Render();
    }
}

void ThreeDimensionalDisplayPage::togglePointsVisibility()
{
    if (pointsActor_)
    {
        is_points_visible_ = !is_points_visible_;
        pointsActor_->SetVisibility(is_points_visible_);
        pointsToggleButton_->setText(is_points_visible_ ? "Hide Points" : "Show Points"); // 原：隐藏点/显示点
        renderWindow_->Render();
    }
}

void ThreeDimensionalDisplayPage::setPointSize()
{
    if (model_pinpeline_builder_->getModelType() == ModelPipelineBuilder::ModelType::PLY)
    {
        ply_point_actor_->GetProperty()->SetPointSize(point_size_edit_->text().toInt());
    }
    else if (model_pinpeline_builder_->getModelType() == ModelPipelineBuilder::ModelType::OBJ)
    {
        pointsActor_->GetProperty()->SetPointSize(point_size_edit_->text().toInt());
    }
    renderWindow_->Render();
}

vtkSmartPointer<vtkLookupTable> ThreeDimensionalDisplayPage::createJetLookupTable(double minValue, double maxValue, double gamma)
{
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(256);
    lut->SetRange(minValue, maxValue); // 设置标量范围

    for (int i = 0; i < 256; ++i)
    {
        double t = static_cast<double>(i) / 255.0;

        // 非线性调整 t，使颜色更集中于中间值
        t = std::pow(t, gamma);

        // Jet 配色映射：蓝 → 青 → 绿 → 黄 → 红
        double r = std::clamp(1.5 - std::abs(4.0 * t - 3.0), 0.0, 1.0);
        double g = std::clamp(1.5 - std::abs(4.0 * t - 2.0), 0.0, 1.0);
        double b = std::clamp(1.5 - std::abs(4.0 * t - 1.0), 0.0, 1.0);

        lut->SetTableValue(i, r, g, b);
    }

    lut->Build();
    return lut;
}

void ThreeDimensionalDisplayPage::updateLUTWithGamma(double gamma)
{
    if (!ply_point_actor_)
        return;

    auto mapper = vtkPolyDataMapper::SafeDownCast(ply_point_actor_->GetMapper());
    if (!mapper)
        return;

    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(256);
    lut->SetRange(-50.0, 50.0); // 必须和 ElevationFilter 高低点一致

    for (int i = 0; i < 256; ++i)
    {
        double t = std::pow(i / 255.0, gamma);
        double r = std::clamp(1.5 - std::abs(4.0 * t - 3.0), 0.0, 1.0);
        double g = std::clamp(1.5 - std::abs(4.0 * t - 2.0), 0.0, 1.0);
        double b = std::clamp(1.5 - std::abs(4.0 * t - 1.0), 0.0, 1.0);
        lut->SetTableValue(i, r, g, b);
    }
    lut->Build();

    mapper->SetLookupTable(lut);
    renderWindow_->Render();
}

void ThreeDimensionalDisplayPage::SlotCilckedCrossSectionBtn()
{
    boxClipper_enabled_ = !boxClipper_enabled_;
    // 如果使用箱体切割器默认只显示面 目前只切割了面
    if (boxClipper_enabled_)
    {
        is_surface_visible_ = true;
        surfaceActor_->SetVisibility(is_surface_visible_);
        surfaceToggleButton_->setText("Hide Surface");
        is_wireframe_visible_ = false;
        wireframeActor_->SetVisibility(is_wireframe_visible_);
        wireframeToggleButton_->setText("Show Wireframe");
        is_points_visible_ = false;
        pointsActor_->SetVisibility(is_points_visible_);
        pointsToggleButton_->setText("Show Points");
    }

    boxClipper_->SetEnabled(boxClipper_enabled_);
}

void ThreeDimensionalDisplayPage::SlotFileSelectBtnClicked()
{
    // 定义文件过滤器
    QString filter = "Supported Files (*.ply *.obj);;PLY Files (*.ply);;OBJ Files (*.obj);;All Files (*)";
    QString dir_name = QFileDialog::getOpenFileName(this, ("请选择加载路径"), "", filter);
    if (!dir_name.isEmpty())
    {
        qDebug() << "dir_name:" << dir_name;
        file_path_edit_->setText(dir_name);
        loadModelByExtension(dir_name);
    }
}

vtkSmartPointer<vtkLookupTable> ThreeDimensionalDisplayPage::createViridisLookupTable(double minValue, double maxValue)
{
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(256);
    lut->SetRange(minValue, maxValue);

    for (int i = 0; i < 256; ++i)
    {
        double t = i / 255.0;
        // 近似 Matplotlib Viridis 色带（更精确的RGB值可参考官方定义）
        double r = 0.2795 + 0.4702 * t + 0.1649 * t * t - 0.0008 * t * t * t;
        double g = 0.0021 + 0.7047 * t + 0.0704 * t * t - 0.0053 * t * t * t;
        double b = 0.3904 + 0.1066 * t + 0.1968 * t * t - 0.0641 * t * t * t;
        lut->SetTableValue(i, r, g, b);
    }
    lut->Build();
    return lut;
}

vtkSmartPointer<vtkLookupTable> ThreeDimensionalDisplayPage::createCoolToWarmLookupTable(double minValue, double maxValue)
{
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(256);
    lut->SetRange(minValue, maxValue);

    for (int i = 0; i < 256; ++i)
    {
        double t = i / 255.0;
        double r = t;                     // 红：从0→1
        double g = 1 - fabs(t - 0.5) * 2; // 绿：中间最亮
        double b = 1 - t;                 // 蓝：从1→0
        lut->SetTableValue(i, r, g, b);
    }
    lut->Build();
    return lut;
}

vtkSmartPointer<vtkLookupTable> ThreeDimensionalDisplayPage::createGrayscaleLookupTable(double minValue, double maxValue)
{
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(256);
    lut->SetRange(minValue, maxValue);

    for (int i = 0; i < 256; ++i)
    {
        double gray = i / 255.0; // 从黑→白
        lut->SetTableValue(i, gray, gray, gray);
    }
    lut->Build();
    return lut;
}

vtkSmartPointer<vtkLookupTable> ThreeDimensionalDisplayPage::createRainbowLookupTable(double minValue, double maxValue)
{
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(256);
    lut->SetHueRange(0.666, 0.0); // 色相从蓝(0.666)→红(0.0)
    lut->SetSaturationRange(1.0, 1.0);
    lut->SetValueRange(1.0, 1.0);
    lut->SetRange(minValue, maxValue);
    lut->Build();
    return lut;
}

// 新增槽函数实现颜色更新
void ThreeDimensionalDisplayPage::updateColorStyle(int style)
{
    current_color_style = style;
    if (ply_point_actor_) // PLY点云模型
    {
        auto mapper = vtkPolyDataMapper::SafeDownCast(ply_point_actor_->GetMapper());
        if (mapper)
        {
            vtkSmartPointer<vtkLookupTable> new_lut;
            switch (style)
            {
            case 0:
                new_lut = createJetLookupTable(current_scalar_range[0], current_scalar_range[1], 1.0);
                break;
            case 1:
                new_lut = createViridisLookupTable(current_scalar_range[0], current_scalar_range[1]);
                break;
            case 2:
                new_lut = createCoolToWarmLookupTable(current_scalar_range[0], current_scalar_range[1]);
                break;
            case 3:
                new_lut = createGrayscaleLookupTable(current_scalar_range[0], current_scalar_range[1]);
                break;
            case 4:
                new_lut = createRainbowLookupTable(current_scalar_range[0], current_scalar_range[1]);
                break;
            }
            mapper->SetLookupTable(new_lut);
            renderWindow_->Render();
        }
    }
    if (surfaceActor_) // OBJ网格模型
    {
        auto mapper = vtkPolyDataMapper::SafeDownCast(surfaceActor_->GetMapper());
        if (mapper)
        {
            vtkSmartPointer<vtkLookupTable> new_lut;
            switch (style)
            {
            case 0:
                new_lut = createJetLookupTable(current_scalar_range[0], current_scalar_range[1]);
                break;
            case 1:
                new_lut = createViridisLookupTable(current_scalar_range[0], current_scalar_range[1]);
                break;
            case 2:
                new_lut = createCoolToWarmLookupTable(current_scalar_range[0], current_scalar_range[1]);
                break;
            case 3:
                new_lut = createGrayscaleLookupTable(current_scalar_range[0], current_scalar_range[1]);
                break;
            case 4:
                new_lut = createRainbowLookupTable(current_scalar_range[0], current_scalar_range[1]);
                break;
            }
            mapper->SetLookupTable(new_lut);
            renderWindow_->Render();
        }
    }
}

void ThreeDimensionalDisplayPage::setZAxisStretching()
{
    double zScale = zaxis_stretching_edit_->text().toDouble();
    model_pinpeline_builder_->setZAxisScale(zScale);

    // 获取模型类型
    auto modelType = model_pinpeline_builder_->getModelType();

    // 1. 移除旧 actor（所有可能的）
    renderer_->RemoveActor(model_pinpeline_builder_->getActor());          // PLY 点云 actor
    renderer_->RemoveActor(model_pinpeline_builder_->getSurfaceActor());   // OBJ 面 actor
    renderer_->RemoveActor(model_pinpeline_builder_->getWireframeActor()); // OBJ 线 actor
    renderer_->RemoveActor(model_pinpeline_builder_->getPointsActor());    // OBJ 点 actor

    // 2. 添加新 actor
    if (modelType == ModelPipelineBuilder::ModelType::PLY)
    {
        renderer_->AddActor(model_pinpeline_builder_->getActor());
    }
    else if (modelType == ModelPipelineBuilder::ModelType::OBJ)
    {
        if (auto surfaceActor = model_pinpeline_builder_->getSurfaceActor())
            renderer_->AddActor(surfaceActor);
        if (auto wireframeActor = model_pinpeline_builder_->getWireframeActor())
            renderer_->AddActor(wireframeActor);
        if (auto pointsActor = model_pinpeline_builder_->getPointsActor())
            renderer_->AddActor(pointsActor);
    }

    // 3. 更新 BoundingBox
    addBoundingBox(model_pinpeline_builder_->getProcessedPolyData());

    // 4. 更新 boxClipper
    vtkSmartPointer<vtkActor> primaryActor = nullptr;
    if (modelType == ModelPipelineBuilder::ModelType::PLY)
        primaryActor = model_pinpeline_builder_->getActor();
    else if (modelType == ModelPipelineBuilder::ModelType::OBJ)
        primaryActor = model_pinpeline_builder_->getSurfaceActor();

    boxClipper_->SetInputDataAndReplaceOriginal(model_pinpeline_builder_->getProcessedPolyData(), primaryActor);

    boxClipper_enabled_ = false;
    renderWindow_->Render();
}

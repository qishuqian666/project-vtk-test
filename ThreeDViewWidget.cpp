#include "ThreeDViewWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <qDebug>
#include <QDoubleValidator>
#include <QSlider>
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

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);
VTK_MODULE_INIT(vtkRenderingContextOpenGL2);

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

    main_layout_ = new QVBoxLayout();
    this->setLayout(main_layout_);
    main_layout_->setContentsMargins(20, 20, 20, 20);
    main_layout_->setSpacing(5);
    setLayout(main_layout_);
    setMinimumSize(800, 600);

    initSelectFilePath();
    initControlBtn();

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

    // 添加一个弹簧，将按钮推到左侧
    // control_btn_layout->addStretch();
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
}

void ThreeDimensionalDisplayPage::loadModelByExtension(const QString &filePath)
{
    if (!QFileInfo::exists(filePath))
    {
        qDebug() << "File does not exist: " << filePath;
        return;
    }

    vtkSmartPointer<vtkPolyData> polyData = nullptr;
    // 关键修改：添加 toStdString() 转换 QString 为 std::string
    std::string ext = QFileInfo(filePath).suffix().toLower().toStdString();

    if (ext == "ply")
    {
        vtkNew<vtkPLYReader> reader;
        reader->SetFileName(filePath.toStdString().c_str());
        reader->Update();
        polyData = reader->GetOutput();
        qDebug() << "vtkNew<vtkPLYReader> INIT ";
        loadPlyFile(polyData);
    }
    else if (ext == "obj")
    {
        vtkNew<vtkOBJReader> reader;
        reader->SetFileName(filePath.toStdString().c_str());
        reader->Update();
        polyData = reader->GetOutput();
        qDebug() << "vtkNew<vtkOBJReader> INIT ";
        loadObjFile(polyData);
    }
    else
    {
        qDebug() << "Unsupported file format: " << QString::fromStdString(ext);
        return;
    }

    if (!polyData || polyData->GetNumberOfPoints() == 0)
    {
        qDebug() << "Failed to load file or file is empty: " << filePath;
        return;
    }
}

void ThreeDimensionalDisplayPage::loadPlyFile(vtkSmartPointer<vtkPolyData> _poly_data)
{
    // 获取包围盒 (Bounds)
    double bounds[6];
    _poly_data->GetBounds(bounds);
    // std::cout << "Bounds: [" << bounds[0] << ", " << bounds[1] << "] x ["
    //           << bounds[2] << ", " << bounds[3] << "] x ["
    //           << bounds[4] << ", " << bounds[5] << "]" << std::endl;
    // std::cout << "Number of points: " << _poly_data->GetNumberOfPoints() << std::endl;
    // std::cout << "Number of polys: " << _poly_data->GetNumberOfPolys() << std::endl;

    // 计算模型中心
    double center[3] = {
        (bounds[0] + bounds[1]) / 2.0,
        (bounds[2] + bounds[3]) / 2.0,
        (bounds[4] + bounds[5]) / 2.0};

    // 将整个模型向反方向移动，使其中心点对齐原点
    vtkNew<vtkTransform> transform;
    transform->Translate(-center[0], -center[1], -center[2]);

    // 使用 vtkTransformPolyDataFilter 将上面的平移变换应用到原始模型 _poly_data 上，生成一个新的变换后的模型数据。
    // transformFilter->Update(); 实际执行数据处理，之后可以从 transformFilter->GetOutput() 获取变换后的模型。
    vtkNew<vtkTransformPolyDataFilter> transformFilter;
    transformFilter->SetInputData(_poly_data);
    transformFilter->SetTransform(transform.Get());
    transformFilter->Update();

    // Elevation着色（根据Z值） - 可选
    vtkNew<vtkElevationFilter> elevationFilter;
    elevationFilter->SetInputConnection(transformFilter->GetOutputPort());
    elevationFilter->SetLowPoint(0, 0, bounds[4] - center[2]);
    elevationFilter->SetHighPoint(0, 0, bounds[5] - center[2]);
    elevationFilter->Update();

    double scalarRange[2];
    elevationFilter->GetOutput()->GetScalarRange(scalarRange);
    vtkSmartPointer<vtkLookupTable> colorLookupTable = createJetLookupTable(scalarRange[0], scalarRange[1]);

    // 加入拓扑结构：点转为vtkVertex 显示点的关键代码
    // 将 vtkPolyData 中每个“孤立点”转换为一个 vtkVertex 图元，加入到拓扑结构中，使得 VTK 能将其作为可渲染的点显示出来。
    auto vertexGlyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
    vertexGlyphFilter->AddInputData(elevationFilter->GetOutput());
    vertexGlyphFilter->Update();

    // 映射器
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(vertexGlyphFilter->GetOutputPort()); // 用加了点结构的polydata
    mapper->SetScalarRange(elevationFilter->GetOutput()->GetScalarRange());
    mapper->SetLookupTable(colorLookupTable);
    mapper->SetColorModeToMapScalars();
    mapper->ScalarVisibilityOn();
    mapper->Update();

    // Actor
    ply_point_actor_ = vtkSmartPointer<vtkActor>::New();
    ply_point_actor_->SetMapper(mapper);
    ply_point_actor_->GetProperty()->SetPointSize(point_size_edit_->text().toInt());

    // 清空旧Actor，设置背景
    renderer_->RemoveAllViewProps();         // 清空旧的视图属性
    renderer_->SetBackground(1.0, 1.0, 1.0); // 设置背景颜色
    renderer_->AddActor(ply_point_actor_);   // 添加点云演员

    // 添加三维坐标轴
    addCoordinateAxes();
    //  添加BoundingBox
    addBoundingBox(transformFilter->GetOutput());

    renderer_->ResetCamera();
    renderWindow_->Render();
    m_pScene->update();
}

// 在 loadObjFile 中
void ThreeDimensionalDisplayPage::loadObjFile(vtkSmartPointer<vtkPolyData> _poly_data)
{
    renderer_->RemoveAllViewProps();
    renderer_->SetBackground(1.0, 1.0, 1.0); // 白色背景

    // 获取边界和中心
    double bounds[6];
    _poly_data->GetBounds(bounds);
    double center[3] = {
        (bounds[0] + bounds[1]) / 2.0,
        (bounds[2] + bounds[3]) / 2.0,
        (bounds[4] + bounds[5]) / 2.0};

    // Elevation Filter 按 Z 值添加标量
    vtkNew<vtkElevationFilter> elevationFilter;
    elevationFilter->SetInputData(_poly_data);
    elevationFilter->SetLowPoint(0, 0, bounds[4]);  // 最低Z
    elevationFilter->SetHighPoint(0, 0, bounds[5]); // 最高Z
    elevationFilter->Update();
    elevationFilter->GetOutput()->GetScalarRange(current_scalar_range); // 保存标量范围

    // 获取标量范围
    double scalarRange[2];
    elevationFilter->GetOutput()->GetScalarRange(scalarRange);

    vtkSmartPointer<vtkLookupTable> colorLookupTable = createJetLookupTable(scalarRange[0], scalarRange[1]);

    // 映射器
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(elevationFilter->GetOutputPort());
    mapper->SetScalarRange(scalarRange);
    mapper->SetLookupTable(colorLookupTable);
    mapper->SetColorModeToMapScalars();
    mapper->ScalarVisibilityOn();

    // 面Actor
    surfaceActor_ = vtkSmartPointer<vtkActor>::New();
    surfaceActor_->SetMapper(mapper);
    surfaceActor_->GetProperty()->SetOpacity(1.0);
    renderer_->AddActor(surfaceActor_);
    surfaceActor_->SetVisibility(is_surface_visible_);

    // 线框Actor
    wireframeActor_ = vtkSmartPointer<vtkActor>::New();
    wireframeActor_->SetMapper(mapper);
    wireframeActor_->GetProperty()->SetRepresentationToWireframe();
    wireframeActor_->GetProperty()->SetColor(0.2, 0.2, 0.2);
    wireframeActor_->GetProperty()->SetLineWidth(1.0);
    renderer_->AddActor(wireframeActor_);
    wireframeActor_->SetVisibility(is_wireframe_visible_);

    // 点Actor
    pointsActor_ = vtkSmartPointer<vtkActor>::New();
    pointsActor_->SetMapper(mapper);
    pointsActor_->GetProperty()->SetRepresentationToPoints();
    pointsActor_->GetProperty()->SetPointSize(point_size_edit_->text().toInt());
    pointsActor_->GetProperty()->SetColor(0.0, 0.0, 1.0);
    renderer_->AddActor(pointsActor_);
    pointsActor_->SetVisibility(is_points_visible_);

    renderer_->ResetCamera();
    renderWindow_->Render();
    m_pScene->update();
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
    if (ply_point_actor_)
        ply_point_actor_->GetProperty()->SetPointSize(point_size_edit_->text().toInt());
    if (pointsActor_)
        pointsActor_->GetProperty()->SetPointSize(point_size_edit_->text().toInt());
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

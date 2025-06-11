#include "modelcomponentpropertyeditor.h"

#include "../Engine/Engine.h"

using namespace Inno;

ModelComponentPropertyEditor::ModelComponentPropertyEditor()
{
}

void ModelComponentPropertyEditor::initialize()
{
    m_MaterialCompEditor = new MaterialComponentPropertyEditor();
    m_MaterialCompEditor->initialize();

    m_dirViewer = new DirectoryViewer();
    m_dirViewer->Initialize();
    m_dirViewer->hide();

    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(4, 4, 4, 4);

    m_title = new QLabel("ModelComponent");
    m_title->setStyleSheet(
                "background-repeat: no-repeat;"
                "background-position: left;"
                );

    m_transformWidget = new TransformWidget();
    m_transformWidget->initialize();

    m_meshPrimitiveTopology = new ComboLabelText();
    m_meshPrimitiveTopology->Initialize("MeshPrimitiveTopology");

    m_textureWrapMethod = new ComboLabelText();
    m_textureWrapMethod->Initialize("TextureWrapMethod");

    m_meshUsage = new ComboLabelText();
    m_meshUsage->Initialize("MeshUsage");

    m_meshSource = new ComboLabelText();
    m_meshSource->Initialize("MeshSource");

    m_proceduralMeshShape = new ComboLabelText();
    m_proceduralMeshShape->Initialize("ProceduralMeshShape");

    m_modelNameLabel = new QLabel("ModelName");

    m_chooseModelButton = new QPushButton();
    m_chooseModelButton->setText("Choose model");
    connect(m_chooseModelButton, SIGNAL(clicked(bool)), this, SLOT(ChooseModel()));

    m_modelListLabel = new QLabel("ModelList");

    m_modelList = new QTableWidget();
    m_modelList->setStyleSheet(
                "border-style: none;"
                "border-bottom: 1px solid #fffff8;"
                "border-right: 1px solid #fffff8;"
                );
    m_modelList->setColumnCount(2);
    m_modelList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_modelList,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            this,
            SLOT(onCustomContextMenuRequested(const QPoint&)));

    connect(m_modelList, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(tableItemClicked(int,int)));

    m_line = new QWidget();
    m_line->setFixedHeight(1);
    m_line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_line->setStyleSheet(QString("background-color: #585858;"));

    int row = 0;
    m_gridLayout->addWidget(m_title, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_transformWidget, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_meshPrimitiveTopology->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_meshPrimitiveTopology->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_textureWrapMethod->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_textureWrapMethod->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_meshUsage->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_meshUsage->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_meshSource->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_meshSource->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_proceduralMeshShape->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_proceduralMeshShape->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_modelNameLabel, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_chooseModelButton, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_modelListLabel, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_modelList, row, 1, 1, 7);
    row++;

    m_gridLayout->addWidget(m_line, row, 0, 1, 7);

    connect(m_transformWidget, SIGNAL(positionChanged()), this, SLOT(SetTransform()));
    connect(m_transformWidget, SIGNAL(rotationChanged()), this, SLOT(SetTransform()));
    connect(m_transformWidget, SIGNAL(scaleChanged()), this, SLOT(SetTransform()));
    connect(m_meshPrimitiveTopology, SIGNAL(ValueChanged()), this, SLOT(SetMeshPrimitiveTopology()));
    connect(m_textureWrapMethod, SIGNAL(ValueChanged()), this, SLOT(SetTextureWrapMethod()));
    connect(m_meshUsage, SIGNAL(ValueChanged()), this, SLOT(SetMeshUsage()));
    connect(m_meshSource, SIGNAL(ValueChanged()), this, SLOT(SetMeshSource()));
    connect(m_proceduralMeshShape, SIGNAL(ValueChanged()), this, SLOT(SetProceduralMeshShape()));

    m_gridLayout->setHorizontalSpacing(m_horizontalSpacing);
    m_gridLayout->setVerticalSpacing(m_verticalSpacing);

    this->setLayout(m_gridLayout);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    this->hide();
}

void ModelComponentPropertyEditor::edit(void *component)
{
    m_component = reinterpret_cast<ModelComponent*>(component);

    m_modelNameLabel->setText(m_component->m_InstanceName.c_str());

    // Update transform widget
    m_transformWidget->setPosition(m_component->m_Transform.m_pos.x, m_component->m_Transform.m_pos.y, m_component->m_Transform.m_pos.z);
    
    auto eulerAngles = Math::quatToEulerAngle(m_component->m_Transform.m_rot);
    m_transformWidget->setRotation(Math::radianToAngle(eulerAngles.x), Math::radianToAngle(eulerAngles.y), Math::radianToAngle(eulerAngles.z));
    
    m_transformWidget->setScale(m_component->m_Transform.m_scale.x, m_component->m_Transform.m_scale.y, m_component->m_Transform.m_scale.z);

    GetMeshPrimitiveTopology();
    GetTextureWrapMethod();
    GetMeshUsage();
    GetMeshSource();
    GetProceduralMeshShape();
    GetModelMap();

    this->show();
}

void ModelComponentPropertyEditor::GetMeshPrimitiveTopology()
{
    m_meshPrimitiveTopology->SetFromInt(0);
}

void ModelComponentPropertyEditor::GetTextureWrapMethod()
{
    m_textureWrapMethod->SetFromInt(0);
}

void ModelComponentPropertyEditor::GetMeshUsage()
{
    m_meshUsage->SetFromInt(0);
}

void ModelComponentPropertyEditor::GetMeshSource()
{
    m_meshSource->SetFromInt(0);
}

void ModelComponentPropertyEditor::GetProceduralMeshShape()
{
    m_proceduralMeshShape->SetFromInt(0);
}

void ModelComponentPropertyEditor::tableItemClicked(int row, int column)
{
    auto item = m_modelList->item(row,column);
    if(column == 0)
    {
        auto l_mesh = static_cast<MeshComponent*>(item->data(Qt::UserRole).value<void*>());
    }
    else
    {
        auto l_material = static_cast<MaterialComponent*>(item->data(Qt::UserRole).value<void*>());
        m_MaterialCompEditor->edit(l_material);
    }
}

void ModelComponentPropertyEditor::ChooseModel()
{
    m_dirViewer->show();
}

void ModelComponentPropertyEditor::GetModelMap()
{
    if (!m_component)
        return;

    auto l_drawCallComponentCount = m_component->m_DrawCallComponents.size();
    if (!l_drawCallComponentCount)
        return;

    m_modelList->setRowCount((int)l_drawCallComponentCount);

    int index = 0;
    for (uint64_t j = 0; j < l_drawCallComponentCount; j++)
    {
        // auto l_pair = g_Engine->Get<AssetSystem>()->GetMeshMaterialPair(m_component->m_model->meshMaterialPairs.m_startOffset + j);
        // auto l_meshItem = new QTableWidgetItem();
        // l_meshItem->setText(l_pair->mesh->m_InstanceName.c_str());
        // l_meshItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(l_pair->mesh)));
        // l_meshItem->setFlags(l_meshItem->flags() & ~Qt::ItemIsEditable);
        // m_modelList->setItem(index, 0, l_meshItem);

        // auto l_materialItem = new QTableWidgetItem();
        // l_materialItem->setText(l_pair->material->m_InstanceName.c_str());
        // l_materialItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(l_pair->material)));
        // l_materialItem->setFlags(l_materialItem->flags() & ~Qt::ItemIsEditable);
        // m_modelList->setItem(index, 1, l_materialItem);

        index++;
    }
}

void ModelComponentPropertyEditor::SetMeshPrimitiveTopology()
{
    //m_component->m_meshPrimitiveTopology = MeshPrimitiveTopology(m_meshPrimitiveTopology->GetAsInt());
}

void ModelComponentPropertyEditor::SetTextureWrapMethod()
{
    //m_component->m_textureWrapMethod = TextureWrapMethod(m_textureWrapMethod->GetAsInt());
}

void ModelComponentPropertyEditor::SetMeshUsage()
{
    //m_component->m_meshUsage = MeshUsage(m_meshUsage->GetAsInt());
}

void ModelComponentPropertyEditor::SetMeshSource()
{
    //m_component->m_meshSource = MeshSource(m_meshSource->GetAsInt());
}

void ModelComponentPropertyEditor::SetProceduralMeshShape()
{
    //m_component->m_proceduralMeshShape = ProceduralMeshShape(m_proceduralMeshShape->GetAsInt());
}

void ModelComponentPropertyEditor::SetTransform()
{
    if (!m_component)
        return;

    float x, y, z;
    m_transformWidget->getPosition(x, y, z);
    m_component->m_Transform.m_pos = Vec4(x, y, z, 1.0f);

    m_transformWidget->getRotation(x, y, z);
    auto roll = Math::angleToRadian(x);
    auto pitch = Math::angleToRadian(y);
    auto yaw = Math::angleToRadian(z);
    m_component->m_Transform.m_rot = Math::eulerAngleToQuat(roll, pitch, yaw);

    m_transformWidget->getScale(x, y, z);
    m_component->m_Transform.m_scale = Vec4(x, y, z, 1.0f);
}

void ModelComponentPropertyEditor::remove()
{
    m_modelList->clear();
    m_component = nullptr;
    this->hide();
}

void ModelComponentPropertyEditor::onCustomContextMenuRequested(const QPoint &pos)
{
    showContextMenu(m_modelList->viewport()->mapToGlobal(pos));
}

void ModelComponentPropertyEditor::showContextMenu(const QPoint &globalPos)
{
    QMenu menu;

    menu.addAction("Model list context menu");
    menu.exec(globalPos);
}

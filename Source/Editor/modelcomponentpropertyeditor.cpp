#include "modelcomponentpropertyeditor.h"
#include "../Engine/Engine.h"
#include "../Engine/Services/ComponentManager.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>

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

    m_modelNameLabel = new QLabel("ModelName");

    m_addMeshButton = new QPushButton();
    m_addMeshButton->setText("Add Mesh");
    connect(m_addMeshButton, SIGNAL(clicked(bool)), this, SLOT(AddMesh()));

    m_addMaterialButton = new QPushButton();
    m_addMaterialButton->setText("Add Material");
    connect(m_addMaterialButton, SIGNAL(clicked(bool)), this, SLOT(AddMaterial()));

    m_removeSelectedButton = new QPushButton();
    m_removeSelectedButton->setText("Remove Selected");
    connect(m_removeSelectedButton, SIGNAL(clicked(bool)), this, SLOT(RemoveSelected()));

    m_drawCallListLabel = new QLabel("Draw Calls (Mesh + Material pairs)");

    m_drawCallList = new QTableWidget();
    m_drawCallList->setStyleSheet(
                "border-style: none;"
                "border-bottom: 1px solid #fffff8;"
                "border-right: 1px solid #fffff8;"
                );
    m_drawCallList->setColumnCount(3);
    QStringList headers;
    headers << "Mesh" << "Material" << "Actions";
    m_drawCallList->setHorizontalHeaderLabels(headers);
    m_drawCallList->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_drawCallList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_drawCallList,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            this,
            SLOT(onCustomContextMenuRequested(const QPoint&)));

    connect(m_drawCallList, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(tableItemClicked(int,int)));

    connect(m_dirViewer, SIGNAL(fileSelected(QString)), this, SLOT(onFileSelected(QString)));

    m_line = new QWidget();
    m_line->setFixedHeight(1);
    m_line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_line->setStyleSheet(QString("background-color: #585858;"));

    int row = 0;
    m_gridLayout->addWidget(m_title, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_transformWidget, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_modelNameLabel, row, 0, 1, 7);
    row++;

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_addMeshButton);
    buttonLayout->addWidget(m_addMaterialButton);
    buttonLayout->addWidget(m_removeSelectedButton);
    
    QWidget* buttonWidget = new QWidget();
    buttonWidget->setLayout(buttonLayout);
    m_gridLayout->addWidget(buttonWidget, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_drawCallListLabel, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_drawCallList, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_line, row, 0, 1, 7);

    connect(m_transformWidget, SIGNAL(positionChanged()), this, SLOT(SetTransform()));
    connect(m_transformWidget, SIGNAL(rotationChanged()), this, SLOT(SetTransform()));
    connect(m_transformWidget, SIGNAL(scaleChanged()), this, SLOT(SetTransform()));

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

    RefreshDrawCallList();

    this->show();
}

void ModelComponentPropertyEditor::RefreshDrawCallList()
{
    if (!m_component)
        return;

    auto drawCallCount = m_component->m_DrawCallComponents.size();
    m_drawCallList->setRowCount((int)drawCallCount);

    for (size_t i = 0; i < drawCallCount; i++)
    {
        auto drawCallId = m_component->m_DrawCallComponents[i];
        auto componentManager = g_Engine->Get<ComponentManager>();
        auto drawCall = componentManager->FindByUUID<DrawCallComponent>(drawCallId);
        
        if (drawCall)
        {
            auto meshItem = new QTableWidgetItem();
            auto materialItem = new QTableWidgetItem();
            
            if (drawCall->m_MeshComponent != 0)
            {
                auto mesh = componentManager->FindByUUID<MeshComponent>(drawCall->m_MeshComponent);
                meshItem->setText(mesh ? mesh->m_InstanceName.c_str() : "Unknown Mesh");
            }
            else
            {
                meshItem->setText("No Mesh");
            }
            
            if (drawCall->m_MaterialComponent != 0)
            {
                auto material = componentManager->FindByUUID<MaterialComponent>(drawCall->m_MaterialComponent);
                materialItem->setText(material ? material->m_InstanceName.c_str() : "Unknown Material");
            }
            else
            {
                materialItem->setText("No Material");
            }
            
            meshItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<qulonglong>(drawCallId)));
            meshItem->setFlags(meshItem->flags() & ~Qt::ItemIsEditable);
            m_drawCallList->setItem((int)i, 0, meshItem);

            materialItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<qulonglong>(drawCallId)));
            materialItem->setFlags(materialItem->flags() & ~Qt::ItemIsEditable);
            m_drawCallList->setItem((int)i, 1, materialItem);
        }
        else
        {
            auto meshItem = new QTableWidgetItem("Invalid DrawCall");
            auto materialItem = new QTableWidgetItem("Invalid DrawCall");
            m_drawCallList->setItem((int)i, 0, meshItem);
            m_drawCallList->setItem((int)i, 1, materialItem);
        }

        auto removeButton = new QPushButton("Remove");
        connect(removeButton, &QPushButton::clicked, [this, i]() { RemoveDrawCall((int)i); });
        m_drawCallList->setCellWidget((int)i, 2, removeButton);
    }
}

void ModelComponentPropertyEditor::AddMesh()
{
    m_currentAssetType = AssetType::Mesh;
    m_dirViewer->SetFilter("*.MeshComponent.json");
    m_dirViewer->show();
}

void ModelComponentPropertyEditor::AddMaterial()
{
    m_currentAssetType = AssetType::Material;
    m_dirViewer->SetFilter("*.MaterialComponent.json");
    m_dirViewer->show();
}

void ModelComponentPropertyEditor::RemoveSelected()
{
    int currentRow = m_drawCallList->currentRow();
    if (currentRow >= 0)
    {
        RemoveDrawCall(currentRow);
    }
}

void ModelComponentPropertyEditor::RemoveDrawCall(int row)
{
    if (!m_component || row < 0 || row >= (int)m_component->m_DrawCallComponents.size())
        return;

    // Remove from array using iterators
    auto it = m_component->m_DrawCallComponents.begin() + row;
    auto heapAddr = m_component->m_DrawCallComponents.begin();
    auto currentSize = m_component->m_DrawCallComponents.size();
    
    // Move elements to fill the gap
    for (size_t i = row; i < currentSize - 1; i++)
    {
        heapAddr[i] = heapAddr[i + 1];
    }
    
    // Decrease the current free index manually
    auto& drawCallArray = m_component->m_DrawCallComponents;
    auto currentFreeIdx = drawCallArray.size() - 1;
    drawCallArray.clear();
    for (size_t i = 0; i < currentFreeIdx; i++)
    {
        drawCallArray.emplace_back(heapAddr[i]);
    }
    
    RefreshDrawCallList();
}

void ModelComponentPropertyEditor::onFileSelected(const QString& filePath)
{
    m_dirViewer->hide();

    if (!m_component)
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Error", "Cannot open file: " + filePath);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();

    if (m_currentAssetType == AssetType::Mesh)
    {
        if (obj["ComponentType"].toInt() != 6) // MeshComponent type ID
        {
            QMessageBox::warning(this, "Error", "Selected file is not a MeshComponent");
            return;
        }
        CreateDrawCall(filePath, true);
    }
    else if (m_currentAssetType == AssetType::Material)
    {
        if (obj["ComponentType"].toInt() != 7) // MaterialComponent type ID
        {
            QMessageBox::warning(this, "Error", "Selected file is not a MaterialComponent");
            return;
        }
        CreateDrawCall(filePath, false);
    }
}

void ModelComponentPropertyEditor::CreateDrawCall(const QString& assetPath, bool isMesh)
{
    auto componentManager = g_Engine->Get<ComponentManager>();
    QFileInfo fileInfo(assetPath);
    QString componentName = fileInfo.completeBaseName();
    
    DrawCallComponent* targetDrawCall = nullptr;
    if (isMesh)
    {
        // Find existing DrawCall without a mesh or create new one
        for (size_t i = 0; i < m_component->m_DrawCallComponents.size(); i++)
        {
            auto drawCallId = m_component->m_DrawCallComponents[i];
            auto drawCall = componentManager->FindByUUID<DrawCallComponent>(drawCallId);
            if (drawCall && drawCall->m_MeshComponent == 0)
            {
                targetDrawCall = drawCall;
                break;
            }
        }
        
        if (!targetDrawCall)
        {
            targetDrawCall = componentManager->Spawn<DrawCallComponent>(m_component->m_Owner, true, ObjectLifespan::Scene);
            if (targetDrawCall)
            {
                m_component->m_DrawCallComponents.emplace_back(targetDrawCall->m_UUID);
            }
        }
        
        if (targetDrawCall)
        {
            uint64_t meshId = componentManager->Load<MeshComponent>(componentName.toStdString().c_str(), m_component->m_Owner);
            targetDrawCall->m_MeshComponent = meshId;
        }
    }
    else // Material
    {
        for (size_t i = 0; i < m_component->m_DrawCallComponents.size(); i++)
        {
            auto drawCallId = m_component->m_DrawCallComponents[i];
            auto drawCall = componentManager->FindByUUID<DrawCallComponent>(drawCallId);
            if (drawCall && drawCall->m_MaterialComponent == 0)
            {
                targetDrawCall = drawCall;
                break;
            }
        }
        
        if (!targetDrawCall)
        {
            targetDrawCall = componentManager->Spawn<DrawCallComponent>(m_component->m_Owner, true, ObjectLifespan::Scene);
            if (targetDrawCall)
            {
                m_component->m_DrawCallComponents.emplace_back(targetDrawCall->m_UUID);
            }
        }
        
        if (targetDrawCall)
        {
            uint64_t materialId = componentManager->Load<MaterialComponent>(componentName.toStdString().c_str(), m_component->m_Owner);
            targetDrawCall->m_MaterialComponent = materialId;
        }
    }
    
    if (targetDrawCall->m_MeshComponent && targetDrawCall->m_MaterialComponent)
        targetDrawCall->m_ObjectStatus = ObjectStatus::Activated;
      
    RefreshDrawCallList();
}

void ModelComponentPropertyEditor::tableItemClicked(int row, int column)
{
    auto item = m_drawCallList->item(row, column);
    if (!item)
        return;

    auto componentManager = g_Engine->Get<ComponentManager>();
    auto drawCallId = item->data(Qt::UserRole).value<qulonglong>();
    auto drawCall = componentManager->FindByUUID<DrawCallComponent>(drawCallId);
    
    if (!drawCall)
        return;

    if (column == 0 && drawCall->m_MeshComponent != 0) // Mesh column
    {
        auto mesh = componentManager->FindByUUID<MeshComponent>(drawCall->m_MeshComponent);
        if (mesh)
        {
            // Open mesh editor or show properties
            QMessageBox::information(this, "Mesh Info", QString("Mesh: %1").arg(mesh->m_InstanceName.c_str()));
        }
    }
    else if (column == 1 && drawCall->m_MaterialComponent != 0) // Material column
    {
        auto material = componentManager->FindByUUID<MaterialComponent>(drawCall->m_MaterialComponent);
        if (material)
        {
            m_MaterialCompEditor->edit(material);
        }
    }
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
    m_drawCallList->clear();
    m_component = nullptr;
    this->hide();
}

void ModelComponentPropertyEditor::onCustomContextMenuRequested(const QPoint &pos)
{
    showContextMenu(m_drawCallList->viewport()->mapToGlobal(pos));
}

void ModelComponentPropertyEditor::showContextMenu(const QPoint &globalPos)
{
    QMenu menu;

    QAction* addMeshAction = menu.addAction("Add Mesh");
    QAction* addMaterialAction = menu.addAction("Add Material");
    menu.addSeparator();
    QAction* removeAction = menu.addAction("Remove Selected");
    
    connect(addMeshAction, &QAction::triggered, this, &ModelComponentPropertyEditor::AddMesh);
    connect(addMaterialAction, &QAction::triggered, this, &ModelComponentPropertyEditor::AddMaterial);
    connect(removeAction, &QAction::triggered, this, &ModelComponentPropertyEditor::RemoveSelected);

    menu.exec(globalPos);
}

#include "visiblecomponentpropertyeditor.h"

#include "../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

VisibleComponentPropertyEditor::VisibleComponentPropertyEditor()
{
}

void VisibleComponentPropertyEditor::initialize()
{
    m_MaterialCompEditor = new MaterialComponentPropertyEditor();
    m_MaterialCompEditor->initialize();

    m_dirViewer = new DirectoryViewer();
    m_dirViewer->Initialize();
    m_dirViewer->hide();

    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(4, 4, 4, 4);

    m_title = new QLabel("VisibleComponent");
    m_title->setStyleSheet(
                "background-repeat: no-repeat;"
                "background-position: left;"
                );

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

void VisibleComponentPropertyEditor::edit(void *component)
{
    m_component = reinterpret_cast<VisibleComponent*>(component);

    m_modelNameLabel->setText(m_component->m_modelFileName.c_str());

    GetMeshPrimitiveTopology();
    GetTextureWrapMethod();
    GetMeshUsage();
    GetMeshSource();
    GetProceduralMeshShape();
    GetModelMap();

    this->show();
}

void VisibleComponentPropertyEditor::GetMeshPrimitiveTopology()
{
    m_meshPrimitiveTopology->SetFromInt((int)m_component->m_meshPrimitiveTopology);
}

void VisibleComponentPropertyEditor::GetTextureWrapMethod()
{
    m_textureWrapMethod->SetFromInt((int)m_component->m_textureWrapMethod);
}

void VisibleComponentPropertyEditor::GetMeshUsage()
{
    m_meshUsage->SetFromInt((int)m_component->m_meshUsage);
}

void VisibleComponentPropertyEditor::GetMeshSource()
{
    m_meshSource->SetFromInt((int)m_component->m_meshSource);
}

void VisibleComponentPropertyEditor::GetProceduralMeshShape()
{
    m_proceduralMeshShape->SetFromInt((int)m_component->m_proceduralMeshShape);
}

void VisibleComponentPropertyEditor::tableItemClicked(int row, int column)
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

void VisibleComponentPropertyEditor::ChooseModel()
{
    m_dirViewer->show();
}

void VisibleComponentPropertyEditor::GetModelMap()
{
    if (!m_component)
        return;
    if (!m_component->m_model)
        return;

    m_modelList->setRowCount((int)m_component->m_model->meshMaterialPairs.m_count);

    int index = 0;

    for (uint64_t j = 0; j < m_component->m_model->meshMaterialPairs.m_count; j++)
    {
        auto l_pair = g_Engine->getAssetSystem()->getMeshMaterialPair(m_component->m_model->meshMaterialPairs.m_startOffset + j);
        auto l_meshItem = new QTableWidgetItem();
        l_meshItem->setText(l_pair->mesh->m_InstanceName.c_str());
        l_meshItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(l_pair->mesh)));
        l_meshItem->setFlags(l_meshItem->flags() & ~Qt::ItemIsEditable);
        m_modelList->setItem(index, 0, l_meshItem);

        auto l_materialItem = new QTableWidgetItem();
        l_materialItem->setText(l_pair->material->m_InstanceName.c_str());
        l_materialItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(l_pair->material)));
        l_materialItem->setFlags(l_materialItem->flags() & ~Qt::ItemIsEditable);
        m_modelList->setItem(index, 1, l_materialItem);

        index++;
    }
}

void VisibleComponentPropertyEditor::SetMeshPrimitiveTopology()
{
    m_component->m_meshPrimitiveTopology = MeshPrimitiveTopology(m_meshPrimitiveTopology->GetAsInt());
}

void VisibleComponentPropertyEditor::SetTextureWrapMethod()
{
    m_component->m_textureWrapMethod = TextureWrapMethod(m_textureWrapMethod->GetAsInt());
}

void VisibleComponentPropertyEditor::SetMeshUsage()
{
    m_component->m_meshUsage = MeshUsage(m_meshUsage->GetAsInt());
}

void VisibleComponentPropertyEditor::SetMeshSource()
{
    m_component->m_meshSource = MeshSource(m_meshSource->GetAsInt());
}

void VisibleComponentPropertyEditor::SetProceduralMeshShape()
{
    m_component->m_proceduralMeshShape = ProceduralMeshShape(m_proceduralMeshShape->GetAsInt());
}

void VisibleComponentPropertyEditor::remove()
{
    m_modelList->clear();
    m_component = nullptr;
    this->hide();
}

void VisibleComponentPropertyEditor::onCustomContextMenuRequested(const QPoint &pos)
{
    showContextMenu(m_modelList->viewport()->mapToGlobal(pos));
}

void VisibleComponentPropertyEditor::showContextMenu(const QPoint &globalPos)
{
    QMenu menu;

    menu.addAction("Model list context menu");
    menu.exec(globalPos);
}

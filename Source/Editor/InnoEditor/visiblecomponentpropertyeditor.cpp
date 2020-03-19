#include "visiblecomponentpropertyeditor.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

VisibleComponentPropertyEditor::VisibleComponentPropertyEditor()
{
}

void VisibleComponentPropertyEditor::initialize()
{
    m_MDCEditor = new MaterialDataComponentPropertyEditor();
    m_MDCEditor->initialize();

    m_dirViewer = new InnoDirectoryViewer();
    m_dirViewer->Initialize();
    m_dirViewer->hide();

    m_gridLayout = new QGridLayout();
    m_gridLayout->setMargin(4);

    m_title = new QLabel("VisibleComponent");
    m_title->setStyleSheet(
                "background-repeat: no-repeat;"
                "background-position: left;"
                );

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

    m_gridLayout->addWidget(m_modelNameLabel, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_chooseModelButton, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_modelListLabel, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_modelList, row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_line, row, 0, 1, 7);

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

    GetModelMap();

    this->show();
}

void VisibleComponentPropertyEditor::tableItemClicked(int row, int column)
{
    auto item = m_modelList->item(row,column);
    if(column == 0)
    {
        auto l_mesh = static_cast<MeshDataComponent*>(item->data(Qt::UserRole).value<void*>());
    }
    else
    {
        auto l_material = static_cast<MaterialDataComponent*>(item->data(Qt::UserRole).value<void*>());
        m_MDCEditor->edit(l_material);
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

    m_modelList->setRowCount((int)m_component->m_model->meshMaterialPairs.m_count);

    int index = 0;

    for (uint64_t j = 0; j < m_component->m_model->meshMaterialPairs.m_count; j++)
    {
        auto l_pair = g_pModuleManager->getAssetSystem()->getMeshMaterialPair(m_component->m_model->meshMaterialPairs.m_startOffset + j);
        auto l_meshItem = new QTableWidgetItem();
        l_meshItem->setText(l_pair->mesh->m_ComponentName.c_str());
        l_meshItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(l_pair->mesh)));
        l_meshItem->setFlags(l_meshItem->flags() & ~Qt::ItemIsEditable);
        m_modelList->setItem(index, 0, l_meshItem);

        auto l_materialItem = new QTableWidgetItem();
        l_materialItem->setText(l_pair->material->m_ComponentName.c_str());
        l_materialItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(l_pair->material)));
        l_materialItem->setFlags(l_materialItem->flags() & ~Qt::ItemIsEditable);
        m_modelList->setItem(index, 1, l_materialItem);

        index++;
    }
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

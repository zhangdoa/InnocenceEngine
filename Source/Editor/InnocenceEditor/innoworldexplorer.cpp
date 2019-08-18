#include "innoworldexplorer.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

InnoWorldExplorer::InnoWorldExplorer(QWidget* parent) : QTreeWidget(parent)
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            SLOT(onCustomContextMenuRequested(const QPoint&)));
}

void InnoWorldExplorer::buildTree()
{
    m_rootItem = new QTreeWidgetItem(this);
    m_rootItem->setText(0, "Entities");
    this->addTopLevelItem(m_rootItem);

    auto l_sceneHierarchyMap = g_pModuleManager->getSceneHierarchyManager()->GetSceneHierarchyMap();

    for (auto& i : l_sceneHierarchyMap)
    {
        if (i.first->m_objectSource == ObjectSource::Asset)
        {
            QTreeWidgetItem* l_entityItem = new QTreeWidgetItem();
            l_entityItem->setText(0, i.first->m_entityName.c_str());
            // Data slot 0 is ComponentType (-1 as the entity), slot 1 is the component ptr
            l_entityItem->setData(0, Qt::UserRole, QVariant(-1));

            addChild(m_rootItem, l_entityItem);

            for (auto& j : i.second)
            {
                QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();
                l_componentItem->setText(0, j->m_componentName.c_str());

                l_componentItem->setData(0, Qt::UserRole, QVariant((int)j->m_ComponentType));
                l_componentItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)j));
                addChild(l_entityItem, l_componentItem);
            }
        }
    }
}

void InnoWorldExplorer::initialize(InnoPropertyEditor* propertyEditor)
{
    m_propertyEditor = propertyEditor;

    buildTree();

    f_sceneLoadingFinishCallback
            = [&]()
    {
        clear();
        buildTree();
    };

    g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);
}

void InnoWorldExplorer::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QTreeWidget::selectionChanged(selected, deselected);
    QList<QTreeWidgetItem*> selectedItems = this->selectedItems();
    QTreeWidgetItem* item;
    if (selectedItems.count() != 0)
    {
        item = selectedItems[0];
        if (item != m_rootItem)
        {
            if (m_propertyEditor)
            {
                auto l_componentType = item->data(0, Qt::UserRole).toInt();
                if (l_componentType != -1)
                {
                    auto l_componentPtr = item->data(1, Qt::UserRole).value<void*>();
                    m_propertyEditor->editComponent(l_componentType, l_componentPtr);
                }
                else
                {
                    m_propertyEditor->remove();
                }
            }
        }
    }
}

void InnoWorldExplorer::addChild(QTreeWidgetItem *parent, QTreeWidgetItem *child)
{
    parent->addChild(child);
}

void InnoWorldExplorer::onCustomContextMenuRequested(const QPoint& pos)
{
    QTreeWidgetItem* item = itemAt(pos);

    if (item)
    {
        showContextMenu(item, viewport()->mapToGlobal(pos));
    }
    else
    {
        showGeneralMenu(viewport()->mapToGlobal(pos));
    }
}

void InnoWorldExplorer::showContextMenu(QTreeWidgetItem* item, const QPoint& globalPos)
{
    QMenu menu;

    if(item != m_rootItem)
    {
        auto l_componentTypeInt = item->data(0, Qt::UserRole).toInt();
        if (l_componentTypeInt != -1)
        {
            auto l_componentType = ComponentType(l_componentTypeInt);
            menu.addAction("Component context menu");
            menu.addAction("Rename");
        }
        else
        {
            menu.addAction("Entity context menu");
            menu.addAction("Rename");
            menu.addAction("Add Component");
        }
    }
    else
    {
        menu.addAction("Add Entity");
    }

    menu.exec(globalPos);
}

InnoEntity* InnoWorldExplorer::AddEntity(const QString& name)
{
    auto l_name = name.toStdString().c_str();
    return g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Asset, ObjectUsage::Gameplay, l_name);
}

void InnoWorldExplorer::showGeneralMenu(const QPoint &globalPos)
{
    QMenu menu;
    menu.addAction("General menu");
    menu.exec(globalPos);
}





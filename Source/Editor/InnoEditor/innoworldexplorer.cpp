#include "innoworldexplorer.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_Engine;

#include "../../Engine/Common/CommonMacro.inl"
#include "../../Engine/ComponentManager/ITransformComponentManager.h"
#include "../../Engine/ComponentManager/IVisibleComponentManager.h"
#include "../../Engine/ComponentManager/ILightComponentManager.h"
#include "../../Engine/ComponentManager/ICameraComponentManager.h"

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

    auto l_sceneHierarchyMap = g_Engine->getSceneHierarchyManager()->GetSceneHierarchyMap();

    for (auto& i : l_sceneHierarchyMap)
    {
        if (i.first->m_ObjectSource == ObjectSource::Asset)
        {
            QTreeWidgetItem* l_entityItem = new QTreeWidgetItem();

            l_entityItem->setText(0, i.first->m_Name.c_str());
            // Data slot 0 is ComponentType (-1 as the entity), slot 1 is the component ptr
            l_entityItem->setData(0, Qt::UserRole, QVariant(-1));
            l_entityItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)i.first));

            addChild(m_rootItem, l_entityItem);

            for (auto& j : i.second)
            {
                QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();
                l_componentItem->setText(0, j->m_Name.c_str());

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

    f_sceneLoadingFinishCallback = [&]()
    {
        clear();
        buildTree();
    };

    g_Engine->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);
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

void InnoWorldExplorer::startRename()
{
    auto l_items = this->selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];
        m_currentEditingItem = item;
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        this->editItem(item);
        connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(endRename()));
    }
}

void InnoWorldExplorer::endRename()
{
    auto l_componentType = m_currentEditingItem->data(0, Qt::UserRole).toInt();

    if (l_componentType != -1)
    {
        auto l_componentPtr = reinterpret_cast<InnoComponent*>(m_currentEditingItem->data(1, Qt::UserRole).value<void*>());
        l_componentPtr->m_Name = (m_currentEditingItem->text(0).toStdString() + "/").c_str();
    }
    else
    {
        auto l_entityPtr = reinterpret_cast<InnoEntity*>(m_currentEditingItem->data(1, Qt::UserRole).value<void*>());
        l_entityPtr->m_Name = (m_currentEditingItem->text(0).toStdString() + "/").c_str();
    }

    m_currentEditingItem->setFlags(m_currentEditingItem->flags() & ~Qt::ItemIsEditable);
    disconnect(this, SIGNAL(itemSelectionChanged()), this, SLOT(endRename()));
}

void InnoWorldExplorer::addEntity()
{
    auto l_entity = g_Engine->getEntityManager()->Spawn(ObjectSource::Asset, ObjectOwnership::Client, "newEntity/");

    QTreeWidgetItem* l_entityItem = new QTreeWidgetItem();

    l_entityItem->setText(0, l_entity->m_Name.c_str());
    l_entityItem->setData(0, Qt::UserRole, QVariant(-1));
    l_entityItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)l_entity));

    addChild(m_rootItem, l_entityItem);
    this->setCurrentItem(l_entityItem);
    startRename();
}

void InnoWorldExplorer::deleteEntity()
{
    auto l_items = this->selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];

        auto l_entityPtr = reinterpret_cast<InnoEntity*>(item->data(1, Qt::UserRole).value<void*>());

        for (int i = 0; i < item->childCount(); i++)
        {
            auto l_childItem = item->child(i);
            auto l_componentPtr = reinterpret_cast<InnoComponent*>(l_childItem->data(1, Qt::UserRole).value<void*>());
            destroyComponent(l_componentPtr);
        }

        g_Engine->getEntityManager()->Destroy(l_entityPtr);

        item->parent()->removeChild(item);
    }
}

void InnoWorldExplorer::addTransformComponent()
{
    auto l_items = this->selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];
        auto l_entityPtr = reinterpret_cast<InnoEntity*>(item->data(1, Qt::UserRole).value<void*>());

        auto l_componentPtr = SpawnComponent(TransformComponent, l_entityPtr, ObjectSource::Asset, ObjectOwnership::Client);
        auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());
        l_componentPtr->m_parentTransformComponent = l_rootTranformComponent;

        QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();

        l_componentItem->setText(0, l_componentPtr->m_Name.c_str());
        l_componentItem->setData(0, Qt::UserRole, QVariant(1));
        l_componentItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)l_componentPtr));

        addChild(item, l_componentItem);
        this->setCurrentItem(l_componentItem);
        startRename();
    }
}

void InnoWorldExplorer::addVisibleComponent()
{
    auto l_items = this->selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];
        auto l_entityPtr = reinterpret_cast<InnoEntity*>(item->data(1, Qt::UserRole).value<void*>());

        auto l_componentPtr = SpawnComponent(VisibleComponent, l_entityPtr, ObjectSource::Asset, ObjectOwnership::Client);

        QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();

        l_componentItem->setText(0, l_componentPtr->m_Name.c_str());
        l_componentItem->setData(0, Qt::UserRole, QVariant(2));
        l_componentItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)l_componentPtr));

        addChild(item, l_componentItem);
        this->setCurrentItem(l_componentItem);
        startRename();
    }
}

void InnoWorldExplorer::addLightComponent()
{
    auto l_items = this->selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];
        auto l_entityPtr = reinterpret_cast<InnoEntity*>(item->data(1, Qt::UserRole).value<void*>());

        auto l_componentPtr = SpawnComponent(LightComponent, l_entityPtr, ObjectSource::Asset, ObjectOwnership::Client);

        QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();

        l_componentItem->setText(0, l_componentPtr->m_Name.c_str());
        l_componentItem->setData(0, Qt::UserRole, QVariant(3));
        l_componentItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)l_componentPtr));

        addChild(item, l_componentItem);
        this->setCurrentItem(l_componentItem);
        startRename();
    }
}

void InnoWorldExplorer::addCameraComponent()
{
    auto l_items = this->selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];
        auto l_entityPtr = reinterpret_cast<InnoEntity*>(item->data(1, Qt::UserRole).value<void*>());

        auto l_componentPtr = SpawnComponent(CameraComponent, l_entityPtr, ObjectSource::Asset, ObjectOwnership::Client);

        QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();

        l_componentItem->setText(0, l_componentPtr->m_Name.c_str());
        l_componentItem->setData(0, Qt::UserRole, QVariant(4));
        l_componentItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)l_componentPtr));

        addChild(item, l_componentItem);
        this->setCurrentItem(l_componentItem);
        startRename();
    }
}

void InnoWorldExplorer::destroyComponent(InnoComponent *component)
{
    if (component->m_ComponentType == 1)
    {
        DestroyComponent(TransformComponent, component);
    }
    else if (component->m_ComponentType == 2)
    {
        DestroyComponent(VisibleComponent, component);
    }
    else if (component->m_ComponentType == 3)
    {
        DestroyComponent(LightComponent, component);
    }
    else if (component->m_ComponentType == 4)
    {
        DestroyComponent(CameraComponent, component);
    }
}

void InnoWorldExplorer::deleteComponent()
{
    auto l_items = this->selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];

        auto l_componentPtr = reinterpret_cast<InnoComponent*>(item->data(1, Qt::UserRole).value<void*>());
        destroyComponent(l_componentPtr);

        item->parent()->removeChild(item);
    }
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

    if (item != m_rootItem)
    {
        auto l_componentTypeInt = item->data(0, Qt::UserRole).toInt();
        if (l_componentTypeInt != -1)
        {
            auto l_componentType = l_componentTypeInt;
            menu.addAction("Component context menu");
            menu.addAction("Rename", this, SLOT(startRename()));
            menu.addAction("Delete", this, SLOT(deleteComponent()));
        }
        else
        {
            menu.addAction("Entity context menu");
            menu.addAction("Rename", this, SLOT(startRename()));

            auto addCompoentMenu = menu.addMenu("Add Component");
            addCompoentMenu->addAction("Add TransformComponent", this, SLOT(addTransformComponent()));
            addCompoentMenu->addAction("Add VisibleComponent", this, SLOT(addVisibleComponent()));
            addCompoentMenu->addAction("Add LightComponent", this, SLOT(addLightComponent()));
            addCompoentMenu->addAction("Add CameraComponent", this, SLOT(addCameraComponent()));

            menu.addAction("Delete", this, SLOT(deleteEntity()));
        }
    }
    else
    {
        menu.addAction("Add Entity", this, SLOT(addEntity()));
    }

    menu.exec(globalPos);
}

void InnoWorldExplorer::showGeneralMenu(const QPoint &globalPos)
{
    QMenu menu;
    menu.addAction("General menu");
    menu.exec(globalPos);
}

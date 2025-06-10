#include "worldexplorer.h"

#include "../Engine/Engine.h"
#include "../Engine/Common/ComponentHeaders.h"
#include "../Engine/Services/SceneService.h"
#include "../Engine/Services/EntityManager.h"
#include "../Engine/Services/ComponentManager.h"

using namespace Inno;

WorldExplorer::WorldExplorer(QWidget* parent) : QTreeWidget(parent)
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            SLOT(onCustomContextMenuRequested(const QPoint&)));
}

void WorldExplorer::buildTree()
{
    m_rootItem = new QTreeWidgetItem(this);
    m_rootItem->setText(0, "Entities");
    this->addTopLevelItem(m_rootItem);

    auto l_sceneHierarchyMap = g_Engine->Get<SceneService>()->getSceneHierarchyMap();

    for (auto& i : l_sceneHierarchyMap)
    {
        if (i.first->m_Serializable)
        {
            QTreeWidgetItem* l_entityItem = new QTreeWidgetItem();

            l_entityItem->setText(0, i.first->m_InstanceName.c_str());
            // Data slot 0 is ComponentType (-1 as the entity), slot 1 is the component ptr
            l_entityItem->setData(0, Qt::UserRole, QVariant(-1));
            l_entityItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)i.first));

            addChild(m_rootItem, l_entityItem);

            for (auto& j : i.second)
            {
                QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();
                l_componentItem->setText(0, j.second->m_InstanceName.c_str());

                l_componentItem->setData(0, Qt::UserRole, QVariant(j.first));
                l_componentItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)j.second));
                addChild(l_entityItem, l_componentItem);
            }
        }
    }
}

void WorldExplorer::initialize(PropertyEditor* propertyEditor)
{
    m_propertyEditor = propertyEditor;

    buildTree();

    f_sceneLoadingFinishCallback = [&]()
    {
        clear();
        buildTree();
    };

    g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishCallback, 2);
}

void WorldExplorer::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
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

void WorldExplorer::addChild(QTreeWidgetItem *parent, QTreeWidgetItem *child)
{
    parent->addChild(child);
}

void WorldExplorer::startRename()
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

void WorldExplorer::endRename()
{
    auto l_componentType = m_currentEditingItem->data(0, Qt::UserRole).toInt();

    if (l_componentType != -1)
    {
        auto l_componentPtr = reinterpret_cast<Component*>(m_currentEditingItem->data(1, Qt::UserRole).value<void*>());
        l_componentPtr->m_InstanceName = (m_currentEditingItem->text(0).toStdString() + "/").c_str();
    }
    else
    {
        auto l_entityPtr = reinterpret_cast<Entity*>(m_currentEditingItem->data(1, Qt::UserRole).value<void*>());
        l_entityPtr->m_InstanceName = (m_currentEditingItem->text(0).toStdString() + "/").c_str();
    }

    m_currentEditingItem->setFlags(m_currentEditingItem->flags() & ~Qt::ItemIsEditable);
    disconnect(this, SIGNAL(itemSelectionChanged()), this, SLOT(endRename()));
}

void WorldExplorer::addEntity()
{
    auto l_entity = g_Engine->Get<EntityManager>()->Spawn(true, ObjectLifespan::Scene, "NewEntity/");

    QTreeWidgetItem* l_entityItem = new QTreeWidgetItem();

    l_entityItem->setText(0, l_entity->m_InstanceName.c_str());
    l_entityItem->setData(0, Qt::UserRole, QVariant(-1));
    l_entityItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)l_entity));

    addChild(m_rootItem, l_entityItem);
    this->setCurrentItem(l_entityItem);
    startRename();
}

void WorldExplorer::deleteEntity()
{
    auto l_items = this->selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];

        auto l_entityPtr = reinterpret_cast<Entity*>(item->data(1, Qt::UserRole).value<void*>());

        for (int i = 0; i < item->childCount(); i++)
        {
            auto l_childItem = item->child(i);
            auto l_componentPtr = reinterpret_cast<Component*>(l_childItem->data(1, Qt::UserRole).value<void*>());
            destroyComponent(l_componentPtr);
        }

        g_Engine->Get<EntityManager>()->Destroy(l_entityPtr);

        item->parent()->removeChild(item);
    }
}

template<class T>
T* WorldExplorer::addComponent()
{
    auto l_items = selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];
        auto l_entityPtr = reinterpret_cast<Entity*>(item->data(1, Qt::UserRole).value<void*>());

        auto l_componentPtr = g_Engine->Get<ComponentManager>()->Spawn<T>(l_entityPtr, true, ObjectLifespan::Scene);

        QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();

        l_componentItem->setText(0, l_componentPtr->m_InstanceName.c_str());
        l_componentItem->setData(0, Qt::UserRole, QVariant(T::GetTypeID()));
        l_componentItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)l_componentPtr));

        addChild(item, l_componentItem);
        setCurrentItem(l_componentItem);
        startRename();

        return l_componentPtr;
    }

    return nullptr;
}

void WorldExplorer::addTransformComponent()
{
    auto l_componentPtr = addComponent<TransformComponent>();
    auto l_rootTranformComponent = static_cast<ITransformSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<TransformComponent>())->GetRootTransformComponent();
    l_componentPtr->m_parentTransformComponent = const_cast<TransformComponent*>(l_rootTranformComponent);
}

void WorldExplorer::addModelComponent()
{
    addComponent<ModelComponent>();
}

void WorldExplorer::addLightComponent()
{
    addComponent<LightComponent>();
}

void WorldExplorer::addCameraComponent()
{
    addComponent<CameraComponent>();
}

void WorldExplorer::destroyComponent(Component *component)
{
    // @TODO: Make this working again
    // if (dynamic_cast<TransformComponent*>(component))
    //     g_Engine->Get<ComponentManager>()->Destroy<TransformComponent>(component);
}

void WorldExplorer::deleteComponent()
{
    auto l_items = this->selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];

        auto l_componentPtr = reinterpret_cast<Component*>(item->data(1, Qt::UserRole).value<void*>());
        destroyComponent(l_componentPtr);

        item->parent()->removeChild(item);
    }
}

void WorldExplorer::onCustomContextMenuRequested(const QPoint& pos)
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

void WorldExplorer::showContextMenu(QTreeWidgetItem* item, const QPoint& globalPos)
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
            addCompoentMenu->addAction("Add ModelComponent", this, SLOT(addModelComponent()));
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

void WorldExplorer::showGeneralMenu(const QPoint &globalPos)
{
    QMenu menu;
    menu.addAction("General menu");
    menu.exec(globalPos);
}

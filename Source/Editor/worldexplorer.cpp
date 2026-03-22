#include "worldexplorer.h"

#include "../Engine/Engine.h"
#include "../Engine/Common/ComponentHeaders.h"
#include "../Engine/Services/SceneService.h"
#include "../Engine/Services/EntityRegistry.h"
#include "../Engine/Services/IGraphicsService.h"
#include "../Engine/Services/CameraService.h"
#include "../Engine/Services/ComponentManager.h" // TODO Phase2-migrate: Task 14 — keep until TextureComponent and AnimationComponent are migrated
#include <QHeaderView>

using namespace Inno;

WorldExplorer::WorldExplorer(QWidget* parent) : QTreeWidget(parent)
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            SLOT(onCustomContextMenuRequested(const QPoint&)));
    
    // Set better row height for readability
    this->setUniformRowHeights(true);
    this->setItemsExpandable(true);
    this->setRootIsDecorated(true);
    
    // Set larger row height and proper styling for input fields
    this->setStyleSheet(
        "QTreeWidget::item { "
        "    height: 32px; "
        "    padding: 4px; "
        "} "
        "QTreeWidget::item:edit { "
        "    height: 32px; "
        "    padding: 4px; "
        "    font-size: 12px; "
        "}"
    );
    
    // Set column width to make input field wider
    this->setColumnCount(1);
    this->header()->setStretchLastSection(true);
    this->setColumnWidth(0, 300); // Set wider column width for better text input
}

void WorldExplorer::buildTree()
{
    m_rootItem = new QTreeWidgetItem(this);
    m_rootItem->setText(0, "Entities");
    this->addTopLevelItem(m_rootItem);

    auto l_sceneHierarchyMap = g_Engine->Get<SceneService>()->getSceneHierarchyMap();

    auto l_registry = g_Engine->Get<EntityRegistry>();
    for (auto& i : l_sceneHierarchyMap)
    {
        if (l_registry->IsValid(i.first))
        {
            QTreeWidgetItem* l_entityItem = new QTreeWidgetItem();

            l_entityItem->setText(0, l_registry->GetName(i.first));
            l_entityItem->setData(0, Qt::UserRole, QVariant(-1));
            l_entityItem->setData(1, Qt::UserRole, QVariant::fromValue((uint32_t)i.first));

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
        // Renaming a component — component items still store void* Component* (old system, Task 13 scope)
        auto l_componentPtr = reinterpret_cast<Component*>(m_currentEditingItem->data(1, Qt::UserRole).value<void*>());
        l_componentPtr->m_InstanceName = (m_currentEditingItem->text(0).toStdString() + "/").c_str();
    }
    else
    {
        // TODO Phase2-migrate: EntityRegistry has no SetName API; entity rename is display-only until SetName is added
        // Entity items store EntityID as uint32_t in slot 1 — no direct name mutation possible here
        Log(Warning, "WorldExplorer: entity rename is display-only; EntityRegistry::SetName not yet implemented.");
    }

    m_currentEditingItem->setFlags(m_currentEditingItem->flags() & ~Qt::ItemIsEditable);
    disconnect(this, SIGNAL(itemSelectionChanged()), this, SLOT(endRename()));
}

void WorldExplorer::addEntity()
{
    auto l_entityID = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Scene, "NewEntity/");

    QTreeWidgetItem* l_entityItem = new QTreeWidgetItem();

    std::string l_displayName = g_Engine->Get<EntityRegistry>()->GetName(l_entityID);
    if (!l_displayName.empty() && l_displayName.back() == '/') l_displayName.pop_back();
    l_entityItem->setText(0, l_displayName.c_str());
    l_entityItem->setData(0, Qt::UserRole, QVariant(-1));
    l_entityItem->setData(1, Qt::UserRole, QVariant::fromValue((uint32_t)l_entityID));

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

        for (int i = 0; i < item->childCount(); i++)
        {
            auto l_childItem = item->child(i);
            auto l_componentPtr = reinterpret_cast<Component*>(l_childItem->data(1, Qt::UserRole).value<void*>());
            // Select the child item so destroyComponent reads the right type and entityID from selectedItems()
            setCurrentItem(l_childItem);
            destroyComponent(l_componentPtr);
        }

        g_Engine->Get<EntityRegistry>()->Destroy((EntityID)item->data(1, Qt::UserRole).toUInt());

        item->parent()->removeChild(item);
    }
}

template<class T>
T* WorldExplorer::addComponent()
{
    // TODO Phase2-migrate: Task 14 — migrate addComponent to EntityRegistry::Emplace<T> when ComponentManager is deleted
    auto l_items = selectedItems();
    QTreeWidgetItem* item;
    if (l_items.count() != 0)
    {
        item = l_items[0];
        auto l_entityPtr = reinterpret_cast<Entity*>(item->data(1, Qt::UserRole).value<void*>());

        auto l_componentPtr = g_Engine->Get<ComponentManager>()->Spawn<T>(l_entityPtr, true, ObjectLifespan::Scene);

        QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();

        l_componentItem->setText(0, T::GetTypeName());
        l_componentItem->setData(0, Qt::UserRole, QVariant(T::GetTypeID()));
        l_componentItem->setData(1, Qt::UserRole, QVariant::fromValue((void*)l_componentPtr));

        addChild(item, l_componentItem);
        setCurrentItem(l_componentItem);
        startRename();

        l_componentPtr->m_ObjectStatus = ObjectStatus::Activated;
        return l_componentPtr;
    }

    return nullptr;
}

void WorldExplorer::addLightComponent()
{
    Log(Warning, "WorldExplorer: addLightComponent not yet migrated to EntityRegistry.");
}

void WorldExplorer::addCameraComponent()
{
    Log(Warning, "WorldExplorer: addCameraComponent not yet migrated to EntityRegistry.");
}

void WorldExplorer::destroyComponent(Component *component)
{
    if (!component)
    {
        return;
    }

    auto l_items = this->selectedItems();
    if (l_items.count() == 0)
        return;

    auto item = l_items[0];
    auto componentType = item->data(0, Qt::UserRole).toInt();
    auto l_entityID = (EntityID)item->parent()->data(1, Qt::UserRole).toUInt();

    if (componentType == LightComponent::GetTypeID())
    {
        g_Engine->Get<EntityRegistry>()->Remove<LightComponent>(l_entityID);
    }
    else if (componentType == CameraComponent::GetTypeID())
    {
        g_Engine->Get<EntityRegistry>()->Remove<CameraComponent>(l_entityID);
    }
    // TODO Phase2-migrate: else if (componentType == MeshComponent::GetTypeID())
    // TODO Phase2-migrate: { g_Engine->Get<ComponentManager>()->Destroy(reinterpret_cast<MeshComponent*>(component)); }
    // TODO Phase2-migrate: else if (componentType == MaterialComponent::GetTypeID())
    // TODO Phase2-migrate: { g_Engine->Get<ComponentManager>()->Destroy(reinterpret_cast<MaterialComponent*>(component)); }
    else if (componentType == TextureComponent::GetTypeID())
    {
        g_Engine->getGraphicsService()->Delete(reinterpret_cast<TextureComponent*>(component));
    }
    // TODO Phase2-migrate: else if (componentType == SkeletonComponent::GetTypeID())
    // TODO Phase2-migrate: { g_Engine->Get<ComponentManager>()->Destroy(reinterpret_cast<SkeletonComponent*>(component)); }
    else if (componentType == AnimationComponent::GetTypeID())
    {
        g_Engine->Get<EntityRegistry>()->Remove<AnimationComponent>(l_entityID);
    }
    else
    {
        // Unknown component type - log warning or handle gracefully
    }
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

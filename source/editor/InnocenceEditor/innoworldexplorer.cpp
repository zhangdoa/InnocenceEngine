#include "innoworldexplorer.h"

#include "../../engine/system/ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

InnoWorldExplorer::InnoWorldExplorer(QWidget* parent) : QTreeWidget(parent)
{
}

void InnoWorldExplorer::initialize()
{
    // root item
    m_rootItem = new QTreeWidgetItem(this);
    m_rootItem->setText(0, "Entities");

    this->addTopLevelItem(m_rootItem);

    auto l_entityNameMap = g_pCoreSystem->getGameSystem()->getEntityNameMap();
    auto l_entityChildrenComponentsMetadataMap = g_pCoreSystem->getGameSystem()->getEntityChildrenComponentsMetadataMap();

    for (auto i : l_entityNameMap)
    {
        QTreeWidgetItem* l_entityItem = new QTreeWidgetItem();
        l_entityItem->setText(0, i.second.c_str());
        AddChild(m_rootItem, l_entityItem);

        auto result = l_entityChildrenComponentsMetadataMap.find(i.first);
        if (result != l_entityChildrenComponentsMetadataMap.end())
        {
            auto& l_componentMetadataMap = result->second;

            for (auto& j : l_componentMetadataMap)
            {
                auto& l_componentMetapair = j.second;
                QTreeWidgetItem* l_componentItem = new QTreeWidgetItem();
                l_componentItem->setText(0, l_componentMetapair.second.c_str());
                l_componentItem->setData(0, Qt::UserRole, QVariant((int)l_componentMetapair.first));
                AddChild(l_entityItem, l_componentItem);
            }
        }
    }
}

void InnoWorldExplorer::AddChild(QTreeWidgetItem *parent, QTreeWidgetItem *child)
{
    parent->addChild(child);
}

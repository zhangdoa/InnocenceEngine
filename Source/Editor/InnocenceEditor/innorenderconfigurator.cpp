#include "innorenderconfigurator.h"
#include <QLineEdit>
#include <QListView>

extern ICoreSystem* g_pCoreSystem;

InnoRenderConfigurator::InnoRenderConfigurator(QWidget* parent) : QComboBox(parent)
{
    m_rows = 6;
    m_model = new QStandardItemModel(m_rows, 1);
    this->setModel(m_model);

    AddCheckItem(0, "Use Motion Blur");
    AddCheckItem(1, "Use TAA");
    AddCheckItem(2, "Use Bloom");
    AddCheckItem(3, "Draw Sky");
    AddCheckItem(4, "Draw Terrain");
    AddCheckItem(5, "Draw Debug Object");

    connect((QListView*) view(), SIGNAL(pressed(QModelIndex)), this, SLOT(OnItemPressed(QModelIndex)));

    this->setEditable(true);
}

InnoRenderConfigurator::~InnoRenderConfigurator()
{
    delete m_model;
}

void InnoRenderConfigurator::showPopup()
{
    GetRenderConfig();
    QComboBox::showPopup();
}

void InnoRenderConfigurator::initialize()
{
    GetRenderConfig();
}

void InnoRenderConfigurator::OnItemPressed(const QModelIndex &index)
{
    if (!m_model)
    {
        return;
    }

    QStandardItem* item = m_model->itemFromIndex(index);
    item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    SetRenderConfig();
}

void InnoRenderConfigurator::AddCheckItem(int row, const QString& text)
{
    if (!m_model)
    {
        return;
    }

    QStandardItem* item = new QStandardItem(text);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);

    m_model->setItem(row, 0, item);
}

void InnoRenderConfigurator::SetRenderConfig()
{
    if (!m_model)
    {
        return;
    }

    std::vector<bool> l_status(m_rows);

    for (unsigned int i = 0; i < m_rows; i++)
    {
        auto checkState = m_model->item(i)->checkState();
        l_status[i] = (checkState == Qt::Checked);
    }

    RenderingConfig l_renderingConfig;

    l_renderingConfig.useMotionBlur = l_status[0];
    l_renderingConfig.useTAA = l_status[1];
    l_renderingConfig.useBloom = l_status[2];
    l_renderingConfig.drawSky = l_status[3];
    l_renderingConfig.drawTerrain = l_status[4];
    l_renderingConfig.drawDebugObject = l_status[5];

    g_pCoreSystem->getRenderingFrontend()->setRenderingConfig(l_renderingConfig);
}

void InnoRenderConfigurator::GetRenderConfig()
{
    if (!m_model)
    {
        return;
    }

    auto l_renderingConfig = g_pCoreSystem->getRenderingFrontend()->getRenderingConfig();

    m_model->item(0)->setCheckState(l_renderingConfig.useMotionBlur ? Qt::Checked : Qt::Unchecked);
    m_model->item(1)->setCheckState(l_renderingConfig.useTAA ? Qt::Checked : Qt::Unchecked);
    m_model->item(2)->setCheckState(l_renderingConfig.useBloom ? Qt::Checked : Qt::Unchecked);
    m_model->item(3)->setCheckState(l_renderingConfig.drawSky ? Qt::Checked : Qt::Unchecked);
    m_model->item(4)->setCheckState(l_renderingConfig.drawTerrain ? Qt::Checked : Qt::Unchecked);
    m_model->item(5)->setCheckState(l_renderingConfig.drawDebugObject ? Qt::Checked : Qt::Unchecked);
}

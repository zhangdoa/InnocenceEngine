#ifndef VISIBLECOMPONENTPROPERTYEDITOR_H
#define VISIBLECOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include "icomponentpropertyeditor.h"
#include "transformwidget.h"
#include "../Engine/Component/ModelComponent.h"
#include "materialComponentpropertyeditor.h"
#include "directoryviewer.h"

enum class AssetType
{
    Mesh,
    Material
};

class ModelComponentPropertyEditor : public IComponentPropertyEditor
{
	Q_OBJECT
public:
	ModelComponentPropertyEditor();

	void initialize() override;
	void edit(void* component) override;

	void RefreshDrawCallList();

private:
	TransformWidget* m_transformWidget;

	QLabel* m_modelNameLabel;
	QPushButton* m_addMeshButton;
	QPushButton* m_addMaterialButton;
	QPushButton* m_removeSelectedButton;
	QLabel* m_drawCallListLabel;
	QTableWidget* m_drawCallList;

	MaterialComponentPropertyEditor* m_MaterialCompEditor;
	DirectoryViewer* m_dirViewer;
	AssetType m_currentAssetType;

    Inno::ModelComponent* m_component;
    
    void CreateDrawCall(const QString& assetPath, bool isMesh);
    void RemoveDrawCall(int row);

public slots:
	void SetTransform();
	void remove() override;
	void AddMesh();
	void AddMaterial();
	void RemoveSelected();
	void onFileSelected(const QString& filePath);

private slots:
	void onCustomContextMenuRequested(const QPoint& pos);
	void showContextMenu(const QPoint& globalPos);
	void tableItemClicked(int row, int column);
};

#endif // VISIBLECOMPONENTPROPERTYEDITOR_H

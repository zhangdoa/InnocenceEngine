#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "IRenderingSystem.h"
#include "DXHeaders.h"
#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/DXMeshDataComponent.h"
#include "../../component/DXTextureDataComponent.h"

class DXRenderingSystem : public IRenderingSystem
{
public:
	InnoHighLevelSystem_EXPORT bool setup() override;
	InnoHighLevelSystem_EXPORT bool initialize() override;
	InnoHighLevelSystem_EXPORT bool update() override;
	InnoHighLevelSystem_EXPORT bool terminate() override;

	InnoHighLevelSystem_EXPORT objectStatus getStatus();

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	DXMeshDataComponent* initializeMeshDataComponent(MeshDataComponent* rhs);
	DXTextureDataComponent* initializeTextureDataComponent(TextureDataComponent* rhs);

	DXMeshDataComponent* addDXMeshDataComponent(EntityID rhs);
	DXTextureDataComponent* addDXTextureDataComponent(EntityID rhs);
	DXMeshDataComponent* getDXMeshDataComponent(EntityID rhs);
	DXTextureDataComponent* getDXTextureDataComponent(EntityID rhs);

	void initializeFinalBlendPass();

	void initializeShader(shaderType shaderType, const std::wstring & shaderFilePath);
	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename);

	void updateFinalBlendPass();

	void drawMesh(EntityID rhs);
	void drawMesh(MeshDataComponent* MDC);

	void updateShaderParameter(shaderType shaderType, ID3D11Buffer* matrixBuffer, mat4* parameterValue);
	void beginScene(float r, float g, float b, float a);
	void endScene();
};

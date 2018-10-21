#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "IRenderingSystem.h"
#include "DXHeaders.h"
#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"

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

	void initializeMesh(MeshDataComponent* rhs);
	void initializeTexture(TextureDataComponent* rhs);

	void initializeFinalBlendPass();

	void initializeShader(shaderType shaderType, const std::wstring & shaderFilePath);
	void OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, const std::string & shaderFilename);

	void updateFinalBlendPass();

	void updateShaderParameter(shaderType shaderType, ID3D11Buffer* matrixBuffer, mat4* parameterValue);
	void beginScene(float r, float g, float b, float a);
	void endScene();
};

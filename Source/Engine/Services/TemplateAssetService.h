#pragma once
#include "../Interface/ISystem.h"

#include "../RenderingServer/IRenderingServer.h"

#include "../Component/MeshComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/SkeletonComponent.h"
#include "../Component/AnimationComponent.h"

#include "../Common/GPUDataStructure.h"

namespace Inno
{
    enum class WorldEditorIconType { DIRECTIONAL_LIGHT, POINT_LIGHT, SPHERE_LIGHT, UNKNOWN };
    
	struct TemplateAssetServiceImpl;
    class TemplateAssetService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(TemplateAssetService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

    	MeshComponent* GetMeshComponent(ProceduralMeshShape shape);
		TextureComponent* GetTextureComponent(WorldEditorIconType iconType);
		MaterialComponent* GetDefaultMaterialComponent();

    private:
        TemplateAssetServiceImpl* m_Impl;
    };
}
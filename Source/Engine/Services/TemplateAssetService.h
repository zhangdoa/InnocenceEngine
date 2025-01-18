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

    	MeshComponent* GetMeshComponent(MeshShape shape);
		TextureComponent* GetTextureComponent(WorldEditorIconType iconType);
		MaterialComponent* GetDefaultMaterialComponent();
		
		bool GenerateMesh(MeshShape shape, MeshComponent* meshComponent);
		void FulfillVerticesAndIndices(MeshComponent* meshComponent, const std::vector<Index>& indices, const std::vector<Vec3>& vertices, uint32_t verticesPerFace);

    private:
        TemplateAssetServiceImpl* m_Impl;
    };
}
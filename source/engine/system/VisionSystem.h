#pragma once
#include<atomic>

#include "interface/IVisionSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"
#include "interface/IGameSystem.h"
#include "interface/IPhysicsSystem.h"

#include "common/ComponentHeaders.h"

#include "GLWindowSystem.h"

#include "GLRenderingSystem.h"
#include "DXRenderingSystem.h"

#include "GLGuiSystem.h"

extern IMemorySystem* g_pMemorySystem;
extern ILogSystem* g_pLogSystem;
extern IGameSystem* g_pGameSystem;
extern IPhysicsSystem* g_pPhysicsSystem;

class VisionSystem : public IVisionSystem
{
public:
	VisionSystem() {};
	~VisionSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void setWindowName(const std::string& windowName) override;

	const objectStatus& getStatus() const override;

private:
	void setupWindow();
	void setupRendering();
	void setupGui();

	void updatePhysics();
	
	void changeDrawPolygonMode();
	void changeDrawTextureMode();
	void changeShadingMode();

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	//Window data
	IWindowSystem* m_WindowSystem;

	//Physics data
	Ray m_mouseRay;

	//Rendering Data
	std::atomic<bool> m_canRender;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;
	std::function<void()> f_changeShadingMode;

	IRenderingSystem* m_RenderingSystem;

	int m_polygonMode = 2;
	int m_textureMode = 0;
	int m_shadingMode = 0;

	//Gui data
	IGuiSystem* m_GuiSystem;
};

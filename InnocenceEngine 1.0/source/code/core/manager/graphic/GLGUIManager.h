//#pragma once
//#include "../../interface/IEventManager.h"
//#include "GLWindowManager.h"
//#include "../../third-party/nuklear.h"
//#include "../../third-party/nuklear_glfw_gl3.h"
//
//class GLGUIManager : public IEventManager
//{
//public:
//	~GLGUIManager();
//
//	static GLGUIManager& getInstance()
//	{
//		static GLGUIManager instance;
//		return instance;
//	}
//
//private:
//	GLGUIManager();
//
//	struct nk_context* ctx;
//	struct nk_color background;
//	struct nk_font_atlas* atlas;
//
//	void initialize() override;
//	void update() override;
//	void shutdown() override;
//};
//

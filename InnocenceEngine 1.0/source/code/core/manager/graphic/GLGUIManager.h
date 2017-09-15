#pragma once
#include "../../interface/IEventManager.h"
#include "GLWindowManager.h"
#include "../../third-party/nuklear.h"
#include "../../third-party/nuklear_glfw_gl3.h"

class GLGUIManager : public IEventManager
{
public:
	~GLGUIManager();

	static GLGUIManager& getInstance()
	{
		static GLGUIManager instance;
		return instance;
	}

	struct nk_context* ctx;
	struct nk_color background;
	struct nk_font_atlas* atlas;

private:
	GLGUIManager();

	enum { EASY, HARD };
	 int op = EASY;
	 int property = 20;
	 int r = 255;
	 int g = 255;
	 int b = 255;

	void initialize() override;
	void update() override;
	void shutdown() override;
};


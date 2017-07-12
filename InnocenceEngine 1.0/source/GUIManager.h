#pragma once
#include "IEventManager.h"
#include "WindowManager.h"

class GUIManager : public IEventManager
{
public:
	~GUIManager();

	static GUIManager& getInstance()
	{
		static GUIManager instance;
		return instance;
	}

	struct nk_context *ctx;
	struct nk_color* background;
	struct nk_font_atlas *atlas;

	glm::vec3 getColor() const;

private:
	GUIManager();


	void init() override;
	void update() override;
	void shutdown() override;
};


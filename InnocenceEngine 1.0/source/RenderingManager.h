#pragma once
#include "IEventManager.h"
class RenderingManager : public IEventManager
{
public:
	RenderingManager();
	~RenderingManager();

private:
	GLuint VertexArrayID;
	GLuint vertexbuffer;

	void init() override;
	void update() override;
	void shutdown() override;
};


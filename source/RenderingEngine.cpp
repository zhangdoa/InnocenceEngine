#include "RenderingEngine.h"
#include "GameObject.h"
#include "CameraComponent.h"
#include "BaseLightComponent.h"


RenderingEngine::RenderingEngine()
{
	init();

}


RenderingEngine::~RenderingEngine()
{
	shutdown();
}

void RenderingEngine::init() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_TEXTURE_2D);
	
}

void RenderingEngine::update()
{
}

void RenderingEngine::render(GameObject* object) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	_lights.clear();

	object->addToRenderingEngine(this);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(false);
	glDepthFunc(GL_EQUAL);

	for (BaseLightComponent* light : _lights) {

		_activeLight = light;

		object->render(light->getShader(), this);

	}

	glDepthFunc(GL_LESS);
	glDepthMask(true);
	glDisable(GL_BLEND);

}

void RenderingEngine::shutdown() {
	
}

void RenderingEngine::addCamera(CameraComponent* camera)
{
	_mainCamera = camera;
}

void RenderingEngine::addLight(BaseLightComponent* activeLight)
{
	_lights.push_back(activeLight);
}

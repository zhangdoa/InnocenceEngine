#include "RenderingEngine.h"
#include "GameObject.h"
#include "BaseLightComponent.h"

Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::setProjection(Mat4f * projection)
{
	_projection = projection;
}

void Camera::setTransform(Transform* transform)
{
	_transform = transform;
}

Mat4f Camera::getViewProjection()
{
	Mat4f cameraRotation = _transform->getTransformedRot().conjugate().toRotationMatrix();
	Vec3f cameraPos = _transform->getTransformedPos() * -1.0f;
	Mat4f cameraTranslation;
	cameraTranslation.initTranslation(cameraPos.getX(), cameraPos.getY(),
		cameraPos.getZ());
	return _projection->operator*(cameraRotation) * cameraTranslation;
}

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

	fprintf(stdout, "Rendering Engine has been initialized.\n");
	
}

void RenderingEngine::update()
{
}

void RenderingEngine::render(GameObject* object) {
	fprintf(stdout, "Rendering Engine starts rendering.\n");
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	_lights.clear();

	Shader* forwardAmbient = new ForwardAmbientShaderWrapper();
	object->render(forwardAmbient);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(false);
	glDepthFunc(GL_EQUAL);
	fprintf(stdout, "GL blend finished.\n");
	for (BaseLightComponent* light : _lights) {

		object->render(light->getShader());

	}

	glDepthFunc(GL_LESS);
	glDepthMask(true);
	glDisable(GL_BLEND);

}

void RenderingEngine::shutdown() {
	
}

void RenderingEngine::addLight(BaseLightComponent* activeLight)
{
	_lights.push_back(activeLight);
}

Camera * RenderingEngine::getMainCamera()
{
		return _mainCamera;
}

void RenderingEngine::setMainCamera(Camera * mainCamera)
{
	_mainCamera = mainCamera;
}

Vec3f * RenderingEngine::getAmbientLight()
{
	return &_ambientLight;
}

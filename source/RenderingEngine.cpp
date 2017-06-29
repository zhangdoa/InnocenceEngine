#include "RenderingEngine.h"
#include "GameObject.h"
#include "CameraComponent.h"
#include "BaseLightComponent.h"

Camera::Camera()
{
	_projection.initPerspective(90, 16.0f / 9.0f, 0.1f, 1000.0f);
}

Camera::Camera(float fov, float aspectRatio, float zNear, float zFar)
{
	_projection.initPerspective(fov, aspectRatio, zNear, zFar);
}


Camera::~Camera()
{
}

Mat4f Camera::getViewProjection()
{
	GameObject* cameraObject = new GameObject();
	Mat4f cameraRotation = cameraObject->getParent()->caclTransformedRot().conjugate().toRotationMatrix();
	Vec3f cameraPos = cameraObject->getParent()->caclTransformedPos() * -1.0f;
	Mat4f cameraTranslation;
	cameraTranslation.initTranslation(cameraPos.getX(), cameraPos.getY(),
		cameraPos.getZ());
	return _projection * cameraRotation * cameraTranslation;
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

	//glFrontFace(GL_CW);
	//glCullFace(GL_BACK);
	//glEnable(GL_CULL_FACE);
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_DEPTH_CLAMP);
	//glEnable(GL_TEXTURE_2D);

	//std::cout << "Rendering Engine has been initialized." << std::endl;
	_ambientLight = Vec3f(0.5f, 0.3f, 0.1f);
	
}

void RenderingEngine::update()
{
}

void RenderingEngine::render(GameObject* object) {
	//std::cout << "Rendering Engine is rendering." << std::endl;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	_lights.clear();

	object->render(&basicShader);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(false);
	glDepthFunc(GL_EQUAL);

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

CameraComponent* RenderingEngine::getMainCamera()
{
	return _mainCamera;
}

void RenderingEngine::setMainCamera(CameraComponent * camera)
{
	_mainCamera = camera;

}

const Vec3f& RenderingEngine::getAmbientLight()
{
	return _ambientLight;
}


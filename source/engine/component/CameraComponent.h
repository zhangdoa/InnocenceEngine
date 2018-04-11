#pragma once
#include "BaseComponent.h"

class CameraComponent : public BaseComponent
{
public:
	CameraComponent();
	~CameraComponent();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	mat4 getPosMatrix() const;
	mat4 getRotMatrix() const;
	mat4 getProjectionMatrix() const;
	std::vector<Vertex>* getFrustumCorners();

	Ray m_rayOfEye;
	bool m_drawRay = false;

private:
	mat4 m_projectionMatrix;
	std::vector<Vertex> m_frustumCorners;
};

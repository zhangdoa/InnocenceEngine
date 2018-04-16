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

	mat4 getInvertTranslationMatrix() const;
	mat4 getInvertRotationMatrix() const;
	mat4 getProjectionMatrix() const;
	mat4 m_initTransMat;
	std::vector<Vertex>* getFrustumCorners();

	double m_FOV = 60.0;
	double m_WHRatio = 16.0 / 9.0;
	double m_zNear = 0.1;
	double m_zFar = 1000.0;

	Ray m_rayOfEye;
	bool m_drawRay = false;
	bool m_drawFrustum = false;
	bool m_drawAABB = false;
	meshID m_FrustumMeshID;
	std::vector<Vertex> m_frustumVertices;
	std::vector<unsigned int> m_frustumIndices;
	AABB m_AABB;
	meshID m_AABBMeshID;

private:
	mat4 m_projectionMatrix;
	void caclFrustumVertices();
};

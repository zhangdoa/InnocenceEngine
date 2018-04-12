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

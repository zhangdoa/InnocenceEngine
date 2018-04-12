#include "CameraComponent.h"

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

mat4 CameraComponent::getInvertTranslationMatrix() const
{
	return getParentEntity()->caclWorldPos().scale(-1.0).toTranslationMatrix();
}

mat4 CameraComponent::getInvertRotationMatrix() const
{
	return getParentEntity()->caclWorldRot().quatConjugate().toRotationMatrix();
}

mat4 CameraComponent::getProjectionMatrix() const
{
	return m_projectionMatrix;
}

void CameraComponent::caclFrustumVertices()
{
	Vertex l_VertexData_1;
	l_VertexData_1.m_pos = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_1.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_2;
	l_VertexData_2.m_pos = vec4(1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_2.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_3;
	l_VertexData_3.m_pos = vec4(-1.0f, -1.0f, 1.0f, 1.0f);
	l_VertexData_3.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_4;
	l_VertexData_4.m_pos = vec4(-1.0f, 1.0f, 1.0f, 1.0f);
	l_VertexData_4.m_texCoord = vec2(0.0f, 1.0f);

	Vertex l_VertexData_5;
	l_VertexData_5.m_pos = vec4(1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_5.m_texCoord = vec2(1.0f, 1.0f);

	Vertex l_VertexData_6;
	l_VertexData_6.m_pos = vec4(1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_6.m_texCoord = vec2(1.0f, 0.0f);

	Vertex l_VertexData_7;
	l_VertexData_7.m_pos = vec4(-1.0f, -1.0f, -1.0f, 1.0f);
	l_VertexData_7.m_texCoord = vec2(0.0f, 0.0f);

	Vertex l_VertexData_8;
	l_VertexData_8.m_pos = vec4(-1.0f, 1.0f, -1.0f, 1.0f);
	l_VertexData_8.m_texCoord = vec2(0.0f, 1.0f);

	m_frustumVertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

	m_frustumIndices = { 0, 1, 3, 1, 2, 3,
		4, 5, 0, 5, 1, 0,
		7, 6, 4, 6, 5, 4,
		3, 2, 7, 2, 6 ,7,
		4, 0, 7, 0, 3, 7,
		1, 5, 2, 5, 6, 2 };

	auto pCamera = mat4();
	pCamera.initializeToPerspectiveMatrix((45.0 / 180.0) * PI, (16.0 / 9.0), 0.1, 10.0);

	for (auto& l_vertexData : m_frustumVertices)
	{
		vec4 l_mulPos;
		l_mulPos = l_vertexData.m_pos;
		// from projection space to view space
		l_mulPos = pCamera.inverse() * l_mulPos;
		// perspective division
		l_mulPos = l_mulPos * (1.0 / l_mulPos.w);
		// from view space to world space
		l_mulPos = getParentEntity()->caclWorldRot().toRotationMatrix() * l_mulPos;
		l_mulPos = getParentEntity()->caclWorldPos().toTranslationMatrix() * l_mulPos;
		l_vertexData.m_pos = l_mulPos;
	}

	for (auto& l_vertexData : m_frustumVertices)
	{
		l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0).normalize();
	}
}

std::vector<Vertex>* CameraComponent::getFrustumCorners()
{
	return &m_frustumVertices;
}

void CameraComponent::setup()
{
	m_projectionMatrix.initializeToPerspectiveMatrix((60.0 / 180.0) * PI, (16.0 / 9.0), 0.01, 10000.0);
	m_rayOfEye.m_origin = getParentEntity()->caclWorldPos();
	m_rayOfEye.m_direction = getParentEntity()->getTransform()->getDirection(Transform::direction::FORWARD);
	caclFrustumVertices();
	m_initTransMat = getParentEntity()->caclTransformationMatrix();
}

void CameraComponent::initialize()
{
}

void CameraComponent::update()
{
	m_rayOfEye.m_origin = getParentEntity()->caclWorldPos();
	m_rayOfEye.m_direction = getParentEntity()->getTransform()->getDirection(Transform::direction::FORWARD);
	caclFrustumVertices();
}

void CameraComponent::shutdown()
{
}


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

private:
	mat4 m_projectionMatrix;
};

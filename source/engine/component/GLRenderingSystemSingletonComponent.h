#pragma once
#include "BaseComponent.h"

class GLRenderingSystemSingletonComponent : public BaseComponent
{
public:
	~GLRenderingSystemSingletonComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;
	
	static GLRenderingSystemSingletonComponent& getInstance()
	{
		static GLRenderingSystemSingletonComponent instance;
		return instance;
	}

	bool m_shouldUpdateEnvironmentMap = true;

private:
	GLRenderingSystemSingletonComponent() {};
};

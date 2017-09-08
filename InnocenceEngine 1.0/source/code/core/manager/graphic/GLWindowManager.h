#pragma once
#include "../../manager/IEventManager.h"
#include "../../manager/LogManager.h"
#include "GUIManager.h"

class GLWindowManager : public IEventManager
{
public:
	~GLWindowManager();

	static GLWindowManager& getInstance()
	{
		static GLWindowManager instance;
		return instance;
	}

	GLFWwindow* getWindow() const;
	glm::vec2 getScreenCenterPosition() const;
	void setWindowName(const std::string& windowName);

private:
	GLWindowManager();

	const unsigned int SCR_WIDTH = 1024;
	const unsigned int SCR_HEIGHT = 768;
	GLFWwindow* m_window;
	std::string m_windowName;

	void init() override;
	void update() override;
	void shutdown() override;
};


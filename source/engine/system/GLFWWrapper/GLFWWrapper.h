#include "../../common/InnoType.h"
#include "GLFW/glfw3.h"

class GLFWWrapper
{
public:
	~GLFWWrapper() {};

	static GLFWWrapper& get()
	{
		static GLFWWrapper instance;
		return instance;
	}
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	void swapBuffer();
	void hideMouseCursor();
	void showMouseCursor();

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	ButtonStatusMap getButtonStatus();

	GLFWwindow* m_window;

private:
	GLFWWrapper() {};
};
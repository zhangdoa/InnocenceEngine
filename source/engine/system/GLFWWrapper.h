#include "../system/GLHeaders.h"
#include "IInputSystem.h"

class windowCallbackWrapper
{
public:
	~windowCallbackWrapper() {};

	static windowCallbackWrapper& getInstance()
	{
		static windowCallbackWrapper instance;
		return instance;
	}
	void initialize(GLFWwindow * window, IInputSystem* rhs);

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, float mouseXPos, float mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, float xoffset, float yoffset);

private:
	windowCallbackWrapper() {};
};
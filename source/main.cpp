#include <glad/gl.h>
#include <GLFW/glfw3.h>

int main()
{
	glfwInit();
	
	GLFWwindow* window = glfwCreateWindow(800, 600, "Window", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	int version = gladLoadGL(glfwGetProcAddress);
	
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glfwSwapBuffers(window);
	}

	return 0;
}
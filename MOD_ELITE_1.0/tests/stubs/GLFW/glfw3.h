// STUB apenas para checar sintaxe do nosso código (NÃO é o GLFW real).
#pragma once
struct GLFWwindow;
#define GLFW_PRESS 1
#define GLFW_RELEASE 0
#define GLFW_REPEAT 2
#define GLFW_KEY_ESCAPE 256
#define GL_COLOR_BUFFER_BIT 0x4000
int glfwInit(); void glfwTerminate();
GLFWwindow* glfwCreateWindow(int, int, const char*, void*, void*);
void glfwMakeContextCurrent(GLFWwindow*); void glfwSwapInterval(int); void glfwSwapBuffers(GLFWwindow*);
void glfwPollEvents(); void glfwWaitEventsTimeout(double); double glfwGetTime();
int glfwWindowShouldClose(GLFWwindow*); void glfwSetWindowShouldClose(GLFWwindow*, int);
GLFWwindow* glfwGetCurrentContext(); void glfwGetWindowSize(GLFWwindow*, int*, int*); void glfwGetFramebufferSize(GLFWwindow*, int*, int*);
void glfwDestroyWindow(GLFWwindow*);
typedef void (*GLFWkeyfun)(GLFWwindow*, int, int, int, int); typedef void (*GLFWscrollfun)(GLFWwindow*, double, double);
typedef void (*GLFWcursorposfun)(GLFWwindow*, double, double);
GLFWkeyfun glfwSetKeyCallback(GLFWwindow*, GLFWkeyfun); GLFWscrollfun glfwSetScrollCallback(GLFWwindow*, GLFWscrollfun);
GLFWcursorposfun glfwSetCursorPosCallback(GLFWwindow*, GLFWcursorposfun);
void glViewport(int, int, int, int); void glClearColor(float, float, float, float); void glClear(unsigned);

#pragma once

#include <iostream>
#include <cstdlib>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glslUtility.hpp"
#include "kernel.h"

// ====================================
// GL stuff
// ====================================

GLuint             m_pbo = (GLuint) NULL;
GLFWwindow*    m_window;
std::string        m_yourName;
unsigned int       m_width;
unsigned int       m_height;
int                m_major;
int                m_minor;
GLuint             m_positionLocation = 0;
GLuint             m_texCoordsLocation = 1;
GLuint             m_image;

// ====================================
// Main
// ====================================
int main(int argc, char* argv[]);

// ====================================
// Main loop
// ====================================
void mainLoop();
void errorCallback(int error, const char *description);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void runCUDA();

// ====================================
// Setup/init stuff
// ====================================
bool init(int argc, char **argv);
void initPBO(GLuint *pbo);
void initCUDA();
void initTextures();
void initVAO();
GLuint initShader();

// ====================================
// Clean-up stuff
// ====================================
void cleanupCUDA();
void deletePBO(GLuint *pbo);
void deleteTexture(GLuint *tex);

#include <cstdio>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include "main.hpp"

/**
 * C main function.
 */
int main(int argc, char* argv[]) {
    // TODO: Change this line to use your name!
    m_yourName = "TODO: YOUR NAME HERE";

    if (init(argc, argv)) {
        mainLoop();
    }

    return 0;
}

/**
 * Initialization of CUDA and GLFW.
 */
bool init(int argc, char **argv) {
    // Set window title to "Student Name: [SM 2.0] GPU Name"
    std::string deviceName;
    cudaDeviceProp deviceProp;
    int gpuDevice = 0;
    int device_count = 0;
    cudaGetDeviceCount(&device_count);
    if (gpuDevice > device_count) {
        std::cout << "Error: GPU device number is greater than the number of devices!" <<
                  "Perhaps a CUDA-capable GPU is not installed?" << std::endl;
        return false;
    }
    cudaGetDeviceProperties(&deviceProp, gpuDevice);
    m_major = deviceProp.major;
    m_minor = deviceProp.minor;

    std::ostringstream ss;
    ss << m_yourName << ": [SM " << m_major << "." << m_minor << "] " << deviceProp.name;
    deviceName = ss.str();

    // Window setup stuff
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        return false;
    }
    m_width = 800;
    m_height = 800;
    m_window = glfwCreateWindow(m_width, m_height, deviceName.c_str(), NULL, NULL);
    if (!m_window) {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_window);
    glfwSetKeyCallback(m_window, keyCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        return false;
    }

    // init all of the things
    initVAO();
    initTextures();
    initCUDA();
    initPBO(&m_pbo);

    GLuint passthroughProgram;
    passthroughProgram = initShader();
    glUseProgram(passthroughProgram);
    glActiveTexture(GL_TEXTURE0);

    return true;
}

void initPBO(GLuint *pbo) {
    if (pbo) {
        // set up vertex data parameter
        int num_texels = m_width * m_height;
        int num_values = num_texels * 4;
        size_t size_tex_data = sizeof(GLubyte) * num_values;

        // Generate a buffer ID called a PBO (Pixel Buffer Object)
        glGenBuffers(1, pbo);
        // Make this the current UNPACK buffer (OpenGL is state-based)
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, *pbo);
        // Allocate data for the buffer. 4-channel 8-bit image
        glBufferData(GL_PIXEL_UNPACK_BUFFER, size_tex_data, NULL, GL_DYNAMIC_COPY);
        cudaGLRegisterBufferObject(*pbo);
    }
}

void initVAO() {
    GLfloat vertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f,
        -1.0f,  1.0f,
    };

    GLfloat texCoords[] = {
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    GLushort indices[] = { 0, 1, 3, 3, 1, 2 };

    GLuint vertexBufferObjID[3];
    glGenBuffers(3, vertexBufferObjID);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)m_positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(m_positionLocation);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)m_texCoordsLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(m_texCoordsLocation);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBufferObjID[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void initCUDA() {
    // Default to device ID 0. If you have more than one GPU and want to test a non-default one,
    // change the device ID.
    cudaGLSetGLDevice(0);

    // Clean up on program exit
    atexit(cleanupCUDA);
}

void initTextures() {
    glGenTextures(1, &m_image);
    glBindTexture(GL_TEXTURE_2D, m_image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, NULL);
}

GLuint initShader() {
    const char *attributeLocations[] = { "Position", "Tex" };
    GLuint program = glslUtility::createDefaultProgram(attributeLocations, 2);
    GLint location;
    glUseProgram(program);
    if ((location = glGetUniformLocation(program, "u_image")) != -1) {
        glUniform1i(location, 0);
    }
    return program;
}

// ====================================
// Main loop stuff
// ====================================

void runCUDA() {
    // Map OpenGL buffer object for writing from CUDA on a single GPU
    // No data is moved (Win & Linux). When mapped to CUDA, OpenGL should not use this buffer
    uchar4 *dptr = NULL;
    cudaGLMapBufferObject((void**)&dptr, m_pbo);

    // Execute the kernel
    kernelVersionVis(dptr, m_width, m_height, m_major, m_minor);

    // Unmap buffer object
    cudaGLUnmapBufferObject(m_pbo);
}

void mainLoop() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        runCUDA();

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
        glBindTexture(GL_TEXTURE_2D, m_image);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, GL_RGBA,
                        GL_UNSIGNED_BYTE, NULL);
        glClear(GL_COLOR_BUFFER_BIT);

        // VAO, shader program, and texture already bound
        glDrawElements(GL_TRIANGLES, 6,  GL_UNSIGNED_SHORT, 0);
        glfwSwapBuffers(m_window);
    }
    glfwDestroyWindow(m_window);
    glfwTerminate();
}


void errorCallback(int error, const char *description) {
    fprintf(stderr, "error %d: %s\n", error, description);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

// ====================================
// Clean-up stuff
// ====================================

void cleanupCUDA() {
    if (m_pbo) {
        deletePBO(&m_pbo);
    }
    if (m_image) {
        deleteTexture(&m_image);
    }
}

void deletePBO(GLuint *pbo) {
    if (pbo) {
        // unregister this buffer object with CUDA
        cudaGLUnregisterBufferObject(*pbo);

        glBindBuffer(GL_ARRAY_BUFFER, *pbo);
        glDeleteBuffers(1, pbo);

        *pbo = (GLuint)NULL;
    }
}

void deleteTexture(GLuint *tex) {
    glDeleteTextures(1, tex);
    *tex = (GLuint)NULL;
}

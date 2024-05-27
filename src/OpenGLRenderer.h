#pragma once

#ifdef __EMSCRIPTEN__
    #include <GLES2/gl2.h>
    #include <emscripten.h>
    #include <emscripten/html5_webgl.h>
    #include <emscripten/html5.h>
#endif

#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
#include <iostream>
#include <string>

void initBuffers();

GLuint compileShader(GLenum shaderType, const char *src);

GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);

GLuint initShaderProgram();

void initContextGL(const std::string& canvasId, uint32_t width, uint32_t height);

bool RenderFrame(int nFrameWidth, int nFrameHeight);

EM_BOOL requestRender(double time,void* p);

void request(void *);

void* initOpenGLRenderer(void *args);

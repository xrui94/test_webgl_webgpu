#include "OpenGLRenderer.h"
#include "Common.h"

#ifdef __EMSCRIPTEN__
    // #include <GLES2/gl2.h>
    #include <GLES3/gl3.h>
    #include <emscripten.h>
    #include <emscripten/html5_webgl.h>
    #include <emscripten/html5.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip> // 必须包含这个头文件来使用setprecision
#include <string>


static const float g_VertexAttrsData[] = {
//     x,     y,   z     r,   g,   b
    -0.5f, -0.5f, 0.f, 1.f, 0.f, 0.f,
     0.5f, -0.5f, 0.f, 0.f, 1.f, 0.f,
     0.0f,  0.5f, 0.f, 0.f, 0.f, 1.f,
}; 

// WebGL1 默认版本：#version 110 es
// WebGL1 Shader Code
// static const char g_vsCode[] =
// "attribute vec3 in_Position;"
// "attribute vec3 in_Color;"
// "varying vec3 color;"
// "void main() {"
// "color = in_Color;"
// "gl_Position = vec4(in_Position, 1.0);"
// "}";
 
// static const char g_fsCode[] =
// "precision lowp float;"
// "varying vec3 color;"
// "void main() {"
// "gl_FragColor = vec4(color, 1.0);"
// "}";

static GLuint g_ShaderProgram;
float g_StartTime;
float g_LastTime;

// 须开启编译参数：-sMAX_WEBGL_VERSION=2，而不是-sMIN_WEBGL_VERSION=2,
// 如果使用后者，则默认仍然是使用WebGL1进行渲染
// WebGL2 Shader Code
static const char g_vsCode[] =
    "#version 300 es\n"
    "layout(location = 0) in vec3 in_Position;"
    "layout(location = 1) in vec3 in_Color;"
    "uniform float u_time;"
    "out vec3 io_Color;"
    "void main() {"
        "io_Color = in_Color;"
        "float angle = radians(0.2) * u_time;"
        "mat4 rotationMatrix = mat4("
            "cos(angle), -sin(angle), 0.0, 0.0,"
            "sin(angle),  cos(angle), 0.0, 0.0,"
            "0.0,        0.0,         1.0, 0.0,"
            "0.0,        0.0,         0.0, 1.0"
        ");"
        "gl_Position = rotationMatrix * vec4(in_Position, 1.0);"
    "}";

static const char g_fsCode[] =
    "#version 300 es\n"
    "precision lowp float;"
    "in vec3 io_Color;"
    "out vec4 out_FragColor;"
    "void main() {"
        "out_FragColor = vec4(io_Color, 1.0);"
    "}";

void initBuffers()
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexAttrsData), g_VertexAttrsData, GL_STATIC_DRAW);
 
    GLint nPosLoc = glGetAttribLocation(g_ShaderProgram, "in_Position");
    glEnableVertexAttribArray(nPosLoc);
    glVertexAttribPointer(nPosLoc, 3, GL_FLOAT, GL_FALSE, 24, 0);
    // glBindBuffer(GL_ARRAY_BUFFER, NULL);

    GLint nColorLoc = glGetAttribLocation(g_ShaderProgram, "in_Color");
    glEnableVertexAttribArray(nColorLoc);
    glVertexAttribPointer(nColorLoc, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    // glBindBuffer(GL_ARRAY_BUFFER, NULL);
}

GLuint compileShader(GLenum shaderType, const char *src)
{
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        char *infoLog = (char*)malloc(maxLength+1);
        glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog);
        printf("Failed to compile shader:%s\n", infoLog);
        free(infoLog);
        return 0;
    }

    return shader;
}

GLuint createProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    //
    GLint isLinked;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, infoLog.data());
        printf("Shader linking failed %s\n", infoLog.data());
    }

    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

GLuint initShaderProgram()
{
    ///< 顶点着色器相关操作
    GLuint vs = compileShader(GL_VERTEX_SHADER, g_vsCode);

    ///< 片段着色器相关操作
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, g_fsCode);

	///<program相关
    return createProgram(vs, fs);
}

void initContextGL(const std::string& canvasId, uint32_t width, uint32_t height)
{
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    // attr.explicitSwapControl = EM_TRUE;  // 查看：https://emscripten.org/docs/api_reference/html5.h.html
    attr.alpha = 0;
#if MAX_WEBGL_VERSION >= 2
    attr.majorVersion = 2;
#endif
    printf("Create context fallback\n");
    // attr.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_FALLBACK;//EMSCRIPTEN_WEBGL_CONTEXT_PROXY_FALLBACK    // // 查看：https://emscripten.org/docs/api_reference/html5.h.html
    // attr.renderViaOffscreenBackBuffer = EM_TRUE; // 查看：https://emscripten.org/docs/api_reference/html5.h.html
    // std::cout << "@@@@@@@@@@@@@@@-----" << g_pHwd << std::endl;
    // EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext = emscripten_webgl_create_context(g_pHwd.c_str(), &attr);
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext = emscripten_webgl_create_context(canvasId.c_str(), &attr);
    assert(glContext);
    // emscripten_set_canvas_element_size(g_pHwd.c_str(), width, height);
    emscripten_set_canvas_element_size(canvasId.c_str(), width, height);
    // emscripten_set_element_css_size(canvasId.c_str(), width, height);
    EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(glContext);
    assert(res == EMSCRIPTEN_RESULT_SUCCESS);
    assert(emscripten_webgl_get_current_context() == glContext);
    
	printf("Create context success\n");
}

void RenderFrame()
{
    glUseProgram(g_ShaderProgram);
    //将所有顶点数据上传至顶点着色器的顶点缓存

    GLint nPosLoc = glGetAttribLocation(g_ShaderProgram, "in_Position");
    glEnableVertexAttribArray(nPosLoc);
    glVertexAttribPointer(nPosLoc, 3, GL_FLOAT, GL_FALSE, 24, 0);
    // glBindBuffer(GL_ARRAY_BUFFER, NULL);

    GLint nColorLoc = glGetAttribLocation(g_ShaderProgram, "in_Color");
    glEnableVertexAttribArray(nColorLoc);
    glVertexAttribPointer(nColorLoc, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    // glBindBuffer(GL_ARRAY_BUFFER, NULL);

    // 传递时间uniform
    float currentTime = static_cast<float>(EM_ASM_DOUBLE(
        return performance.now();
    ));
    glUniform1f(glGetUniformLocation(g_ShaderProgram, "u_time"), currentTime);
    // std::cout << "currentTime" << std::setprecision(4) << std::fixed << currentTime << std::endl;

    // Draw
    glClearColor(0.0f, 0.0f, 0.0f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
 
    #define EXPLICIT_SWAP
    #ifdef EXPLICIT_SWAP
        // printf("EXPLICIT_SWAP emscripten_webgl_commit_frame\n");
        emscripten_webgl_commit_frame();
    #endif

    glUseProgram(NULL);
}

void* initOpenGLRenderer(void *args)
{
    auto initArgs = static_cast<InitArgs*>(args);

    //初始化Context
    initContextGL(initArgs->canvasId, initArgs->width, initArgs->height);

    //初始化着色器程序
    g_ShaderProgram = initShaderProgram();

    //初始化buffer
    initBuffers();

    double startTime = EM_ASM_DOUBLE(
        return performance.now();
    );
    g_StartTime = static_cast<float>(startTime);

    // RenderFrame();
    emscripten_set_main_loop(RenderFrame, 0, false);
	
    return nullptr;
}

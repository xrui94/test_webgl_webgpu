cmake_minimum_required(VERSION 3.10)

project(EngineCore VERSION 0.1.0 LANGUAGES CXX C)

# 设置编译选项
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 Emscripten 目标
if(EMSCRIPTEN)
    # option(ENABLE_ASYNC "Enable async mode and use async/await(return a js promise) in js to initialize WebGPU device" ON)

    # 添加头文件搜索目录
    INCLUDE_DIRECTORIES(
        ${CMAKE_SOURCE_DIR}/src
    )

    # 添加源文件
    FILE(GLOB SRC_FILES
        src/*.cpp
    )

    add_executable(${PROJECT_NAME} ${SRC_FILES})

    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

    # 
    set(CMAKE_EXECUTABLE_SUFFIX ".js") # Enable js output (no html).

   
    # 设置编译参数
    target_compile_options(${PROJECT_NAME} PRIVATE -pthread)    # -pthread在编译阶段和链接阶段都要使用，另外，注意：“-s USE_PTHREADS”已经弃用，应使用“-pthread”取代

    # 设置链接参数
    target_link_options(${PROJECT_NAME} PRIVATE -sENVIRONMENT=web,worker)
    target_link_options(${PROJECT_NAME} PRIVATE -pthread -sPTHREAD_POOL_SIZE_STRICT=0 -sALLOW_MEMORY_GROWTH=0 --bind)
    target_link_options(${PROJECT_NAME} PRIVATE -sMIN_WEBGL_VERSION=2 -sUSE_WEBGPU -sOFFSCREENCANVAS_SUPPORT=1 -sOFFSCREEN_FRAMEBUFFER=1)
    target_link_options(${PROJECT_NAME} PRIVATE -sASYNCIFY -sEXPORT_ES6=1 -sMODULARIZE=1 -sEXPORT_NAME="EngineCore")
    # target_link_options(${PROJECT_NAME} PRIVATE -sMIN_SAFARI_VERSION=-1) # 移除对Safari的支持，因为开启“离屏画布支持”后，如果调用时不使用离屏画布，会报错：canvas.getContextSafariWebGL2Fixed is not a function
    target_link_options(${PROJECT_NAME} PRIVATE -Os)    # Release模式进行优化
    # target_link_options(${PROJECT_NAME} PRIVATE -g) # Debug模式使用，报错时，将会打印C++中真实的函数名，可读性较高，而不会是混淆的函数名
    # target_link_options(${PROJECT_NAME} PRIVATE -sGL_DEBUG=1)
else()
   message(FATAL_ERROR "For native app that have not yet been implemented!")
endif()
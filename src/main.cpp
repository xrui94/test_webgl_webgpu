#include "OpenGLRenderer.h"
#include "WebGPURenderer.h"
#include "Event.h"
#include "Common.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/html5_webgl.h>
    #include <emscripten/html5.h>
    #include <emscripten/bind.h>
    #include <emscripten/threading.h>

    #include <pthread.h>
#endif

#include <iostream>
#include <cstdlib>


#ifdef __EMSCRIPTEN__

    struct StartupOpts
    {
        // StartupOpts(std::string containerId,uint32_t width, uint32_t height) 
        //     : containerId(containerId), width(width), height(height),
        //     backend("webgpu"), usingOffscreenCanvas(true), 
        //     customCanvas(false), canvasId("#canvas"), style("")
        // {
        // }

        std::string containerId;
        uint32_t width;
        uint32_t height;
        std::string backend = "webgpu";
        bool usingOffscreenCanvas = true;
        bool customCanvas = false;
        std::string canvasId = "#manvas";
        std::string style = "";
    };

    void createElement(const std::string& containerId, const std::string& canvasId, const std::string& style)
    {
        emscripten::val canvas = emscripten::val::global("document").call<emscripten::val>("createElement", emscripten::val("canvas"));
        canvas.set("id", canvasId.substr(1).c_str());   // 当在JS中传过来的Canvas加了“#”号时，要去掉“#”号
        // // canvas.set("id", canvasId.c_str());  // 在JS中传过来的Canvas没有“#”号时，则无须处理，直接使用
        // canvas.set("style.width", "100%");
        // canvas.set("style.height", "100%");
        // canvas.set("style.margin", "0");
        // canvas.set("style.padding", "0");

        if (style.size() == 0)
        {
            canvas.call<void>("setAttribute", emscripten::val("style"), emscripten::val("height: 100%; margin: 0; padding: 0"));
        }
        else
        {
            canvas.call<void>("setAttribute", emscripten::val("style"), emscripten::val(style.c_str()));
        }

        //
        emscripten::val parentElement = emscripten::val::global("document").call<emscripten::val>("getElementById", emscripten::val(containerId.c_str()));
        parentElement.call<void>("appendChild", canvas);

        // 将canvas实例添加到Module对象上
        EM_ASM_ARGS({
            let canvasId = UTF8ToString($0);
            if (canvasId.startsWith("#")) canvasId = canvasId.substr(1);
            const canvas = document.getElementById(canvasId);
            if (!canvas) throw "A valid HTML Canvas element is required.";
            canvas.addEventListener("webglcontextlost", (e) => { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);
            Module["canvas"] = canvas;
        }, canvasId.c_str());

        // emscripten::val updatedDiv = emscripten::val::global("document").call<emscripten::val>("getElementById", emscripten::val("myDiv"));
        // updatedDiv.set("innerHTML", "Updated content of the div element.");
    }

    InitArgs initArgs{};
    
    void startEngine(StartupOpts startupOpts)
    {
        // g_pHwd = "#" + startupOpts.canvasId;
        // g_Width = startupOpts.width;
        // g_Height = startupOpts.height;

        
        // InitArgs initArgs = {
        //     .window = nullptr,
        //     .canvasId = startupOpts.canvasId,
        //     .width = startupOpts.width,
        //     .height = startupOpts.height
        // };
        initArgs.window = nullptr;
        initArgs.canvasId = "#" + startupOpts.canvasId;
        initArgs.width = startupOpts.width;
        initArgs.height = startupOpts.height;

        //
        printf("Start Main Thread: CanvasID:%s, FrameWidth:%d, FrameHeight:%d\n", startupOpts.canvasId.c_str(), startupOpts.width, startupOpts.height);

        
        // 如果不调用该函数，则需要在js代码中创建canvas对象，并且将其添加到Module对象
        // 然后，将该“Module对象”，作为参数传给WASM模块的初始化函数
        if (!startupOpts.customCanvas)
        {
            // createElement(startupOpts.containerId, g_pHwd, startupOpts.style);
            createElement(startupOpts.containerId, initArgs.canvasId, startupOpts.style);
        }

        //
        Event event(initArgs.canvasId);

        if (startupOpts.usingOffscreenCanvas)
        {
            if (!emscripten_supports_offscreencanvas())
            {
                printf("Current browser does not support OffscreenCanvas. Skipping the rest of the tests.\n");
        #ifdef REPORT_RESULT
                REPORT_RESULT(1);
        #endif
                return;
            }
        
            // 设置线程中OffscreenCanvas对象的属性
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            // emscripten_pthread_attr_settransferredcanvases(&attr, g_pHwd.c_str());
            emscripten_pthread_attr_settransferredcanvases(&attr, initArgs.canvasId.c_str());

            // 创建渲染线程
            pthread_t thread;
            printf("Create rendering thread.\n");
            if (startupOpts.backend == "webgl2")
            {
                std::cout << "Using WebGL2 backend." << std::endl;
                pthread_create(&thread, &attr, initOpenGLRenderer, (void*)&initArgs);
            }
            else if (startupOpts.backend == "webgpu")
            {
                std::cout << "Using WebGPU backend." << std::endl;
                pthread_create(&thread, &attr, initWebGPURenderer, (void*)&initArgs);
            }
            else
            {
                std::cerr << "Invalid engine backend, which must be one of webgpu and webgl." << std::endl;
            }

            // 将渲染线程作为后台线程启动
            // 使用该启动方式，只有主进程停止了，渲染线程才会停止
            pthread_detach(thread);
        }
        else
        {
            printf("Start rendering in the main thread.\n");
            if (startupOpts.backend == "webgl2")
            {
                std::cout << "Using WebGL2 backend." << std::endl;
                initOpenGLRenderer((void*)&initArgs);
            }
            else if (startupOpts.backend == "webgpu")
            {
                std::cout << "Using WebGPU backend." << std::endl;
                initWebGPURenderer((void*)&initArgs);
            }
            else
            {
                std::cerr << "Invalid engine backend, which must be one of webgpu and webgl." << std::endl;
            }
        }
        
        // EM_ASM(noExitRuntime=true);
    }

    EMSCRIPTEN_BINDINGS(EngineCore)
    {
        emscripten::value_object<StartupOpts>("StartupOpts")
            .field("containerId", &StartupOpts::containerId)
            .field("width", &StartupOpts::width)
            .field("height", &StartupOpts::height)
            .field("backend", &StartupOpts::backend)
            .field("usingOffscreenCanvas", &StartupOpts::usingOffscreenCanvas)
            .field("customCanvas", &StartupOpts::customCanvas)
            .field("canvasId", &StartupOpts::canvasId)
            .field("style", &StartupOpts::style)
            ;

        emscripten::function("startEngine", &startEngine);
    }

#else

    int main()
    {
        InitArgs initArgs{};
        initArgs.window = nullptr;
        initArgs.canvasId = "#main-canvas";
        initArgs.width = 340;
        initArgs.height = 250;

        //
        GetDevice([initArgs](wgpu::Device dev) {
            device = dev;
            run(initArgs.canvasId, initArgs.width, initArgs.height);
        });

        return 0;
    }

#endif  // __EMSCRIPTEN__

// #Ref：
// web端 子线程调用opengl es：https://blog.csdn.net/qq_34754747/article/details/108150004
// 使用C、C++ 和Rust中的WebAssembly线程,以及报"Tried to spawn a new thread, but the thread pool is exhausted"错误的原因和解决方案: https://web.dev/articles/webassembly-threads?hl=zh-cn
// https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html?highlight=struct
#include "Event.h"
#include "Common.h"

#include <iostream>
#include <cstdlib>

#ifdef __EMSCRIPTEN__

    static bool g_Active = false;
   
    Event::Event(std::string canvasId)
        : m_CanvasId(canvasId)
    {
        m_DragState.active = false;
        m_DragState.startMouse = {
            .x = 100.f,
            .y = 100.f
        };

        RegisterEvent();
    }

    void Event::RegisterEvent()
    {
        // 关于事件的定义：https://github.com/emscripten-core/emscripten/blob/main/system/include/emscripten/html5.h
        EMSCRIPTEN_RESULT ret;

        // --- 以下方式不行，对于回调函数参数，无法引用静态函数 --
        // ret = emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, OnMouseClick);

        // ret = emscripten_set_dblclick_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, OnMouseDbClick);

        // ret = emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, OnMouseDown);

        // ret = emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, OnMouseUp);

        // ret = emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, OnMouseMove);

        // ret = emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, OnMouseWheel);

        // ret = emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, OnResize);

        // --- lambda必须 ---
        // lambda必须返回“EM_BOOL”类型，否则会报错：函数不匹配
        // std::cout << "this---@-->" << this << std::endl;
        ret = emscripten_set_click_callback(m_CanvasId.c_str(), this, true, [](int eventType, const EmscriptenMouseEvent *e, void *userData) -> EM_BOOL {
            return static_cast<Event*>(userData)->OnMouseClick(eventType, e, userData);
        });

        ret = emscripten_set_dblclick_callback(m_CanvasId.c_str(), this, true, [](int eventType, const EmscriptenMouseEvent *e, void *userData) -> EM_BOOL {
            return static_cast<Event*>(userData)->OnMouseDbClick(eventType, e, userData);
        });

        ret = emscripten_set_mousedown_callback(m_CanvasId.c_str(), this, true, [](int eventType, const EmscriptenMouseEvent *e, void *userData) -> EM_BOOL {
            return static_cast<Event*>(userData)->OnMouseDown(eventType, e, userData);
        });

        ret = emscripten_set_mouseup_callback(m_CanvasId.c_str(), this, true, [](int eventType, const EmscriptenMouseEvent *e, void *userData) -> EM_BOOL {
            return static_cast<Event*>(userData)->OnMouseUp(eventType, e, userData);
        });

        ret = emscripten_set_mousemove_callback(m_CanvasId.c_str(), this, true, [](int eventType, const EmscriptenMouseEvent *e, void *userData) -> EM_BOOL {
            return static_cast<Event*>(userData)->OnMouseMove(eventType, e, userData);
        });

        ret = emscripten_set_wheel_callback(m_CanvasId.c_str(), this, true, [](int eventType, const EmscriptenWheelEvent *e, void *userData) -> EM_BOOL {
            return static_cast<Event*>(userData)->OnMouseWheel(eventType, e, userData);
        });

        ret = emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, [](int eventType, const EmscriptenUiEvent *e, void *userData) -> EM_BOOL {
            return static_cast<Event*>(userData)->OnResize(eventType, e, userData);
        });
    }

    EM_BOOL Event::OnMouseClick(int eventType, const EmscriptenMouseEvent *e, void *userData)
    {
        // printf("%s, screen: (%d,%d), client: (%d,%d),%s%s%s%s button: %hu, buttons: %hu, movement: (%d,%d), target: (%d, %d)\n",
        //     emscripten_event_type_to_string(eventType), e->screenX, e->screenY, e->clientX, e->clientY,
        //     e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "",
        //     e->button, e->buttons, e->movementX, e->movementY, e->targetX, e->targetY);

        // if (e->screenX != 0 && e->screenY != 0 && e->clientX != 0 && e->clientY != 0 && e->targetX != 0 && e->targetY != 0)
        // {
        //     if (eventType == EMSCRIPTEN_EVENT_CLICK)
        //         gotClick = 1;
        //     if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN && e->buttons != 0)
        //         gotMouseDown = 1;
        //     if (eventType == EMSCRIPTEN_EVENT_DBLCLICK)
        //         gotDblClick = 1;
        //     if (eventType == EMSCRIPTEN_EVENT_MOUSEUP)
        //         gotMouseUp = 1;
        //     if (eventType == EMSCRIPTEN_EVENT_MOUSEMOVE && (e->movementX != 0 || e->movementY != 0))
        //         gotMouseMove = 1;
        // }

        if (eventType == EMSCRIPTEN_EVENT_CLICK && e->screenX == -500000)
        {
            printf("ERROR! Received an event to a callback that should have been unregistered!\n");
            // gotClick = 0;
            // report_result(1);
        }

        std::cout << "Processing mouse click event" << std::endl;
        std::cout << "screenX:" << e->screenX << std::endl;
        std::cout << "screenY:" << e->screenY << std::endl;
        std::cout << "clientX:" << e->clientX << std::endl;
        std::cout << "clientY:" << e->clientY << std::endl;
        std::cout << "targetX:" << e->targetX << std::endl;
        std::cout << "targetY:" << e->targetY << std::endl;

        return 0;
    }

    EM_BOOL Event::OnMouseDbClick(int eventType, const EmscriptenMouseEvent *e, void *userData)
    {
        std::cout << "Processing mouse double click event" << std::endl;
        std::cout << "screenX:" << e->screenX << std::endl;
        std::cout << "screenY:" << e->screenY << std::endl;
        std::cout << "clientX:" << e->clientX << std::endl;
        std::cout << "clientY:" << e->clientY << std::endl;
        std::cout << "targetX:" << e->targetX << std::endl;
        std::cout << "targetY:" << e->targetY << std::endl;
        
        return 0;
    }

    EM_BOOL Event::OnMouseDown(int eventType, const EmscriptenMouseEvent *e, void *userData)
    {
        m_DragState.active = true;
        g_Active = true;
        std::cout << "Processing mouse down event" << std::endl;
        std::cout << "screenX:" << e->screenX << std::endl;
        std::cout << "screenY:" << e->screenY << std::endl;
        std::cout << "clientX:" << e->clientX << std::endl;
        std::cout << "clientY:" << e->clientY << std::endl;
        std::cout << "targetX:" << e->targetX << std::endl;
        std::cout << "targetY:" << e->targetY << std::endl;

        return 0;
    }

    EM_BOOL Event::OnMouseUp(int eventType, const EmscriptenMouseEvent *e, void *userData)
    {
        m_DragState.active = false;
        g_Active = false;
        std::cout << "Processing mouse up event" << std::endl;
        std::cout << "screenX:" << e->screenX << std::endl;
        std::cout << "screenY:" << e->screenY << std::endl;
        std::cout << "clientX:" << e->clientX << std::endl;
        std::cout << "clientY:" << e->clientY << std::endl;
        std::cout << "targetX:" << e->targetX << std::endl;
        std::cout << "targetY:" << e->targetY << std::endl;
        
        return 0;
    }

    EM_BOOL Event::OnMouseMove(int eventType, const EmscriptenMouseEvent *e, void *userData)
    {
        // if (m_DragState.active)  // not working
        if (g_Active)
        {
            std::cout << "Processing mouse move event" << std::endl;
            std::cout << "screenX:" << e->screenX << std::endl;
            std::cout << "screenY:" << e->screenY << std::endl;
            std::cout << "clientX:" << e->clientX << std::endl;
            std::cout << "clientY:" << e->clientY << std::endl;
            std::cout << "targetX:" << e->targetX << std::endl;
            std::cout << "targetY:" << e->targetY << std::endl;
        }

        return 0;
    }

    EM_BOOL Event::OnMouseWheel(int eventType, const EmscriptenWheelEvent *e, void *userData)
    {
        // printf("%s, screen: (%d,%d), client: (%d,%d),%s%s%s%s button: %hu, buttons: %hu, target: (%d, %d), delta:(%g,%g,%g), deltaMode:%u\n",
        //     emscripten_event_type_to_string(eventType), e->mouse.screenX, e->mouse.screenY, e->mouse.clientX, e->mouse.clientY,
        //     e->mouse.ctrlKey ? " CTRL" : "", e->mouse.shiftKey ? " SHIFT" : "", e->mouse.altKey ? " ALT" : "", e->mouse.metaKey ? " META" : "",
        //     e->mouse.button, e->mouse.buttons, e->mouse.targetX, e->mouse.targetY,
        //     (float)e->deltaX, (float)e->deltaY, (float)e->deltaZ, e->deltaMode);

        // if (e->deltaY > 0.f || e->deltaY < 0.f)
        //     gotWheel = 1;

        std::cout << "Processing mouse wheel event" << std::endl;
        std::cout << "deltaX:" << e->deltaX << std::endl;
        std::cout << "deltaY:" << e->deltaY << std::endl;

        return 0;
    }

    EM_BOOL Event::OnResize(int eventType, const EmscriptenUiEvent *e, void *userData)
    {
        int width = (int) e->windowInnerWidth;
        int height = (int) e->windowInnerHeight;

        std::cout << "Processing resize event" << std::endl;
        std::cout << "width:" << width << std::endl;
        std::cout << "height:" << height << std::endl;

        return 0;
    }

    inline const char* Event::emscripten_event_type_to_string(int eventType)
    {
        const char *events[] = {"(invalid)", "(none)", "keypress", "keydown", "keyup", "click", "mousedown", "mouseup", "dblclick", "mousemove", "wheel", "resize",
                                "scroll", "blur", "focus", "focusin", "focusout", "deviceorientation", "devicemotion", "orientationchange", "fullscreenchange", "pointerlockchange",
                                "visibilitychange", "touchstart", "touchend", "touchmove", "touchcancel", "gamepadconnected", "gamepaddisconnected", "beforeunload",
                                "batterychargingchange", "batterylevelchange", "webglcontextlost", "webglcontextrestored", "(invalid)"};
        ++eventType;
        if (eventType < 0)
            eventType = 0;
        if (eventType >= sizeof(events) / sizeof(events[0]))
            eventType = sizeof(events) / sizeof(events[0]) - 1;
        return events[eventType];
    }

    const char* Event::emscripten_result_to_string(EMSCRIPTEN_RESULT result)
    {
        if (result == EMSCRIPTEN_RESULT_SUCCESS)
            return "EMSCRIPTEN_RESULT_SUCCESS";
        if (result == EMSCRIPTEN_RESULT_DEFERRED)
            return "EMSCRIPTEN_RESULT_DEFERRED";
        if (result == EMSCRIPTEN_RESULT_NOT_SUPPORTED)
            return "EMSCRIPTEN_RESULT_NOT_SUPPORTED";
        if (result == EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED)
            return "EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED";
        if (result == EMSCRIPTEN_RESULT_INVALID_TARGET)
            return "EMSCRIPTEN_RESULT_INVALID_TARGET";
        if (result == EMSCRIPTEN_RESULT_UNKNOWN_TARGET)
            return "EMSCRIPTEN_RESULT_UNKNOWN_TARGET";
        if (result == EMSCRIPTEN_RESULT_INVALID_PARAM)
            return "EMSCRIPTEN_RESULT_INVALID_PARAM";
        if (result == EMSCRIPTEN_RESULT_FAILED)
            return "EMSCRIPTEN_RESULT_FAILED";
        if (result == EMSCRIPTEN_RESULT_NO_DATA)
            return "EMSCRIPTEN_RESULT_NO_DATA";
        return "Unknown EMSCRIPTEN_RESULT!";
    }

#endif

// #Ref:
// emscripten event examples：https://github.com/emscripten-core/emscripten/blob/main/test/test_html5_mouse.c
// emscripten事件API：https://emscripten.org/docs/api_reference/html5.h.html#how-to-use-this-api
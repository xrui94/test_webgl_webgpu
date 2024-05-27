#pragma once

#include "Math.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <functional>
#include <iostream>

#ifdef __EMSCRIPTEN__

class Event
{
public:
    Event(std::string canvasId);

public:
    // static EM_BOOL OnMouseClick(int eventType, const EmscriptenMouseEvent *e, void *userData);

    // static EM_BOOL OnMouseDbClick(int eventType, const EmscriptenMouseEvent *e, void *userData);

    // static EM_BOOL OnMouseDown(int eventType, const EmscriptenMouseEvent *e, void *userData);

    // static EM_BOOL OnMouseUp(int eventType, const EmscriptenMouseEvent *e, void *userData);

    // static EM_BOOL OnMouseMove(int eventType, const EmscriptenMouseEvent *e, void *userData);

    // static EM_BOOL OnMouseWheel(int eventType, const EmscriptenWheelEvent *e, void *userData);

    // static EM_BOOL OnResize(int eventType, const EmscriptenUiEvent *e, void *userData);




    EM_BOOL OnMouseClick(int eventType, const EmscriptenMouseEvent *e, void *userData);

    EM_BOOL OnMouseDbClick(int eventType, const EmscriptenMouseEvent *e, void *userData);

    EM_BOOL OnMouseDown(int eventType, const EmscriptenMouseEvent *e, void *userData);

    EM_BOOL OnMouseUp(int eventType, const EmscriptenMouseEvent *e, void *userData);

    EM_BOOL OnMouseMove(int eventType, const EmscriptenMouseEvent *e, void *userData);

    EM_BOOL OnMouseWheel(int eventType, const EmscriptenWheelEvent *e, void *userData);

    EM_BOOL OnResize(int eventType, const EmscriptenUiEvent *e, void *userData);

    static inline const char *emscripten_event_type_to_string(int eventType);

    const char *emscripten_result_to_string(EMSCRIPTEN_RESULT result);

private:
    void RegisterEvent();

private:
    std::string m_CanvasId;

    struct DragState {
        bool active = false;

		vec2 startMouse = {
            .x = 0.f,
            .y = 0.f
        };
	};

    DragState m_DragState;
};

#endif

// @Ref:
// emscripten event examples：https://github.com/emscripten-core/emscripten/blob/main/test/test_html5_mouse.c
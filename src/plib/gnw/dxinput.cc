#include "plib/gnw/dxinput.h"

#include <cstdlib>

namespace fallout {

static bool dxinput_mouse_init();
static void dxinput_mouse_exit();
static bool dxinput_keyboard_init();
static void dxinput_keyboard_exit();

static int gMouseWheelDeltaX = 0;
static int gMouseWheelDeltaY = 0;

static SDL_GameController* gGamepad = nullptr;
static bool gGamepadSubsystemReady = false;
static float gGamepadRemainderX = 0.0f;
static float gGamepadRemainderY = 0.0f;

// Opens the first available gamepad, and re-checks if it gets unplugged.
static void gamepadUpdate()
{
    if (!gGamepadSubsystemReady) {
        gGamepadSubsystemReady = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
        if (!gGamepadSubsystemReady) {
            return;
        }
    }

    if (gGamepad != nullptr && !SDL_GameControllerGetAttached(gGamepad)) {
        SDL_GameControllerClose(gGamepad);
        gGamepad = nullptr;
    }

    if (gGamepad == nullptr) {
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                gGamepad = SDL_GameControllerOpen(i);
                if (gGamepad != nullptr) {
                    break;
                }
            }
        }
    }
}

// 0x4E0400
bool dxinput_init()
{
    if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0) {
        return false;
    }

    if (!dxinput_mouse_init()) {
        goto err;
    }

    if (!dxinput_keyboard_init()) {
        goto err;
    }

    return true;

err:

    dxinput_mouse_exit();

    return false;
}

// 0x4E0478
void dxinput_exit()
{
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

// 0x4E04E8
bool dxinput_acquire_mouse()
{
    return true;
}

// 0x4E0514
bool dxinput_unacquire_mouse()
{
    return true;
}

// 0x4E053C
bool dxinput_get_mouse_state(MouseData* mouseState)
{
    // CE: This function is sometimes called outside loops calling `get_input`
    // and subsequently `GNW95_process_message`, so mouse events might not be
    // handled by SDL yet.
    //
    // TODO: Move mouse events processing into `GNW95_process_message` and
    // update mouse position manually.
    SDL_PumpEvents();

    Uint32 buttons = SDL_GetRelativeMouseState(&(mouseState->x), &(mouseState->y));
    mouseState->buttons[0] = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    mouseState->buttons[1] = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

    // Gamepad: left stick moves the cursor, A = left click, B = right click.
    gamepadUpdate();
    if (gGamepad != nullptr) {
        const int kDeadzone = 8000;
        const float kMaxSpeed = 14.0f;

        int axisX = SDL_GameControllerGetAxis(gGamepad, SDL_CONTROLLER_AXIS_LEFTX);
        int axisY = SDL_GameControllerGetAxis(gGamepad, SDL_CONTROLLER_AXIS_LEFTY);

        float fx = 0.0f;
        float fy = 0.0f;
        if (abs(axisX) > kDeadzone) {
            fx = (axisX > 0 ? axisX - kDeadzone : axisX + kDeadzone) / (32767.0f - kDeadzone);
        }
        if (abs(axisY) > kDeadzone) {
            fy = (axisY > 0 ? axisY - kDeadzone : axisY + kDeadzone) / (32767.0f - kDeadzone);
        }

        fx = fx * (fx < 0 ? -fx : fx);
        fy = fy * (fy < 0 ? -fy : fy);

        gGamepadRemainderX += fx * kMaxSpeed;
        gGamepadRemainderY += fy * kMaxSpeed;
        int dx = (int)gGamepadRemainderX;
        int dy = (int)gGamepadRemainderY;
        gGamepadRemainderX -= dx;
        gGamepadRemainderY -= dy;

        mouseState->x += dx;
        mouseState->y += dy;

        if (SDL_GameControllerGetButton(gGamepad, SDL_CONTROLLER_BUTTON_A)) {
            mouseState->buttons[0] = 1;
        }
        if (SDL_GameControllerGetButton(gGamepad, SDL_CONTROLLER_BUTTON_B)) {
            mouseState->buttons[1] = 1;
        }
    }
    mouseState->wheelX = gMouseWheelDeltaX;
    mouseState->wheelY = gMouseWheelDeltaY;

    gMouseWheelDeltaX = 0;
    gMouseWheelDeltaY = 0;

    return true;
}

// 0x4E05A8
bool dxinput_acquire_keyboard()
{
    return true;
}

// 0x4E05D4
bool dxinput_unacquire_keyboard()
{
    return true;
}

// 0x4E05FC
bool dxinput_flush_keyboard_buffer()
{
    SDL_FlushEvents(SDL_KEYDOWN, SDL_TEXTINPUT);
    return true;
}

// 0x4E0650
bool dxinput_read_keyboard_buffer(KeyboardData* keyboardData)
{
    return true;
}

// 0x4E070C
bool dxinput_mouse_init()
{
    return SDL_SetRelativeMouseMode(SDL_TRUE) == 0;
}

// 0x4E078C
void dxinput_mouse_exit()
{
}

// 0x4E07B8
bool dxinput_keyboard_init()
{
    return true;
}

// 0x4E0874
void dxinput_keyboard_exit()
{
}

void handleMouseEvent(SDL_Event* event)
{
    // Mouse movement and buttons are accumulated in SDL itself and will be
    // processed later in `mouseDeviceGetData` via `SDL_GetRelativeMouseState`.

    if (event->type == SDL_MOUSEWHEEL) {
        gMouseWheelDeltaX += event->wheel.x;
        gMouseWheelDeltaY += event->wheel.y;
    }
}

} // namespace fallout

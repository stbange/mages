/* SDL3 front end: window, input (keyboard, gamepad, mouse, touch), audio and
   the main loop. Everything else lives in the platform-free core. */
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "palette.h"
#include "script.h"
#include "ui.h"

#define TICK_NS (1000000000ull / 60ull)

static Ui ui;
static Framebuffer fb;
static Mixer mixer;
static uint32_t rgba[SCREEN_H * SCREEN_W];
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_AudioStream *stream;
static bool audio_tried;
static Uint64 last_ns, acc_ns;
static long frames_left = -1;
static SDL_Gamepad *pad;
static int stick_dir = -1;

static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *s, int additional, int total)
{
    (void)userdata;
    (void)total;
    int16_t buf[1024];
    int frames = additional / (int)sizeof(int16_t);
    while (frames > 0) {
        int n = frames > 1024 ? 1024 : frames;
        audio_render(&mixer, buf, n);
        SDL_PutAudioStreamData(s, buf, n * (int)sizeof(int16_t));
        frames -= n;
    }
}

/* Browsers only allow audio after a user gesture, so open it lazily. */
static void ensure_audio(void)
{
    if (audio_tried) return;
    audio_tried = true;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("audio unavailable: %s", SDL_GetError());
        return;
    }
    SDL_AudioSpec spec = {SDL_AUDIO_S16, 1, AUDIO_RATE};
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, NULL);
    if (!stream) {
        SDL_Log("cannot open audio: %s", SDL_GetError());
        return;
    }
    SDL_ResumeAudioStreamDevice(stream);
}

static void play_pending_sounds(void)
{
    int s;
    if (stream) SDL_LockAudioStream(stream);
    while ((s = ui_pop_sfx(&ui)) >= 0) audio_play(&mixer, s);
    if (stream) SDL_UnlockAudioStream(stream);
}

static void send(InputKind k, float x, float y)
{
    ui_input(&ui, (Input){k, (int)x, (int)y});
}

static InputKind key_to_input(SDL_Keycode k, SDL_Keymod mod)
{
    switch (k) {
    case SDLK_UP: case SDLK_W: case SDLK_KP_8: return IN_UP;
    case SDLK_DOWN: case SDLK_S: case SDLK_KP_2: return IN_DOWN;
    case SDLK_LEFT: case SDLK_A: case SDLK_KP_4: return IN_LEFT;
    case SDLK_RIGHT: case SDLK_D: case SDLK_KP_6: return IN_RIGHT;
    case SDLK_HOME: case SDLK_KP_7: return IN_UPLEFT;
    case SDLK_PAGEUP: case SDLK_KP_9: return IN_UPRIGHT;
    case SDLK_END: case SDLK_KP_1: return IN_DOWNLEFT;
    case SDLK_PAGEDOWN: case SDLK_KP_3: return IN_DOWNRIGHT;
    case SDLK_SPACE: case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_KP_5: return IN_FIRE;
    case SDLK_ESCAPE: case SDLK_BACKSPACE: return IN_CANCEL;
    case SDLK_TAB: return (mod & SDL_KMOD_SHIFT) ? IN_PREV : IN_NEXT;
    case SDLK_V: return IN_VIEW;
    case SDLK_C: return IN_CAST;
    case SDLK_F: return IN_FLY;
    case SDLK_G: return IN_PICKUP;
    case SDLK_X: return IN_DROP;
    case SDLK_Q: return IN_DRINK;
    case SDLK_U: return IN_USE;
    case SDLK_R: return IN_RIDE;
    case SDLK_T: return IN_END_TURN;
    case SDLK_H: case SDLK_F1: return IN_HELP;
    default: return IN_NONE;
    }
}

static InputKind button_to_input(Uint8 b)
{
    switch (b) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP: return IN_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return IN_DOWN;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return IN_LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return IN_RIGHT;
    case SDL_GAMEPAD_BUTTON_SOUTH: return IN_FIRE;
    case SDL_GAMEPAD_BUTTON_EAST: return IN_CANCEL;
    case SDL_GAMEPAD_BUTTON_WEST: return IN_CAST;
    case SDL_GAMEPAD_BUTTON_NORTH: return IN_VIEW;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return IN_PREV;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return IN_NEXT;
    case SDL_GAMEPAD_BUTTON_START: return IN_END_TURN;
    case SDL_GAMEPAD_BUTTON_BACK: return IN_HELP;
    default: return IN_NONE;
    }
}

/* One pointer gesture (mouse button or first finger): a press becomes a
   click/tap on release unless it moved far enough to be a drag. */
#define DRAG_THRESHOLD 4.0f
static struct {
    bool active, dragging, touch, clickable;
    SDL_FingerID finger;
    float sx, sy, lx, ly;
} gesture;

static void gesture_begin(float x, float y, bool touch, bool clickable, SDL_FingerID finger)
{
    gesture.active = true;
    gesture.dragging = false;
    gesture.touch = touch;
    gesture.clickable = clickable;
    gesture.finger = finger;
    gesture.sx = gesture.lx = x;
    gesture.sy = gesture.ly = y;
}

static void gesture_move(float x, float y)
{
    if (!gesture.active) return;
    float tdx = x - gesture.sx, tdy = y - gesture.sy;
    if (!gesture.dragging && tdx * tdx + tdy * tdy > DRAG_THRESHOLD * DRAG_THRESHOLD) {
        gesture.dragging = true;
        gesture.lx = gesture.sx;
        gesture.ly = gesture.sy;
    }
    if (gesture.dragging) {
        int dx = (int)(x - gesture.lx), dy = (int)(y - gesture.ly);
        if (dx || dy) {
            send(IN_DRAG, (float)dx, (float)dy);
            gesture.lx += (float)dx;
            gesture.ly += (float)dy;
        }
    }
}

static void gesture_end(float x, float y)
{
    if (!gesture.active) return;
    gesture.active = false;
    if (!gesture.dragging && gesture.clickable) send(gesture.touch ? IN_TAP : IN_CLICK, x, y);
}

static void handle_stick(void)
{
    if (!pad) return;
    const int dead = 16000;
    int x = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX), y = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY);
    int dx = x > dead ? 1 : (x < -dead ? -1 : 0), dy = y > dead ? 1 : (y < -dead ? -1 : 0);
    static const InputKind map[3][3] = {
        {IN_UPLEFT, IN_UP, IN_UPRIGHT}, {IN_LEFT, IN_NONE, IN_RIGHT}, {IN_DOWNLEFT, IN_DOWN, IN_DOWNRIGHT}};
    int d = (dx || dy) ? (dy + 1) * 3 + (dx + 1) : -1;
    if (d != stick_dir && d >= 0) send(map[dy + 1][dx + 1], 0, 0);
    stick_dir = d;
}

static char *read_file(const char *path)
{
    size_t n = 0;
    void *data = SDL_LoadFile(path, &n);
    return data;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    (void)appstate;
    const char *script_path = NULL;
    uint64_t seed = (uint64_t)SDL_GetTicksNS() ^ 0x5DEECE66Dull;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--script") && i + 1 < argc) script_path = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames_left = strtol(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--seed") && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
    }
    SDL_SetAppMetadata("Mages", "0.1.0", "io.github.stbange.mages");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("Mages", SCREEN_W * 3, SCREEN_H * 3, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("cannot create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
#ifdef __EMSCRIPTEN__
    SDL_SetRenderLogicalPresentation(renderer, SCREEN_W, SCREEN_H, SDL_LOGICAL_PRESENTATION_LETTERBOX);
#else
    SDL_SetRenderLogicalPresentation(renderer, SCREEN_W, SCREEN_H, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
#endif
    SDL_SetRenderVSync(renderer, 1);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    if (!texture) {
        SDL_Log("cannot create texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    audio_init(&mixer);
    ui_init(&ui, seed);
#ifndef __EMSCRIPTEN__
    ensure_audio();
#endif
    if (script_path) {
        char *text = read_file(script_path);
        if (!text) {
            SDL_Log("cannot read script %s", script_path);
            return SDL_APP_FAILURE;
        }
        Script s = {&ui, &fb, NULL, 0, "", 0};
        bool ok = script_run(&s, text);
        SDL_free(text);
        if (!ok) {
            SDL_Log("script failed at line %d: %s", s.line, s.error);
            return SDL_APP_FAILURE;
        }
        SDL_Log("script passed (%d commands)", s.commands);
    }
    last_ns = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e)
{
    (void)appstate;
    switch (e->type) {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN: {
        ensure_audio();
        if (e->key.key == SDLK_F11 || (e->key.key == SDLK_RETURN && (e->key.mod & SDL_KMOD_ALT))) {
            SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
            break;
        }
        if (e->key.key >= SDLK_1 && e->key.key <= SDLK_9) {
            send(IN_DIGIT, (float)(e->key.key - SDLK_1 + 1), 0);
            break;
        }
        InputKind k = key_to_input(e->key.key, e->key.mod);
        if (k != IN_NONE) send(k, 0, 0);
        break;
    }
    case SDL_EVENT_GAMEPAD_ADDED:
        if (!pad) pad = SDL_OpenGamepad(e->gdevice.which);
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (pad && SDL_GetGamepadID(pad) == e->gdevice.which) {
            SDL_CloseGamepad(pad);
            pad = NULL;
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        ensure_audio();
        send(button_to_input(e->gbutton.button), 0, 0);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        if (e->motion.which == SDL_TOUCH_MOUSEID) break;
        SDL_ConvertEventToRenderCoordinates(renderer, e);
        if (!gesture.touch || !gesture.active) gesture_move(e->motion.x, e->motion.y);
        send(IN_POINT, e->motion.x, e->motion.y);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        ensure_audio();
        if (e->button.which == SDL_TOUCH_MOUSEID) break;
        SDL_ConvertEventToRenderCoordinates(renderer, e);
        if (e->button.button == SDL_BUTTON_RIGHT) send(IN_CANCEL, 0, 0);
        else gesture_begin(e->button.x, e->button.y, false, e->button.button == SDL_BUTTON_LEFT, 0);
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e->button.which == SDL_TOUCH_MOUSEID || gesture.touch) break;
        SDL_ConvertEventToRenderCoordinates(renderer, e);
        gesture_end(e->button.x, e->button.y);
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        if (e->wheel.y != 0) send(IN_WHEEL, 0, e->wheel.y > 0 ? -1.0f : 1.0f);
        break;
    case SDL_EVENT_FINGER_DOWN:
        ensure_audio();
        if (gesture.active) break; /* only the first finger counts */
        SDL_ConvertEventToRenderCoordinates(renderer, e);
        gesture_begin(e->tfinger.x, e->tfinger.y, true, true, e->tfinger.fingerID);
        break;
    case SDL_EVENT_FINGER_MOTION:
        if (!gesture.active || !gesture.touch || e->tfinger.fingerID != gesture.finger) break;
        SDL_ConvertEventToRenderCoordinates(renderer, e);
        gesture_move(e->tfinger.x, e->tfinger.y);
        break;
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        if (!gesture.active || !gesture.touch || e->tfinger.fingerID != gesture.finger) break;
        SDL_ConvertEventToRenderCoordinates(renderer, e);
        if (e->type == SDL_EVENT_FINGER_CANCELED) gesture.active = false;
        else gesture_end(e->tfinger.x, e->tfinger.y);
        break;
    default: break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    (void)appstate;
    Uint64 now = SDL_GetTicksNS();
    acc_ns += now - last_ns;
    last_ns = now;
    if (acc_ns > TICK_NS * 10) acc_ns = TICK_NS * 10;
    handle_stick();
    while (acc_ns >= TICK_NS) {
        ui_tick(&ui);
        acc_ns -= TICK_NS;
    }
    play_pending_sounds();
    ui_render(&ui, &fb);
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) rgba[y * SCREEN_W + x] = PALETTE_RGB[fb.px[y][x] & 15];
    SDL_UpdateTexture(texture, NULL, rgba, SCREEN_W * (int)sizeof(uint32_t));
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    if (frames_left > 0 && --frames_left == 0) return SDL_APP_SUCCESS;
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)appstate;
    (void)result;
    if (stream) SDL_DestroyAudioStream(stream);
    if (pad) SDL_CloseGamepad(pad);
}

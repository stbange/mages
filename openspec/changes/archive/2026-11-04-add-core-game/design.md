# Design

## Context

This is a new repository. The game must run on Windows, Linux and the web
(including phones) and must be heavily testable: at least 95% unit test coverage
and scripted game tests. The look is a 320x200, 16-colour 8-bit screen.

## Goals / Non-Goals

**Goals:**
- One C11 code base, with SDL3 as the only runtime dependency.
- All game logic, UI state, rendering and audio synthesis testable without a
  window, audio device or GPU.
- Deterministic games for replayable tests.
- A 3D view that looks like it belongs on the same 8-bit machine.

**Non-Goals:**
- Hardware-accelerated 3D, shaders or OpenGL.
- Music, the wizard designer and campaign progression, save games (later changes).
- Pixel-exact emulation of any existing game.

## Decisions

1. **Core / platform split.** `src/core` (static library `mages_core`) contains
   everything except SDL calls. `src/platform/main_sdl.c` handles the window,
   presenting the texture, translating input, the audio stream and the
   Emscripten main loop. Coverage is measured on `src/core` only.
   *Alternative:* mocking SDL. Rejected because it is brittle and adds nothing.

2. **Indexed framebuffer.** The core renders into `uint8_t[200][320]` palette
   indices. The platform converts to RGBA once per frame. Tests can assert that
   pixels are palette entries, and the 3D view cannot leak colours outside the
   palette. Darkening for distance uses a palette-to-palette lookup table
   instead of arithmetic.

3. **Grid raycaster for 3D.** The map is a grid of squares that wraps around,
   so a DDA raycaster (wrapping cell coordinates) is simple and fast in
   software on every target. There are eight facings to match grid movement,
   and turns are animated by interpolating the angle. Creatures, trees and
   objects are billboards that use the same 16x16 sprites as the map,
   z-buffered per column. Floor and ceiling are cast per pixel using the
   floor colour of each terrain, and roofed squares get a ceiling.

4. **UI as a pure state machine.** `ui.c` turns abstract inputs (direction,
   fire, cancel, tap at screen x/y, 3D turn) into game actions and modes. The
   platform maps keys, gamepad buttons, mouse and touch to these inputs. Game
   test scripts inject the same inputs, so they cover the real control path.

5. **Rules API.** `game_*` functions return a result code plus a message, and
   push events (sound and message cues) to a small queue the UI drains. AI
   code calls the same API as players, so it cannot cheat on rules.
   Visibility is recomputed per player after every action from Bresenham lines
   with wrap-around.

6. **Deterministic RNG.** A xorshift64* generator is seeded from the title
   screen (scripts set the seed). There is no other source of randomness.

7. **Audio.** A small SID-inspired synthesiser: 8 voices, each with a
   waveform (pulse, triangle, saw, noise), a frequency slide and an ADSR
   envelope, mixed into 16-bit mono at 22050 Hz with clamping. Effects are
   tables of parameters. The platform pulls samples through an
   `SDL_AudioStream`.

8. **Art.** All sprites, tiles, icons and the 5x7 font are original and
   defined in C as strings of palette hex digits. There are no binary asset
   files, so the web build is a single `.wasm` file.

9. **Builds.** CMake 3.24+. SDL3 comes from `find_package(SDL3)` when
   available, otherwise FetchContent of a pinned release (also used for
   Emscripten). CI matrix: ubuntu (gcc with coverage, game tests, smoke test),
   windows (MSVC build and unit tests), emscripten (build and Pages artifact).
   The Pages deploy job runs only on the default branch.

## Risks / Trade-offs

- [Touch on small screens] A 320x200 screen scaled to a phone gives about
  10 mm squares in landscape. Mitigation: tap-to-move-cursor/tap-again-to-act,
  and tapping a menu entry chooses it directly.
- [Browser audio unlock] Browsers block audio until a user gesture.
  Mitigation: the audio device is opened on the first input event on the web.
- [Coverage on rendering code] Pixel code has many branches. Mitigation:
  tests render every screen and mode and check palette invariants and
  specific pixels.
- [Game balance] Numbers are first guesses and will be tuned in playtesting.

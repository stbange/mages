# Mages

An original turn-based wizard battle game in the style of late-1980s 8-bit
home computers: a 320x200 screen, the classic 16-colour palette, chunky pixel
art and synthesised sound effects. There is one twist: you can step into the
eyes of any of your own creatures and see the world in first-person 3D, drawn
in the same palette and style.

**Play in the browser:** https://stbange.github.io/mages/ (works on phones,
best in landscape).

## The game

1–4 players share one computer (hot seat), plus up to three computer wizards.
Summon creatures, explore the Sundered Vale, fight with steel and spells, and
be the first wizard to escape through the portal when it opens on turn 12, or
be the last wizard standing.

- **Action points** pay for everything. **Stamina** runs out when you push too
  hard. Wounded creatures get weaker.
- **Engagement:** a creature next to an enemy can't walk away. It has to
  fight (or fly off).
- **Flying, riding, doors, objects** (potions, rune swords that hurt the
  undead, shields).
- **Hidden movement and line of sight:** you only see what your creatures
  see.
- **20 spells:** 12 summons, lightning, fireball, sparks, heal, invisibility,
  quicken, teleport and banish.

### Controls

| Action | Keyboard | Gamepad | Touch / mouse |
|---|---|---|---|
| Move cursor | Arrows, WASD, QAOP, numpad (diagonals: numpad 7/9/1/3, Home/PgUp/End/PgDn) | D-pad, left stick | Tap a square |
| Fire / choose | Space, Enter | A | Tap the cursor square again, or tap a menu entry |
| Cancel | Esc, Backspace | B | Tap outside the menu (right click with a mouse) |
| Menu | Fire on the cursor | A | Tap the info panel |
| Next creature | Tab, N | RB | Menu → NEXT |
| 3D view | V | Y | Menu → VIEW 3D; tap left/right third to turn, middle to step, bottom strip to exit |
| Fullscreen | F11, Alt+Enter | | |

With a creature selected, fire on a neighbouring square to move or attack.
The cursor keeps its offset, so pressing fire again keeps walking.

## Building

You need CMake 3.24 or newer and a C11 compiler. SDL3 is used when it is
installed. Otherwise it is downloaded and built automatically.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build
```

```bash
ctest --test-dir build --output-on-failure
```

Web build, with the [Emscripten SDK](https://emscripten.org) active:

```bash
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release && cmake --build build-web
```

Coverage of the platform-free core (requires gcovr; CI fails below 95%):

```bash
cmake -S . -B build-cov -DMAGES_COVERAGE=ON -DMAGES_BUILD_APP=OFF && cmake --build build-cov && ctest --test-dir build-cov && cmake --build build-cov --target coverage
```

## Layout

- `src/core/` contains the rules, map, line of sight, spells, AI, software
  renderer (2D and raycast 3D), audio synthesiser, UI state machine and the
  game-test script engine. It has no SDL dependency and is fully unit tested.
- `src/platform/main_sdl.c` is the thin SDL3 layer: window, input, audio and
  the main loop.
- `tests/unit/` holds the unit tests.
- `tests/game/*.script` holds automated game tests that drive the game with
  keys and taps (`game_runner` runs them headless and saves screenshots).
- `openspec/` holds the specifications and change proposals
  ([OpenSpec](https://github.com/Fission-AI/OpenSpec)).
- `web/shell.html` is the browser page.

All art, the font, the map and the text are original.

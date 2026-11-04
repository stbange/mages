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

Mages plays like a modern tactics game. Point at things to inspect them,
click your creatures to select them, and click where they should go.

**Mouse and keyboard (Windows, Linux, browser)**

| Action | Mouse | Keyboard |
|---|---|---|
| Inspect a square or creature | Hover | Move the cursor with Arrows/WASD (numpad or Home/PgUp/End/PgDn for diagonals) |
| Select your creature | Left click | Enter on it, or Tab / Shift+Tab for next/previous creature with AP left |
| Move (any distance, through doors) | Left click a green square (the path and AP cost are shown) | Enter |
| Attack (walks next to the enemy first) | Left click a red-marked enemy | Enter |
| Open an adjacent door | Click the door | U |
| Cancel / deselect | Right click | Esc |
| Scroll the map | Drag | Move the cursor to the edge |
| Next creature | Mouse wheel | Tab |
| Spell book | CAST button | C, then 1–9 (wheel or MORE SPELLS to page) |
| Fly/land, pick up, drop, drink, ride/dismount | Buttons | F, G, X, Q, R |
| 3D view of the selected creature | 3D VIEW button | V |
| End turn | END TURN button (press again to confirm if creatures can still act) | T |
| Help | HELP button | H or F1 |
| Fullscreen | | F11, Alt+Enter |

**In the 3D view:** turn with A/D, the arrow keys, the wheel, a click on the
left or right third, or a drag. Step forward with W/Up or a click in the
middle. Tab looks through the next creature's eyes. Esc, V or right-click
returns to the map.

**Phones and tablets (best in landscape)**

- Tap your creature to select it.
- Tap a square to see the move, attack or spell (path, AP cost and target),
  then tap it again to do it.
- Drag with one finger to scroll the map.
- Use the big buttons for everything else.
- In 3D, swipe to turn, tap the middle to step, and press EXIT 3D to return.

**Saving:** the game saves itself at the start of each of your turns, so
closing the window or reloading the browser tab loses nothing. Choose
CONTINUE on the title screen to carry on. Press Esc with nothing selected
(or the MENU button) for the game menu, where QUIT TO TITLE keeps your save.
On Windows and Linux the save lives in your user data folder; in the browser
it lives in the page's local storage.

**Gamepad:** d-pad or stick moves the cursor, A acts, B cancels, X casts,
Y opens 3D, the shoulder buttons cycle creatures, Start ends the turn and
Back shows help.

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

See [ROADMAP.md](ROADMAP.md) for what comes next.

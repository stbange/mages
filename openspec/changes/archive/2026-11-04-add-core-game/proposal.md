# Proposal

## Why

There is no game yet. This change delivers the first playable version of
*Mages*: an original turn-based wizard battle game with an 8-bit home-computer
look. It also adds a twist the classic genre lacked: you can jump into any
friendly creature's eyes and see the world in first-person 3D. The version
must be playable on desktop (Windows, Linux) and in the browser, including
phones, so friends can playtest from a link.

## What Changes

- New platform-free C core containing the game rules, world map, line of
  sight, spells, computer opponent, 2D/3D software renderer and sound
  synthesiser.
- New SDL3 front end for Windows, Linux and Web (Emscripten) with keyboard,
  mouse, gamepad and touch input.
- One scenario ("The Sundered Vale") for 1-4 hot-seat players plus up to
  three computer wizards.
- Unit tests (>= 95% core line coverage) and scripted automated game tests.
- CI on GitHub Actions for all three platforms; web build deployed to
  GitHub Pages.

Out of scope for this change: music, wizard designer and experience
points between scenarios, more scenarios, save/load, online multiplayer.

## Capabilities

### New Capabilities
- `game-rules`: turns, action points, stamina, movement, terrain, combat, engagement, flying, riding, objects, and victory.
- `spellcasting`: the wizard spell book, mana, summoning, attack and utility spells.
- `visibility`: line of sight, fog of war, and hidden movement between players.
- `computer-opponent`: behaviour of computer wizards and independent creatures.
- `map-view`: the classic top-down screen, cursor, menus, info panel, and message line.
- `first-person-view`: the 3D point-of-view mode for friendly creatures.
- `input-controls`: keyboard, gamepad, mouse and touch controls.
- `sound-effects`: synthesised retro sound effects for game events.
- `platform-delivery`: desktop and web builds and the GitHub Pages deployment.
- `quality-gates`: coverage threshold and automated game tests.

### Modified Capabilities

## Impact

New repository content: `src/core`, `src/platform`, `tests`, `web`,
`CMakeLists.txt`, `.github/workflows`. External dependency: SDL3 (fetched at
build time). No existing code is affected.

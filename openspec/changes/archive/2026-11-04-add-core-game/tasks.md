# Tasks

## 1. Project setup

- [x] 1.1 CMake project with core library, SDL3 app, headless test runner and Emscripten target
- [x] 1.2 Minimal unit test harness and ctest integration
- [x] 1.3 Coverage target (gcovr, fail under 95% line coverage of src/core)

## 2. Core rules

- [x] 2.1 RNG, palette, map with terrain table and wrap-around
- [x] 2.2 Creature and object tables; scenario "The Sundered Vale"
- [x] 2.3 Turns, AP, stamina, wounds, movement, engagement, flying
- [x] 2.4 Combat, undead rule, death, victory points
- [x] 2.5 Doors, objects, riding
- [x] 2.6 Portal and victory conditions
- [x] 2.7 Line of sight, fog of war, invisibility

## 3. Spells and computer opponent

- [x] 3.1 Twenty spells: summons, attack spells and utility spells
- [x] 3.2 Computer wizard and independent creature AI

## 4. Presentation

- [x] 4.1 Font, sprites, tiles and icons (original art)
- [x] 4.2 Map view, info panel, menus, message line, title, hand-over and game-over screens
- [x] 4.3 UI state machine with keyboard/gamepad/pointer inputs
- [x] 4.4 First-person raycast view with animated turning and stepping
- [x] 4.5 Sound effect synthesiser and event-to-sound mapping

## 5. Platform

- [x] 5.1 SDL3 front end: window scaling, keyboard, gamepad, mouse, touch, audio
- [x] 5.2 Emscripten shell page for desktop and mobile browsers

## 6. Quality and delivery

- [x] 6.1 Unit tests for every core module with >= 95% line coverage
- [x] 6.2 Script-driven automated game tests and headless SDL smoke test
- [x] 6.3 GitHub Actions: Linux, Windows and Web builds; Pages deploy

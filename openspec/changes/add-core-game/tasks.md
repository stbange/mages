# Tasks

## 1. Project setup

- [ ] 1.1 CMake project with core library, SDL3 app, headless test runner and Emscripten target
- [ ] 1.2 Minimal unit test harness and ctest integration
- [ ] 1.3 Coverage target (gcovr, fail under 95% line coverage of src/core)

## 2. Core rules

- [ ] 2.1 RNG, palette, map with terrain table and wrap-around
- [ ] 2.2 Creature and object tables; scenario "The Sundered Vale"
- [ ] 2.3 Turns, AP, stamina, wounds, movement, engagement, flying
- [ ] 2.4 Combat, undead rule, death, victory points
- [ ] 2.5 Doors, objects, riding
- [ ] 2.6 Portal and victory conditions
- [ ] 2.7 Line of sight, fog of war, invisibility

## 3. Spells and computer opponent

- [ ] 3.1 Twenty spells: summons, attack spells and utility spells
- [ ] 3.2 Computer wizard and independent creature AI

## 4. Presentation

- [ ] 4.1 Font, sprites, tiles and icons (original art)
- [ ] 4.2 Map view, info panel, menus, message line, title, hand-over and game-over screens
- [ ] 4.3 UI state machine with keyboard/gamepad/pointer inputs
- [ ] 4.4 First-person raycast view with animated turning and stepping
- [ ] 4.5 Sound effect synthesiser and event-to-sound mapping

## 5. Platform

- [ ] 5.1 SDL3 front end: window scaling, keyboard, gamepad, mouse, touch, audio
- [ ] 5.2 Emscripten shell page for desktop and mobile browsers

## 6. Quality and delivery

- [ ] 6.1 Unit tests for every core module with >= 95% line coverage
- [ ] 6.2 Script-driven automated game tests and headless SDL smoke test
- [ ] 6.3 GitHub Actions: Linux, Windows and Web builds; Pages deploy

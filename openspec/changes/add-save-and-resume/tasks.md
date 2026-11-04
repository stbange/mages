# Tasks

## 1. Core

- [x] 1.1 `save.c`: versioned, checksummed little-endian serialisation of the game
- [x] 1.2 Load with full validation (magic, version, length, checksum, value ranges)
- [x] 1.3 UI: save requests at human turn start and game over, `ui_save`, `ui_offer_save`
- [x] 1.4 Title CONTINUE entry; resume into PLAY or the hand-over screen
- [x] 1.5 Game menu (RESUME, HELP, QUIT TO TITLE) via Escape and the MENU button

## 2. Platform

- [x] 2.1 Desktop save file in the per-user preference folder
- [x] 2.2 Browser save in localStorage (base64) with error handling

## 3. Tests

- [x] 3.1 Round-trip, determinism and corruption unit tests
- [x] 3.2 UI tests for continue, game menu and save requests
- [x] 3.3 Game script: play, save, resume, continue identically

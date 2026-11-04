# Proposal

## Why

Phone browsers discard background tabs, and desktop players close the
window. Today any of that loses the game. A long turn-based game needs to
survive interruptions, and the planned campaign needs persistent state.
There is also no way to leave a running game without finishing it.

## What Changes

- Games are saved automatically at the start of every human turn, after the
  computer has moved. On desktop the save goes to a file in the user's
  per-app data folder; in the browser it goes to the page's local storage.
- The title screen offers CONTINUE when a save exists. Continuing restores
  the exact game, including the random number state. Hot-seat games resume
  behind the hand-over screen.
- The save is removed when the game ends.
- A game menu (Escape with nothing selected, or the MENU button) offers
  RESUME, HELP and QUIT TO TITLE. Quitting keeps the save.
- Save files are versioned and checksummed. Damaged or outdated saves are
  ignored, never loaded.

## Capabilities

### New Capabilities
- `save-game`: automatic saving, resuming, the save format and its validation, and where saves are stored.

### Modified Capabilities
- `map-view`: the title screen gains CONTINUE, and a game menu is added.

## Impact

New `src/core/save.c`, plus UI changes (title entry, game menu, save
requests). The SDL layer gains file and browser storage. New tests and a game
script. Game rules are unchanged.

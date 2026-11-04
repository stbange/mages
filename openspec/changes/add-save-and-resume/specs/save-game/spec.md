# Spec Delta

## Purpose

Lets a game survive closed windows and reloaded browser tabs by saving it
automatically and resuming it exactly from the title screen.

## ADDED Requirements

### Requirement: Autosave
The game SHALL save automatically when a human player's turn begins, after any computer turns have been played.

#### Scenario: Save at turn start
- **WHEN** a human ends their turn and the computer has moved
- **THEN** the game is saved before the human can act again

### Requirement: Continue
When a valid save exists, the title screen SHALL offer CONTINUE. Choosing it SHALL restore the saved game exactly, including the map, creatures, objects, fog of war, victory points, turn and random number state. A game with two or more human players SHALL resume behind the hand-over screen.

#### Scenario: Exact resume
- **WHEN** a saved game is continued and played with the same inputs as the original
- **THEN** it develops exactly as the original game would have

#### Scenario: Hot-seat resume
- **WHEN** a two-player hot-seat game is continued
- **THEN** the hand-over screen for the player to move is shown first

### Requirement: Save lifetime
The save SHALL be deleted when the game ends. Starting a new game SHALL replace the save at its first human turn.

#### Scenario: Game over clears the save
- **WHEN** a game ends with a winner
- **THEN** the title screen no longer offers CONTINUE

### Requirement: Save validation
Saves SHALL carry a format identifier, a version and a checksum. Saves that are truncated, damaged or of another version SHALL be ignored, and CONTINUE SHALL not be offered for them.

#### Scenario: Damaged save
- **WHEN** a byte of the save has been changed
- **THEN** the save is rejected and the title screen offers only a new game

### Requirement: Save storage
On Windows and Linux the save SHALL be a file in the user's per-application data folder. In the browser it SHALL be kept in the page's local storage so that it survives reloads.

#### Scenario: Browser reload
- **WHEN** the web page is reloaded during a game
- **THEN** the title screen offers CONTINUE

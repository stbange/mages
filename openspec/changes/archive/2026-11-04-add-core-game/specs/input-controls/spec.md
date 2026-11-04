# Spec Delta

## Purpose

Defines how players control the game with a keyboard, gamepad, mouse or
touch screen, so it can be played on desktop and phones.

## ADDED Requirements

### Requirement: Keyboard and gamepad
Arrow keys, WASD and QAOP SHALL move the cursor, numpad and Home/PgUp/End/PgDn SHALL move diagonally, Space/Enter SHALL be fire and Escape/Backspace SHALL cancel. Gamepad d-pad and stick SHALL move, A SHALL be fire and B SHALL cancel.

#### Scenario: Fire with keyboard
- **WHEN** Enter is pressed in cursor mode
- **THEN** the menu opens

### Requirement: Pointer and touch
A tap or click on a map square SHALL move the cursor there. A second tap on the cursor square SHALL act as fire. A tap on a menu entry SHALL choose it, and a tap outside an open menu SHALL cancel it. A tap on the info panel SHALL open the menu.

#### Scenario: Tap to move
- **WHEN** a creature is selected and the player taps an adjacent square twice
- **THEN** the creature moves there

#### Scenario: Tap a menu entry
- **WHEN** a menu is open and the player taps an entry
- **THEN** that entry is chosen

### Requirement: Touch in 3D view
In the 3D view, taps on the left or right third SHALL turn, a tap in the middle SHALL step forward, and a tap on the exit strip at the bottom SHALL leave the view.

#### Scenario: Tap to turn
- **WHEN** the player taps the left third of the 3D view
- **THEN** the view turns left

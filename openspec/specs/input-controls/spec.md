# input-controls Specification

## Purpose
Defines how players control the game with a keyboard, gamepad, mouse or
touch screen, so it can be played on desktop and phones.

## Requirements

### Requirement: Keyboard and gamepad
Arrow keys, WASD and the numpad SHALL move the cursor. Enter/Space SHALL act on the cursor square and Escape SHALL cancel. Hotkeys SHALL be: Tab/Shift+Tab next/previous creature, C cast, F fly/land, G pick up, X drop, Q drink, U door, R ride/dismount, V 3D view, T end turn, H/F1 help, 1-9 spells. On a gamepad, the d-pad moves, A acts, B cancels, X casts, Y opens 3D, the shoulder buttons cycle creatures and Start ends the turn.

#### Scenario: Fire with keyboard
- **WHEN** Enter is pressed with the cursor on an own creature
- **THEN** that creature is selected

#### Scenario: Act with keyboard
- **WHEN** a creature is selected and Enter is pressed on a reachable square
- **THEN** the creature walks there

#### Scenario: Hotkey
- **WHEN** C is pressed with a wizard selected
- **THEN** the spell book opens

### Requirement: Pointer and touch
With a mouse, moving the pointer SHALL move the cursor, left-click SHALL act at once, right-click SHALL cancel or deselect, the wheel SHALL cycle creatures and dragging SHALL pan the map. On touch, a tap on an own creature or a button SHALL act at once. A tap on any other square SHALL preview it, and a second tap on the same square SHALL confirm. A one-finger drag SHALL pan the map.

#### Scenario: Click to move
- **WHEN** a creature is selected and the player left-clicks a reachable square
- **THEN** the creature walks there

#### Scenario: Tap to preview then confirm
- **WHEN** a creature is selected and the player taps a reachable square once
- **THEN** the path and cost are shown but the creature does not move until the square is tapped again

#### Scenario: Tap to move
- **WHEN** a creature is selected and the player taps an adjacent square twice
- **THEN** the creature moves there

#### Scenario: Tap a menu entry
- **WHEN** the spell book is open and the player taps an entry
- **THEN** that entry is chosen

#### Scenario: Drag to pan
- **WHEN** the player drags across the map by more than one square
- **THEN** the map scrolls and no square is acted on

### Requirement: Touch in 3D view
In the 3D view, a horizontal swipe SHALL turn the view and a tap in the middle third SHALL step forward. Taps on the left or right third SHALL turn, and an EXIT button SHALL leave the view.

#### Scenario: Tap to turn
- **WHEN** the player taps the left third of the 3D view
- **THEN** the view turns left

#### Scenario: Swipe to turn
- **WHEN** the player swipes left across the 3D view
- **THEN** the view turns

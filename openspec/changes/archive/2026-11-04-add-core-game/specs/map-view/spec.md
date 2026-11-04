# Spec Delta

## Purpose

Defines the classic top-down presentation: a 320x200 screen with the C64
palette, a scrolling map window, an info panel with status icons and bar
graphs, menus, and a message line.

## ADDED Requirements

### Requirement: Screen layout
The game SHALL render to a 320x200 pixel screen using only the 16-colour C64 palette. The screen SHALL have a framed map window on the left, an info panel on the right and a message line at the bottom.

#### Scenario: Palette only
- **WHEN** any screen is rendered
- **THEN** every pixel is one of the 16 palette colours

### Requirement: Cursor and scrolling
A flashing cursor SHALL move over the map. The map window SHALL scroll to keep the cursor in view. The message line SHALL name the creature, object or terrain under the cursor.

#### Scenario: Cursor at window edge
- **WHEN** the cursor moves past the edge of the map window
- **THEN** the window scrolls so the cursor stays visible

### Requirement: Info panel
The info panel SHALL show the creature under the cursor: its name, owner, status icons (undead, flying, mount, wounded, invisible), carried objects, and bars for AP, stamina, constitution, combat and defence. For wizards it SHALL also show mana.

#### Scenario: Inspecting a creature
- **WHEN** the cursor is over a visible creature
- **THEN** its name and stat bars are shown in the panel

### Requirement: Menus
Pressing fire SHALL open a context menu listing only the actions that are currently possible. A CANCEL entry SHALL always be present. Ending a turn SHALL require confirmation.

#### Scenario: Selected wizard menu
- **WHEN** fire is pressed on the selected wizard
- **THEN** the menu offers CAST, END MOVE and CANCEL among the available actions

#### Scenario: End turn confirmation
- **WHEN** END TURN is chosen
- **THEN** a confirmation menu appears and the turn only ends if confirmed

### Requirement: Title and game over screens
A title screen SHALL let players choose 1-4 human and 0-3 computer wizards (2-4 wizards in total). A game-over screen SHALL announce the winner and victory points.

#### Scenario: Start a game
- **WHEN** the default settings are accepted on the title screen
- **THEN** a game starts with one human and one computer wizard

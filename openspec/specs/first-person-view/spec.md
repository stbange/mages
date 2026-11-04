# first-person-view Specification

## Purpose
The game's twist: let a player look through the eyes of any of their own
creatures in a 3D first-person view drawn in the same palette and art style.

## Requirements

### Requirement: Enter point of view
Players SHALL be able to enter the first-person view of any creature they own, from the cursor or the selection menu. They SHALL NOT be able to do so for enemy or independent creatures.

#### Scenario: Enemy creature
- **WHEN** the cursor is over an enemy creature
- **THEN** no VIEW 3D option is offered

### Requirement: 3D rendering
The view SHALL be drawn into the map window using the 16-colour palette. Walls, doors and mountains SHALL be drawn as textured blocks, and creatures, trees and objects as sprites. Roofed squares SHALL show a ceiling and open squares a sky. Only things the player can currently see SHALL be drawn.

#### Scenario: Invisible enemy
- **WHEN** an invisible enemy stands in front of the viewing creature
- **THEN** it is not drawn

### Requirement: Looking and moving
In the 3D view, left and right SHALL turn the view by 45 degrees with a short animation. Forward SHALL move the creature one square in the facing direction using the normal movement rules. Fire or cancel SHALL return to the map.

#### Scenario: Step forward
- **WHEN** forward is pressed in the 3D view of a creature with enough AP
- **THEN** the creature moves one square in the facing direction and the view follows

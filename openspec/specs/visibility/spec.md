# visibility Specification

## Purpose
Defines what each player can see: line of sight from their creatures, fog of
war, and hidden movement so players cannot watch each other's turns.

## Requirements

### Requirement: Line of sight
A player SHALL see a square when one of their creatures is within sight range and no opaque square (wall, closed door, tree, mountain) lies strictly between them.

#### Scenario: Wall blocks sight
- **WHEN** a wall lies on the straight line between a creature and a square
- **THEN** the square is not visible from that creature

### Requirement: Fog of war
Squares a player has never seen SHALL be drawn black. Squares seen earlier SHALL show terrain only. Enemy creatures SHALL only be drawn on currently visible squares, and invisible enemies SHALL never be drawn.

#### Scenario: Remembered terrain
- **WHEN** a square was seen earlier but is no longer in sight
- **THEN** its terrain is drawn but creatures on it are not

### Requirement: Hidden movement
Computer turns SHALL NOT be shown. When two or more humans play, a hand-over screen SHALL hide the map between human turns.

#### Scenario: Hand-over between humans
- **WHEN** human player 1 ends their turn and human player 2 is next
- **THEN** a screen naming player 2 is shown and the map stays hidden until they press fire

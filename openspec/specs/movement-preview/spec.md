# movement-preview Specification

## Purpose
Shows players where a selected creature can go and what it costs, and lets a
single click carry out multi-square moves and attacks.

## Requirements

### Requirement: Reachable squares
When a player's creature is selected during their turn, the map SHALL highlight every square it can reach this turn with its remaining AP. This SHALL follow the movement rules: terrain costs, blocked squares, flying, doors it can open, and stopping next to visible enemies.

#### Scenario: Highlight after selection
- **WHEN** a goblin with 12 AP is selected on open grass
- **THEN** every grass square within 4 steps is highlighted and squares beyond are not

#### Scenario: Engaged creature
- **WHEN** the selected creature is engaged
- **THEN** no squares are highlighted for movement

### Requirement: Attackable enemies
Visible enemies that the selected creature can reach and still attack SHALL be highlighted differently from reachable squares.

#### Scenario: Enemy in reach
- **WHEN** an enemy is two squares away and the selected creature has enough AP to step next to it and attack
- **THEN** the enemy's square is highlighted as attackable

### Requirement: Path preview
Hovering a reachable square SHALL show the path the creature will take and the message line SHALL state the AP cost.

#### Scenario: Hover shows cost
- **WHEN** the pointer rests on a reachable square three grass squares away
- **THEN** the path is drawn and the message line reads "MOVE: 9 AP"

### Requirement: Multi-square move
Acting on a reachable square SHALL move the creature along the previewed path one square at a time, opening closed doors on the way if it has hands. The walk SHALL stop early if a step is refused, for example by an enemy that was hidden.

#### Scenario: Walk through a door
- **WHEN** a wizard inside its house acts on a square outside the closed door
- **THEN** the wizard walks to the door, opens it and continues to the square

#### Scenario: Ambush stops the walk
- **WHEN** a hidden enemy becomes adjacent during a walk
- **THEN** the walk stops at that square

### Requirement: Move and attack
Acting on an attackable enemy SHALL move the creature next to it by the cheapest path and then attack.

#### Scenario: Charge
- **WHEN** a troll acts on an enemy goblin two squares away
- **THEN** the troll steps next to the goblin and attacks it

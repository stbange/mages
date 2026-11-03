# Spec Delta

## Purpose

Defines how a game of Mages is played: turn order, action points, stamina,
movement over terrain, combat, flying, riding, objects, and how a game is won.

## ADDED Requirements

### Requirement: Turn order
Each game turn SHALL begin with the independent creatures acting, followed by each surviving wizard player in seat order. Turn 1 SHALL allow casting but no movement.

#### Scenario: Independent creatures act first
- **WHEN** the last wizard of turn N ends their turn
- **THEN** independent creatures act, the turn counter becomes N+1 and the first surviving wizard player starts

#### Scenario: No movement on turn one
- **WHEN** a player tries to move a creature during game turn 1
- **THEN** the move is refused with a message and no action points are spent

### Requirement: Action points
Every creature SHALL have an action point (AP) allowance restored at the start of its owner's turn. Moving, attacking, casting and using objects SHALL cost AP, and an action SHALL be refused when the creature lacks the AP.

#### Scenario: Insufficient action points
- **WHEN** a creature with 1 AP left tries to move onto grass costing 3 AP
- **THEN** the move is refused and the creature stays where it is

### Requirement: Stamina and exhaustion
Moving and fighting SHALL consume stamina. Stamina SHALL partly recover each turn. A creature whose stamina is below a quarter of its maximum at the start of its turn SHALL receive only half its AP allowance.

#### Scenario: Exhausted creature
- **WHEN** a creature starts its owner's turn with stamina below 25% of maximum
- **THEN** its AP is set to half its allowance

### Requirement: Terrain movement
Ground creatures SHALL move one square in any of eight directions, paying the terrain's AP cost. Walls, closed doors, water and mountains SHALL block ground movement. The map SHALL wrap around at its edges.

#### Scenario: Blocked by water
- **WHEN** a ground creature tries to move into water
- **THEN** the move is refused

#### Scenario: Wrap-around
- **WHEN** a creature on the east edge moves east
- **THEN** it appears on the west edge of the same row

### Requirement: Flying
Creatures able to fly SHALL be able to take off and land outdoors. Airborne creatures SHALL pay a fixed AP cost per square, cross water, trees and mountains, never enter roofed squares, and SHALL only fight other airborne creatures.

#### Scenario: Cannot take off indoors
- **WHEN** a flying creature on a roofed square tries to take off
- **THEN** the action is refused

#### Scenario: Flying over water
- **WHEN** an airborne creature moves over water
- **THEN** the move succeeds

### Requirement: Engagement
A creature adjacent to a visible enemy on the same layer SHALL be engaged and SHALL NOT move, except by attacking or by taking off.

#### Scenario: Engaged creature cannot walk away
- **WHEN** an engaged creature tries to move to an empty square
- **THEN** the move is refused with an "engaged" message

### Requirement: Melee combat
Moving into an enemy SHALL attack it. Hit chance SHALL depend on attacker combat versus defender defence, damage SHALL reduce constitution, and a creature at zero constitution SHALL die. A surviving, non-exhausted defender MAY strike back.

#### Scenario: Killing a creature
- **WHEN** an attack reduces the defender's constitution to zero or below
- **THEN** the defender is removed and the attacker's player gains victory points

### Requirement: Wounds
A creature below half its maximum constitution SHALL have its AP allowance, combat and defence reduced by a quarter.

#### Scenario: Wounded creature is weaker
- **WHEN** a creature's constitution falls below 50%
- **THEN** its effective combat and defence are 75% of normal

### Requirement: Undead
Undead creatures SHALL only be damaged by undead attackers, attackers wielding a magic weapon, or spells.

#### Scenario: Ordinary attack on undead
- **WHEN** a living creature without a magic weapon hits an undead creature
- **THEN** the undead creature takes no damage

### Requirement: Riding
Creatures able to ride SHALL be able to mount an adjacent friendly mount that has no rider. The mount then carries the rider, and dismounting SHALL place the rider on a free adjacent square.

#### Scenario: Wizard rides a mount
- **WHEN** a wizard adjacent to a friendly unridden mount chooses RIDE
- **THEN** the wizard leaves the map, travels with the mount and can still cast spells

#### Scenario: Mount dies
- **WHEN** a mount carrying a rider is killed
- **THEN** the rider is placed on the mount's square

### Requirement: Doors
A creature with hands SHALL be able to open or close an adjacent door.

#### Scenario: Opening a door
- **WHEN** a creature with hands uses an adjacent closed door
- **THEN** the door opens and becomes passable and transparent

### Requirement: Objects
Creatures with hands SHALL pick up, drop and carry up to four objects. Carried swords SHALL raise combat (and count as magic weapons), and carried shields SHALL raise defence. Potions SHALL be drunk to restore constitution or mana.

#### Scenario: Picking up a sword
- **WHEN** a creature with hands picks up a sword
- **THEN** its effective combat increases

#### Scenario: Creature without hands
- **WHEN** a creature without hands tries to pick up an object
- **THEN** the action is refused

### Requirement: Victory
A portal SHALL open at a scenario-defined square and turn. A wizard who steps onto the open portal SHALL win. A wizard who dies SHALL be eliminated together with all their creatures, and the last surviving wizard SHALL win.

#### Scenario: Escape through the portal
- **WHEN** a wizard moves onto the open portal
- **THEN** that wizard's player wins and the game-over screen is shown

#### Scenario: Last wizard standing
- **WHEN** all other wizards have died
- **THEN** the remaining wizard's player wins

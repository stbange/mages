# Spec Delta

## Purpose

Defines the wizard's spell book: how spells cost mana and AP, and what
summoning, attack and utility spells do.

## ADDED Requirements

### Requirement: Spell book and mana
Every wizard SHALL know twenty spells, each with a mana cost and level. Casting SHALL cost mana and 4 AP. Mana SHALL regenerate slowly each turn. Spells the wizard cannot afford SHALL be shown in red and SHALL NOT be castable.

#### Scenario: Not enough mana
- **WHEN** a wizard with 5 mana selects a spell costing 10
- **THEN** the spell is refused and mana is unchanged

### Requirement: Summoning
Summon spells SHALL create as many creatures of the spell's type as its level, on free squares next to the wizard, owned by the caster. Summoned creatures SHALL NOT move in the turn they are summoned.

#### Scenario: Summon a level 2 spell
- **WHEN** a wizard with free adjacent squares casts a level 2 summon spell
- **THEN** two creatures of that type appear next to the wizard with 0 AP

#### Scenario: No room
- **WHEN** every square next to the wizard is occupied or blocked
- **THEN** the spell fails and no mana is spent

### Requirement: Targeted spells
Attack and utility spells SHALL target a square within range that the caster can see in line of sight.

#### Scenario: Target out of sight
- **WHEN** a wizard targets a square behind a wall
- **THEN** the spell is refused

#### Scenario: Lightning bolt
- **WHEN** a lightning bolt hits an enemy creature
- **THEN** the creature loses constitution even if it is undead

#### Scenario: Fireball splash
- **WHEN** a fireball hits a square
- **THEN** the creature on that square and creatures on adjacent squares take damage

### Requirement: Utility spells
The spell book SHALL include heal, invisibility, quicken (restore AP), teleport and banish.

#### Scenario: Invisibility
- **WHEN** a friendly creature is made invisible
- **THEN** enemy players cannot see it for three of its owner's turns

#### Scenario: Teleport
- **WHEN** a wizard casts teleport onto a visible empty square
- **THEN** the wizard is moved to that square

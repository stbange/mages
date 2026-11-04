# Spec Delta

## Purpose

Defines how computer-controlled wizards and independent creatures behave, so
single players have a meaningful opponent.

## ADDED Requirements

### Requirement: Computer wizard
A computer wizard SHALL summon creatures while it has mana, attack visible enemies with its creatures and spells, and move its wizard to the portal once it is open. It SHALL only use information its own creatures can see.

#### Scenario: Computer summons on turn one
- **WHEN** a computer wizard takes turn 1 with enough mana
- **THEN** it has more than one creature at the end of its turn

#### Scenario: Computer heads for the portal
- **WHEN** the portal is open and visible to the computer wizard
- **THEN** its wizard moves toward the portal

### Requirement: Independent creatures
Independent creatures SHALL attack adjacent creatures and approach visible creatures within six squares. They SHALL otherwise stay near where they are.

#### Scenario: Independent creature attacks
- **WHEN** a player's creature stands next to an independent creature at the start of the independent phase
- **THEN** the independent creature attacks it

### Requirement: Determinism
Given the same seed and the same inputs, a game SHALL play out identically.

#### Scenario: Replayed game
- **WHEN** the same script is run twice with the same seed
- **THEN** the final game state is identical

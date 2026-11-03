# Spec Delta

## Purpose

Gives game events audible feedback with synthesised 8-bit style sound
effects. Music is deferred.

## ADDED Requirements

### Requirement: Synthesised effects
The game SHALL synthesise sound effects from oscillators (pulse, triangle, sawtooth, noise) with envelopes, without sample files. Effects SHALL play for cursor moves, menu choices, steps, hits, misses, deaths, spell casts, summons, doors, the portal opening and the start of a turn.

#### Scenario: Attack hit sound
- **WHEN** a melee attack hits
- **THEN** the hit sound effect is queued

### Requirement: Mixing
Several effects SHALL be able to play at once, and the output SHALL never clip outside the 16-bit sample range.

#### Scenario: Many overlapping effects
- **WHEN** eight effects play at the same time
- **THEN** every output sample is within the 16-bit range

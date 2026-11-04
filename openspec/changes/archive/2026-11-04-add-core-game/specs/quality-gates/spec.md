# Spec Delta

## Purpose

Keeps the game correct as it grows: a unit test coverage threshold and
scripted automated game tests that drive the game the way a player does.

## ADDED Requirements

### Requirement: Unit test coverage
Unit tests SHALL cover at least 95% of the lines of the platform-free core. CI SHALL fail when coverage drops below that.

#### Scenario: Coverage drop
- **WHEN** a change lowers core line coverage below 95%
- **THEN** the CI coverage job fails

### Requirement: Automated game tests
Game test scripts SHALL drive the game through the same input path as players (keys and taps) with a fixed seed, and SHALL assert on game state and screen contents. They SHALL run headless in CI.

#### Scenario: Scripted game
- **WHEN** a game script starts a game, summons creatures, moves, opens the 3D view and ends turns
- **THEN** all its assertions pass in CI without a display

### Requirement: Platform smoke test
CI SHALL run the real SDL executable headless for a number of frames with scripted input and SHALL fail if it crashes or reports an error.

#### Scenario: Headless run
- **WHEN** the desktop executable runs with the dummy video driver and a script
- **THEN** it exits with status 0

# Spec Delta

## MODIFIED Requirements

### Requirement: Title and game over screens
A title screen SHALL let players choose 1-4 human and 0-3 computer wizards (2-4 wizards in total), and SHALL offer CONTINUE when a saved game exists. A game-over screen SHALL announce the winner and victory points.

#### Scenario: Start a game
- **WHEN** the default settings are accepted on the title screen
- **THEN** a game starts with one human and one computer wizard

#### Scenario: Continue is offered
- **WHEN** the title screen is shown and a valid save exists
- **THEN** CONTINUE is the highlighted first entry

## ADDED Requirements

### Requirement: Game menu
Escape with nothing selected, or the MENU button, SHALL open a game menu with RESUME, HELP and QUIT TO TITLE. QUIT TO TITLE SHALL keep the save.

#### Scenario: Leave a game
- **WHEN** the player chooses QUIT TO TITLE
- **THEN** the title screen is shown and offers CONTINUE

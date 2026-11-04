# Spec Delta

## MODIFIED Requirements

### Requirement: Cursor and scrolling
The cursor SHALL follow the mouse pointer or the keyboard. The message line SHALL describe the hovered square, or the action that acting on it would perform with its AP cost. The map window SHALL scroll to keep the keyboard cursor in view and SHALL also scroll by dragging.

#### Scenario: Cursor at window edge
- **WHEN** the keyboard cursor moves past the edge of the map window
- **THEN** the window scrolls so the cursor stays visible

#### Scenario: Describe the action
- **WHEN** a creature is selected and the pointer is over an attackable enemy
- **THEN** the message line names the attack and its AP cost

### Requirement: Info panel
The info panel SHALL show the hovered visible creature, otherwise the selected creature, otherwise the hovered square and the game status. A creature's details SHALL include its name, owner, status icons, carried objects, AP, mana for wizards, and bars for AP, stamina, constitution, combat and defence.

#### Scenario: Inspecting a creature
- **WHEN** the pointer hovers over a visible enemy while another creature is selected
- **THEN** the panel shows the enemy's details

## ADDED Requirements

### Requirement: Action bar
The info panel SHALL end with an action bar of buttons for the selected creature. It SHALL always contain 3D VIEW, NEXT and END TURN, plus CAST for wizards and up to two more of the currently possible actions (FLY/LAND, PICK UP, DRINK, DROP, RIDE/DISMOUNT, DOOR). Further actions SHALL go behind a MORE button. Buttons SHALL show their hotkey when a keyboard is in use and SHALL be at least 20 pixels tall.

#### Scenario: Wizard selected
- **WHEN** a wizard standing on a potion is selected
- **THEN** the action bar shows CAST, 3D VIEW, PICK UP, NEXT and END TURN

### Requirement: End turn confirmation
Ending a turn while any own creature can still act SHALL ask for confirmation by pressing END TURN again within three seconds. Otherwise the turn SHALL end at once.

#### Scenario: Creatures still able to act
- **WHEN** END TURN is pressed while the wizard has 14 AP left
- **THEN** the button shows CONFIRM? and the turn only ends when it is pressed again

### Requirement: Help overlay
Pressing H, F1 or the HELP button SHALL show the controls for the current input device over the map until any input is given.

#### Scenario: Touch help
- **WHEN** a touch player opens help
- **THEN** the overlay explains tapping, double-tapping and dragging

## REMOVED Requirements

### Requirement: Menus
**Reason**: The joystick-era fire-opens-menu flow is replaced by direct manipulation and the action bar.
**Migration**: Use clicks or taps on the map, the action bar buttons or hotkeys. Lists still appear for the spell book and the MORE button.

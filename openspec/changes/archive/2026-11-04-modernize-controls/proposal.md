# Proposal

## Why

The first version copied the joystick-era control scheme: one cursor, one
fire button, and every action hidden in a menu that opens on fire. That is
authentic, but slow with a mouse and awkward on a phone. Players expect
modern tactics-game controls: point at things to learn about them, click a
creature to select it, see where it can go, click to go there, and use
visible buttons or hotkeys for actions.

## What Changes

- **Mouse:** hovering inspects squares and creatures. Left-click selects, moves
  (with pathfinding over several squares), attacks (walking into range first
  if needed) and opens doors. Right-click cancels or deselects. The wheel
  cycles creatures. Dragging pans the map.
- **Movement range and path preview:** the selected creature's reachable
  squares and attackable enemies are highlighted. Hovering shows the path and
  its AP cost.
- **Action bar:** the joystick menus are replaced by on-screen buttons in the
  info panel (CAST, 3D VIEW, contextual actions, NEXT, END TURN). Each button
  has a hotkey.
- **Keyboard:** the cursor keys stay for keyboard-only play. New hotkeys:
  Tab/Shift+Tab for next/previous creature, C cast, F fly/land, G pick up,
  X drop, Q drink, U door, R ride/dismount, V 3D view, T end turn, H help,
  and 1-9 in the spell book.
- **Spell book:** shows descriptions on hover. Targeted spells highlight their
  valid targets.
- **End turn:** confirmation is only asked when creatures still have actions
  left.
- **Mobile:** tap a creature to select it. Tap a square to preview the move,
  attack or cast, and tap it again to confirm. Drag with one finger to pan.
  Buttons are at least 20 game pixels tall. In 3D, swipe to turn and tap to
  step.
- **Help overlay** listing the controls for the device in use.
- **BREAKING (controls):** the fire-opens-a-menu flow and the cursor/selection
  menus are removed.

## Capabilities

### New Capabilities
- `movement-preview`: reachable-square highlighting, path preview with AP cost, multi-square moves and move-and-attack.

### Modified Capabilities
- `input-controls`: keyboard, mouse, gamepad and touch mappings are redefined.
- `map-view`: menus are replaced by an action bar; hover inspection and a help overlay are added.
- `first-person-view`: new mouse, swipe and creature-cycling controls in 3D.

## Impact

`src/core/ui.c`, `src/core/render.c`, `src/core/script.c` (new script
commands), `src/core/data.*` (spell descriptions), `src/platform/main_sdl.c`
(mouse motion, drag and wheel), unit tests and game scripts. Game rules are
unchanged.

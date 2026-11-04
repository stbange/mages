# Tasks

## 1. Core input model

- [x] 1.1 New input kinds and touch/mouse distinction in `ui.h`
- [x] 1.2 Reach map (Dijkstra over the wrapping grid) and path reconstruction
- [x] 1.3 Act-at-square logic: select, walk, open doors, move-and-attack, target spells
- [x] 1.4 Animated walking, finished early by any input
- [x] 1.5 Action bar (`ui_buttons`), hotkeys, MORE list, end-turn confirmation
- [x] 1.6 Spell book with descriptions, number keys, wheel paging
- [x] 1.7 3D view controls: swipe, wheel, click thirds, NEXT switches the viewed creature
- [x] 1.8 Help overlay, title screen hover

## 2. Presentation

- [x] 2.1 Panel layout with the action bar, hover inspection, action descriptions on the message line
- [x] 2.2 Map overlays: reachable squares, attackable enemies, path, spell targets

## 3. Platform

- [x] 3.1 SDL mouse motion, click/drag disambiguation, wheel, right-click, touch drag, new key map, gamepad buttons

## 4. Tests and docs

- [x] 4.1 Unit tests rewritten for the new UI; core coverage >= 95%
- [x] 4.2 Game scripts for mouse, keyboard and touch play; script commands for the new inputs
- [x] 4.3 README controls table

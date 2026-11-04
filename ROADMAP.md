# Roadmap

Done:
- **Core game:** rules, spells, line of sight, computer opponent, 2D map, 3D view, sound effects; Windows/Linux/Web builds, tests, CI and Pages.
- **Modern controls:** mouse, keyboard, touch and gamepad; movement preview; action bar.

Next, in order:

1. **Save and resume** *(done: `openspec/changes/add-save-and-resume`)*
   - The game saves automatically at the start of every human turn and offers
     CONTINUE on the title screen, so a phone browser that reloads the tab
     loses nothing.
   - An in-game menu lets you leave a game.
   - The campaign below needs this.
2. **Smarter opponent and balance**
   - The computer flies, rides, drinks potions, retreats when wounded, and
     picks its targets.
   - Difficulty levels.
   - Numbers tuned from playtests.
3. **Wizard designer and campaign**
   - Name your wizard and choose its spells within a points budget.
   - Earn experience by escaping, and carry your wizard from one scenario to
     the next.
4. **More scenarios**
   - Two more worlds with their own terrain mixes, monsters and objectives.
   - Choose the scenario on the title screen.
5. **Music and audio settings**
   - SID-style tunes for the title, play and victory.
   - Volume and mute.
6. **Polish**
   - Smooth scrolling and movement animation.
   - Remappable keys and a settings screen.
   - Install as an app and play offline on phones (PWA).

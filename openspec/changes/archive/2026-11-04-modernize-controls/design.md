# Design

## Context

The UI is a pure state machine in `ui.c` driven by abstract inputs, rendered by
`render.c` into the 320x200 indexed framebuffer. The SDL layer translates
devices into those inputs. That split stays. What changes is the input
vocabulary and the interaction model.

## Goals / Non-Goals

**Goals:**
- Direct manipulation with the mouse; all actions also on hotkeys.
- The same flows on touch, made safe with preview-then-confirm.
- Keep the classic screen layout: map window, info panel with bars, message
  line.

**Non-Goals:**
- Changing game rules or costs.
- Remappable keys (later).
- Pixel-smooth scrolling. Scrolling stays square by square.

## Decisions

1. **Richer input vocabulary.** New input kinds: `IN_POINT` (hover),
   `IN_CLICK` (mouse), `IN_TAP` (touch), `IN_DRAG` (pan by screen pixels),
   `IN_WHEEL`, `IN_PREV`, `IN_DIGIT`, and one kind per hotkey. Click and tap
   differ only in that a tap confirms dangerous actions with a second tap. The
   UI remembers whether the last pointer input was touch, to pick help text
   and hotkey hints.

2. **Click/drag disambiguation in the platform layer.** A press starts a
   gesture. Movement beyond 4 game pixels turns it into a drag, which emits
   `IN_DRAG` deltas. Releasing without dragging emits `IN_CLICK`/`IN_TAP`.
   The core accumulates drag deltas and scrolls one square per 16 pixels.

3. **Reach map.** After every change, a Dijkstra-style relaxation over the
   wrapping grid computes the cheapest AP cost and the previous direction for
   each square. It follows the rules: terrain or air cost, visible creatures
   block, closed doors cost 2 extra for creatures with hands, and squares next
   to visible enemies are end points. The UI stores it so that rendering stays
   a const function. Hidden enemies are ignored on purpose: the real move
   then fails, which is how ambushes work.

4. **Walking is animated in the UI**, one square every 4 ticks, through
   `game_move` / `game_use_door`. Any new input completes the walk at once,
   so the game never feels sluggish. A pending attack runs when the walk
   ends.

5. **Action bar from a pure function.** `ui_buttons()` computes the visible
   buttons (rectangle, action, label, hotkey, enabled) from the state. Both
   the renderer and hit testing use it, so they cannot disagree. The layout
   is three rows of two 60x20 buttons at the bottom of the panel: slot 1 for
   CAST or a contextual action, then 3D VIEW, two contextual slots (the last
   becomes MORE when there are more actions), then NEXT and END TURN.

6. **Lists stay for the spell book and MORE.** The menu struct is reused, and
   rows are 14 pixels tall. Spells get a `desc` string shown on hover.

7. **Scripts.** The game-test script engine gets `click`, `clickmap`,
   `hover`, `drag`, `wheel` and `press <hotkey>` commands, and `expect`
   finishes any walk first. Taps keep their touch semantics.

## Risks / Trade-offs

- [More UI state] Reach map, walking and confirm timers add state that tests
  must cover. Mitigation: unit tests per flow, plus a game script for mouse,
  keyboard and touch each.
- [Phone precision] The squares are about 5.6 mm on a typical phone in
  landscape. Mitigation: preview-then-confirm for map actions, and big
  buttons.

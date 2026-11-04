# Spec Delta

## MODIFIED Requirements

### Requirement: Looking and moving
In the 3D view, Left/Right or A/D, the wheel, a click or tap on the left or right third, or a horizontal swipe SHALL turn the view by 45 degrees with a short animation. Up/W or a click or tap in the middle SHALL move the creature one square forward using the normal movement rules. Tab/NEXT SHALL switch to the next own creature's view. Escape, V, right-click or EXIT SHALL return to the map.

#### Scenario: Step forward
- **WHEN** forward is pressed in the 3D view of a creature with enough AP
- **THEN** the creature moves one square in the facing direction and the view follows

#### Scenario: Look through the next creature
- **WHEN** Tab is pressed in the 3D view
- **THEN** the view switches to the next own creature

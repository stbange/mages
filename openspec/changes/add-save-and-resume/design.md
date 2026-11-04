# Design

## Context

All game state lives in the plain-data `Game` struct inside the core. The core
has no file system or browser access; the SDL layer does.

## Goals / Non-Goals

**Goals:**
- Exact, deterministic resume.
- The core stays free of I/O: it turns state into bytes and back.
- One save slot (autosave).

**Non-Goals:**
- Multiple named save slots or cloud sync.
- Compatibility with save files from future versions (they are rejected).

## Decisions

1. **Explicit binary format, not a memory dump.** `save.c` writes each field
   little-endian through small `put_u8/u16/u32/u64` helpers. The layout is:
   header (`"MAGS"`, version, length), body, then an FNV-1a checksum over the
   body. A memory dump would depend on compiler padding and endianness, which
   differ between MSVC, GCC and wasm.
2. **What is stored:** the map (doors and portal change it), RNG state,
   creatures, objects, players (with the `seen` fog memory), turn, current
   player, portal and outcome fields, plus the UI's player counts and seed.
   Both `seen` and `vis` are stored, because recomputing `vis` would reveal
   slightly more than the player had seen. Events are not stored.
3. **The UI requests saves; the platform performs them.** The UI sets
   `save_request` (`SAVE_WRITE` at the start of each human turn, `SAVE_CLEAR`
   at game over) and exposes `ui_save()` to serialise. At start-up the
   platform hands any stored bytes to `ui_offer_save()`, which validates them
   and adds CONTINUE to the title. This keeps every decision testable in the
   core.
4. **Storage:** desktop uses `SDL_GetPrefPath("stbange", "mages")/autosave.sav`.
   The web uses `localStorage["mages.autosave"]` holding base64 text, through
   two `EM_JS` helpers. This is synchronous and simple, and the data is
   about 12 KB.
5. **Game menu** reuses the list panel (`M_LIST`) with RESUME, HELP and
   QUIT TO TITLE. A MENU button takes the HELP button's place when nothing is
   selected.

## Risks / Trade-offs

- [Format changes] Every change to `Game` needs a version bump. Mitigation:
  a round-trip test compares `game_hash` and every field group, and the
  version constant sits next to the writer.
- [Private browsing] localStorage may throw. Mitigation: the JS helpers catch
  errors, and the game simply runs without saves.

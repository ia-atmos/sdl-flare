# TODO: what is pending in the session `sdl-flare`

Kept current by the session: an item closes when it is settled, and one
comes whenever something is left open. Items are `#N` (the standing rule
of the author of 2026-11-18): a number is unique in this session, taken
in order, never reused nor changed; an item from another session is
`session#N` (`atmos-lang#22`). Milestones keep their names (F0 to F9).
Last review: 2026-11-18, through the whole conversation.

## Index

| # | Title | State |
|---|-------|-------|
| #1 | The remotes of the checkouts of the compiler and of this repository | closed: #1a, done (2026-11-21) |
| #2 | The friction of `pin` with module tables (atmos-lang#40) | settled by set :out (#16) |
| #3 | When to relax exactness for expressiveness | decided: #3a (2026-11-21) |
| #4 | Going back on the first migration after the relaxed rules of pin | closed: done in the second migration (#15) |
| #5 | Moved from F5 to F6: game over menu, combat text, transform and block powers | open, work |
| #6 | F6: items, inventory, HUD, menus, pickup, loot of events, what is not drawn | open, work |
| #7 | F7: the alpha demo by script, the benchmark of the port, the documents | open, work |
| #8 | F8 and F9 (the Empyrean Campaign) | open, work |
| #9 | A leak of 132 KB at the end of the game | closed: gone |
| #10 | `await(n, :e)` may replace the task `Frames` | closed: done |
| #11 | Move the platform layer to `ia-atmos/env-sdl` | closed: #11a, merged (2026-11-25) |
| #12 | The three items of the plan settled as recommended | reported, not acknowledged |
| #13 | Scope of F5 by the data of the alpha demo | reported, not acknowledged |
| #14 | Options added to the patch of the original | reported, not acknowledged |
| #15 | The two migrations to the rules of pin | reported, not acknowledged |
| #16 | The experiment of a per-map context (branch `exp-map-context`) | closed: #16c, confirmed (2026-11-21) |
| #17 | Event powers (traps) are queued and never activated | closed: done |
| #18 | Other reports: `until` of the compiler, the corrupted golden, the permission to install | reported, not acknowledged |

## A. Waiting for an answer of the author

#1 (Closed: #1a, authorized by the author on 2026-11-21; both `origin`s
now name `ia-atmos/lang` and `ia-atmos/sdl-flare`.) **The remotes of the checkouts.** The compiler moved to `ia-atmos/lang`;
`git remote set-url origin https://github.com/ia-atmos/lang` in
`../ceu-atmos` was denied by the permission check of the session. The
same for this repository, now `ia-atmos/sdl-flare` (was `fsantanna/flare`,
then `ia-atmos/flare`): its `origin` still names `fsantanna/flare`.
Pushes and pulls go through the redirects; nothing breaks.
- #1a (recommended) the author runs both `git remote set-url`, or allows them.
- #1b leave them: the redirects work.

#3 (Decided: #3a, by the author on 2026-11-21: relax only after F7, with
a tag on the faithful commit.) **When to relax exactness for expressiveness** (the rule of the author
of 2026-11-14: allowed once a port is fully faithful, keeping the intent
of the original). Where Flare stands: the 31 stored runs (walks, a
fight, duels, stands, and a pickup in 13 maps) give the same trace as the
original, draws included, and enemies, hazards, and loot the same pixels;
but the slice is not complete (F6, F7 open), so a difference is still a
defect.
- #3a (recommended) relax only after F7 (the alpha demo played by script,
  identical end to end), with a tag on the faithful commit, one small
  commit per relaxation, and a list of deliberate differences in
  `doc/fidelity.md`; not mixed with #11. Candidates then: the AI timers
  and flags kept as state only for the order of the draws
  (`doc/state.md`), and the hidden draws of the original.
- #3b relax now, piece by piece.

#16 (Closed: #16c, confirmed by the author on 2026-11-21. Settled by the session first: `set :out`
is on `main`, after atmos-pin#16b was approved (ia-atmos/lang a80a77f):
the five places refer to the entity itself, the ids and copies are gone,
the 32 runs are identical. Reversible: the branch `exp-map-context`
stays as the alternative.) **The experiment of a per-map context** (asked by the author through
atmos-pin): everything that lives for one map in one table pinned by
`MapRun`. On the branch `exp-map-context` (dc92020), tests identical,
-32 lines; four of the five ids and copies of #2 disappear; the hero,
outer to every map, sends its powers and its check of events to the map
as requests. Reported to atmos-pin.
- #16a (recommended) merge it into `main` (rebased on the second
  migration, tests checked again).
- #16b keep it as a branch for the study only.
- #16c keep `main` as it is and use `set :out` (ia-atmos/lang 8361f88,
  atmos-pin#1) where a module or the hero must refer to an entity of a
  map: the place returns to nil when the map ends. Smaller than #16a,
  but it keeps the per-map state in module tables. Tried on the branch `exp-set-out` (5a29b27,
  by the session, to be confirmed): it serves one of the five places,
  the hero's enemy under the mouse (set in the block of `MapRun`); it
  does not serve the places set inside module functions (the last enemy
  hit, a hazard's source, a summoner, the dead that drop): there the
  place returns to nil when the function returns, which the traces did
  not show (the focus is not tested by a run where it matters). So #16a
  remains the recommendation. With the alternative of atmos-pin#16 (the place
  ends with the value, branch `pin-out` of ia-atmos/lang, not approved):
  all five places are `set :out` (branch `exp-pin-out`, b74b904), the
  ids and copies go, traces identical, no slowdown; it depends on his
  decision of atmos-pin#16.

## B. Known work, not done

#5 Moved from F5 to F6. Done: the death of the hero, its death penalty
(half of the currency), the game over menu and its Continue, the respawn
at the place of respawn of the map (`respawn-ice_hall`). Left: the menu
drawn, its Save & Exit, the combat text, and the powers of types
transform and block (only the hero's).

#6 F6: items and inventory, the HUD and menus of the slice. Done, each
with a run or a screenshot identical to the original: the loot on the
ground; currency picked up automatically (`pickup-goblin_warrens`); the
loot of events (`chest-ancient_temple`); a stack picked up with a click
(`pickup-averguard_academy`); traps (#17); the equipment (slots,
requirements, auto-equip, the stats with the absorption and bonuses of
the items: `equip-ancient_temple`); the hero in layers with its
equipment; the NPCs (`stand-cave_goblins`); walls faded over the hero;
the animations of effects (`shield-cave_goblins`); the HUD: the action
bar, the bars of life, mana, and experience (with the experience of the
kills), and the cursor, identical in their regions (`HUD=1 REGION=` of
`tools/compare-shot.sh`). Next: the enemy bar, the title of the region,
the minimap, the active effects, the tooltips; the absorption
of shields and the heal effect; the carried inventory (slots, stacks of
max_quantity); a click on the tooltip of a stack, and ACCEPT; the HUD
(action bar, life and mana orbs, the log, combat text), the menus (the
inventory, the character), and the game over menu with its respawn.

#7 F7: the alpha demo played by script, end to end; the benchmark of the
port against `doc/bench.md` (measured so far: about 11.3 s of CPU and
199 MB for the 900 frames of the run `fight`, with no comparison yet);
the documents. For the paper (paper-games#4, through atmos-lang):
memory and CPU of the port against the original on the same scripted
runs; the size in lines of both; and the CPU of the game logic alone,
with the rasterizer and the presentation skipped in both alike (CPU
time of the process, median of 3 runs, ms per frame and the ratio,
beside the whole-frame number), `doc/bench.md` saying what is skipped.

#8 F8 and F9, as in the plan (the Empyrean Campaign: its menus, parallax
layers, fog of war).

#9 Closed: `ATMOS_ASAN=1` reports no leak on the whole runs `fight` (900
frames) and `maps` (400 frames, map changes), with ia-atmos/lang c4e99c4
(cycles of unpinned values broken since 7480111) and the two
migrations (no module keeps an entity).

#10 Closed: the task `Frames` is gone; its one use is `await(10, :frame)`.
The counters of frames of the game are timers of the original, kept.

#11 (Closed: #11a, decided by the author on 2026-11-25; merged into
`main` at 8748120, over env-sdl 79940a9, the 38 runs identical.) Asked
by env-sdl on 2026-11-25 (Flare is its E6): done on the branch
`env-sdl` (f724aae, from 6e79237, over env-sdl 48e1d50): the 38 runs, the
data, and the screenshots identical; what does not fit was sent to
env-sdl (an image converted from C, the mouse of the software path, fonts
with a style). Asked through atmos:
- #11a (recommended) merge it into `main`; a checkout of env-sdl is then
  needed beside this one.
- #11b keep it as a branch until env-sdl settles what does not fit.
The plan was: move the platform part of the layer (window,
images, fonts, sounds, input, the test mode of `engine/flare.h` and
`engine/engine.atm`) to `ia-atmos/env-sdl` (Flare third, after Pingus and
SNKRX), with the golden outputs identical; the game part (`astar.h`,
`collision.h`, `map.h`, `geom.h`) stays. The scripted input will need a
converter to the form of the environment. Meanwhile, new platform code is
kept apart from game code.

#17 Closed: the power of an event is activated at once from its StatBlock,
when its cooldown has ended (`MapRenderer::activatePower`); the run
`trap-ice_hall` (a booby-trapped chest) matches the original. On the way:
a source of perfect accuracy no longer draws for overhit and miss, and
`requires_item` is checked (against the currency, until the inventory).

## C. Reported or decided, not acknowledged

#2 The friction of `pin` with module tables (atmos-lang#40): the rules of
2026-11-17 removed the holder tables; the five entities kept by modules
as ids or copies remain on `main` (#16 removes four).

#4 Closed: the holder tables of the first migration were removed in the
second (#15).

#12 The three items of the plan marked to confirm (slice, data read as is,
fidelity by trace) were settled as the plan recommended, after "Faça tudo
o que for preciso. Vá em frente" (`doc/decisions.md`).

#13 Scope of F5 by the data of the alpha demo: its enemies use only melee
and ranged AI powers (no on_hit, half_dead, on_death, debuff), and none
can flee; those parts are not ported. F5 was closed with drawing (loot,
hero's layers, NPCs, effect animations), the game over menu, and the
combat text moved to F6.

#14 The patch of the original gained `FLARE_DUMPE`, `FLARE_NOHUD`,
`FLARE_NOHERO`, and the trace line `pickup` (trace and screenshots only;
no change of behavior).

#15 The two migrations to the rules of pin (2026-11-16 and 2026-11-17) are
done, with identical traces, pixels, and dumps; the reports went to the
compiler session (now atmos-lang; pin is now atmos-pin). Two defects of
the compiler found and fixed on the way (`0ba7afc`, the crash of a boxed
`pin var`).

#18 Other reports: the values of `until <cnd>, <vals>` are evaluated with
the condition, unlike the reading of the manual (`doc/compiler.md`); the
golden output of the runs was committed corrupted once (two writers) and
restored in `fe3e438`, and the suite and its update now run one after the
other; the permission to install packages (SDL2 development files,
`time`) was asked and given at the start.

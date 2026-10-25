# Pocket Pro Tracker PT2 v0.2 experimental

A test fork of [ProTracker 2 Clone v1.92](https://github.com/8bitbubsy/pt2-clone) by 8bitbubsy.
The PT2 GUI, original sample functions, playback and .MOD loader/saver belong to upstream.
PPT adds a hardware-specific input layer for one RG35XX Pro with muOS.

**No stable ARM64 build is bundled in the source kit.** Generate it using the included GitHub Actions workflow.

Controls in this alpha:
- D-pad: pattern navigation with repeat (280 ms / 85 ms)
- A: insert last note/sample or effect/parameter at cursor
- A + direction: edit active NOTE, SAMPLE, FX or PARAM
- B: copy current field; B then A: cut current field; SELECT + A: paste current field
- R2: virtual left mouse click; L2: virtual right mouse click
- L1 + START: leave program (without autosaving! save MOD first)
- Right analog: *experimental* SDL cursor, currently uses ABS_RX/ABS_RY; calibration unverified

**NOT YET IMPLEMENTED:** LSDJ-style multi-row/block selection, full keyboard shortcuts, exact decimal row labels, note names beyond original PT2 C-1..B-3, verified physical joystick axes, full audio/video device testing.

Preserves original ProTracker GUI, modulation and MOD file format (31-sample / 4-channel model).
License: BSD-3-Clause, see upstream LICENSE, attribution in source kit.

# Game controls

Use a plain text file with the same filename stem as the ROM:

- `roms/Pong.ch8` uses `controls/Pong.txt`.
- `roms/tetris.rom` uses `controls/tetris.txt`.
- For a new `roms/Game.ch8`, create `controls/Game.txt`.

Write physical keyboard keys and their actions, one per line. No special syntax
or commands are needed. These files are display text; they do not remap keys or
execute instructions. Use the same filename capitalization as the ROM.

The Debug panel shows the first six non-empty, non-comment lines. Keep each line
within 35 characters for the large pixel font; longer lines display an ellipsis.
Use simple ASCII characters, such as `Q - ROTATE PIECE`. Blank lines and lines
starting with `#` are ignored, so source notes can stay in the same file.

Controls load with the ROM. After editing a text file, restart the app or press
F5. F5 also resets the game, preserving the current pause and display mode.
Missing or empty controls files show a helpful message and do not stop the ROM.

The loader first looks in the `controls` folder beside the ROM's containing
folder (normally the project root beside `roms`), then in `controls` under the
current working directory. This also works when launching from `build` with
`../roms/Pong.ch8` or using an absolute ROM path.

## Verification notes

The bundled descriptions were matched to the local ROM SHA-1 hashes using the
[CHIP-8 database](https://raw.githubusercontent.com/chip-8/chip-8-database/master/database/programs.json)
and checked against each ROM's key-test instructions. The old author notes use
calculator keyboard labels; these files use this project's physical keyboard.
Pong is the two-player Paul Vervalin version. Tetris uses hold-to-drop-faster,
not an instant hard drop. Test the games normally and edit the descriptions if
you replace either ROM with another version.

## Downloaded games

The retained netpro2k collection has matching controls descriptions for every ROM.
Existing Pong.txt and tetris.txt were preserved. On Windows, both tetris.rom and
the identical Tetris.ch8 use tetris.txt (case-insensitive filenames).

See [the ROM inventory](../roms/README.md) for sources and compatibility checks.
Each new text file records its ROM hash and evidence in # comments. X-mirror was
removed because it immediately hits an unsupported opcode. The remaining ROMs
passed startup and sampled-input smoke checks, but have not been fully playtested.
Picture/logo/maze demos explain that they do not need game keys.

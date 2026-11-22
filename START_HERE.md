# Atomic-AGD-Suite: start here

Kees van Oss's (oss003) suite to convert, compile and play AGD (Arcade Game Designer) games, with KC's 2018 BBC Micro port added as `BBC Suite`: ZX Spectrum AGD games compiled to 6502 (CC65's ca65/ld65) and put on BBC discs. Tool/port, unfinished.

| | |
|---|---|
| GitHub | https://github.com/kieranhj/Atomic-AGD-Suite (public; not a GitHub fork) |
| Upstream | `oss003` remote = https://github.com/oss003/Atomic-AGD-Suite (43 of the 76 commits on this branch are oss003's) |
| Local path | D:\beeb\mine\Atomic-AGD-Suite (moved from OneDrive\BEEB\Repos\Atomic-AGD-Suite in the 2026-11 tidy) |
| Branch | mode1-colour (checked out); others: master, compiler-investigation (2 commits unpushed), shift-tables, sprite-plotting, update-from-kees, weird-bug, wip/2026-11-tidy |
| Last commit | 2018-08-16: SWRAM implementation |

## What's in here
- `BBC Suite\`: KC's BBC port. `Scripts\*.agd` (games), `AGD\AGD.exe` (the compiler, from Atomic-AGD-Script-Compiler), `CC65\` (ca65/ld65, `game.asm` engine, `MAKE.BAT`), `Output\*.bin`, `Discs\*.ssd`, `notes.txt` (design notes), the `.bat` scripts.
- `Atom Suite\`, `ZX Suite\`, `CPC Suite\`, `CPC Suite Patch\`: Kees's suites for the other machines.
- `Snapshots\`: ZX Spectrum `.sna` files the converter reads (125 files).

## Build and run
- `make`: prints how to build (info-only Makefile added 2026-11-22). Nothing is built by plain make because a build rewrites tracked files that already have uncommitted changes.
- By hand, in `BBC Suite`: `.\build.bat GAME [m p s i d a]` compiles `Scripts\GAME.agd` and assembles it to `Output\GAME.bin`; `.\buildall.bat` does every script with `p`; `.\run.bat GAME` makes `Discs\GAME.ssd` with beebasm (`..\..\..\bin\beebasm.exe`, which resolves to D:\beeb\Bin) **and launches b-em** (`..\..\..\bin\b-em\b-em.exe`).
- Watch out: `build.bat` runs `call make %1 ...` inside `CC65\`, relying on cmd finding `CC65\MAKE.BAT` in the current folder. If cmd is set not to search the current folder (`NoDefaultCurrentDirectoryInExePath`), that runs GNU make instead.

## Local changes
- Large uncommitted state: 82 modified tracked files (66 in `BBC Suite\Output`, 8 disc images, CC65 engine files incl. `game.asm`, `AGD\AGD.exe`, `notes.txt`) and 157 untracked (`CC65\*.inc` per game, more `Output\*.bin`, `Scripts\basicdig.agd`, top-level `allhallows.agd`).
- All of it is saved on branch `wip/2026-11-tidy` (pushed; 122 files), the snapshot taken before the move. The working tree was left as it was.

## Status and next steps
- Left in August 2018: MODE 1 sprite plotting and SWRAM on `mode1-colour`. `BBC Suite\notes.txt` has the design notes and ideas.
- Decide which branches still matter (`weird-bug`, `sprite-plotting`, `shift-tables`, `update-from-kees` look like merged or abandoned side branches).

## See also
- README.md, `BBC Suite\notes.txt`. D:\beeb\mine\Atomic-AGD-Script-Compiler.

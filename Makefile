# Wrapper so that `make` says how to build from PowerShell / Windows Terminal.
# GNU make runs recipes with cmd.exe here (no sh on PATH).
# The suite builds one AGD game at a time from batch files in "BBC Suite";
# a build rewrites tracked files (CC65\game.inc, game.lst, Output\*.bin), so
# nothing here runs it for you.
.PHONY: all help
all: help

help:
	@echo Nothing is built by plain make here: the AGD suite builds one game at a time.
	@echo   cd "BBC Suite"
	@echo   .\build.bat GAME [m p s i d a]   - compile Scripts\GAME.agd, assemble with CC65\ca65, writes Output\GAME.bin
	@echo   .\buildall.bat                   - build every Scripts\*.agd with p
	@echo   .\run.bat GAME                   - make Discs\GAME.ssd with beebasm AND launch b-em
	@echo See START_HERE.md and README.md

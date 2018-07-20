echo PUTFILE "cc65\%1.atm", "AGDGAME", $1100, $1100 > disc.asm
..\..\..\bin\beebasm.exe -i disc.asm -do %1.ssd -boot AGDGAME
..\..\..\bin\b-em\b-em.exe %1.ssd
del disc.asm

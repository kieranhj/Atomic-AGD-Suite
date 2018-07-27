@echo off

rem Covert ZX snapshot to AGD file
 copy ..\snapshots\%1.sna convert
 if errorlevel 1 goto error
 cd convert
 convert %1
 echo EVENT COLLECTBLOCK >> %1.agd
 copy %1.agd ..\scripts
 del %1.sna
 del %1.agd
 cd ..
 goto end

:error
 echo %1.agd not found .....

:end

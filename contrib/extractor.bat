@ECHO OFF

:MENU
ECHO.
ECHO -------------------------------------------------
ECHO Trinitycore dbc/db2, maps, vmaps, mmaps extractor
ECHO -------------------------------------------------
ECHO   1 - Extract dbc/db2 and maps
ECHO   2 - Extract vmaps (needs maps to be extracted before you run this)
ECHO   3 - Extract mmaps (needs vmaps to be extracted before you run this, may take hours)
ECHO   4 - Extract all (may take hours)
ECHO   5 - Exit
ECHO -------------------------------------------------
SET /P M="Type 1, 2, 3, 4 or 5 then press ENTER: "
IF %M%==1 GOTO MAPS
IF %M%==2 GOTO VMAPS
IF %M%==3 GOTO MMAPS
IF %M%==4 GOTO ALL
IF %M%==5 GOTO EOF

REM Invalid option
ECHO Invalid option.
GOTO MENU

:MAPS
ECHO Extracting dbc/db2 and maps ...
.\mapextractor.exe
ECHO Finished.
GOTO MENU

:VMAPS
ECHO Extracting vmaps ...
.\vmap4extractor.exe
md vmaps
.\vmap4assembler.exe Buildings vmaps
rmdir Buildings /s /q
ECHO Finished.
GOTO MENU

:MMAPS
ECHO Extracting mmaps ...
md mmaps
.\mmaps_generator.exe
ECHO Finished.
GOTO MENU

:ALL
ECHO Extracting all ...
.\mapextractor.exe
.\vmap4extractor.exe
md vmaps
.\vmap4assembler.exe Buildings vmaps
rmdir Buildings /s /q
md mmaps
.\mmaps_generator.exe
ECHO Finished.
GOTO MENU

:EOF

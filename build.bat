@echo off
rem ---------------------------------------------------------------
rem  Builds tetris.exe (game) and tetris_server.exe (REST server)
rem  with MSYS2 / MinGW. Run after every code change.
rem ---------------------------------------------------------------
setlocal
cd /d "%~dp0"

where g++ >nul 2>nul
if not errorlevel 1 goto have_gpp
if exist "C:\msys64\ucrt64\bin\g++.exe" set "PATH=C:\msys64\ucrt64\bin;%PATH%"
where g++ >nul 2>nul
if errorlevel 1 goto no_gpp

:have_gpp
echo [1/3] Building the game: tetris.exe
g++ -std=c++17 -O2 -Wall -Wextra src/Terminal.cpp src/ConsoleRenderer.cpp src/States.cpp src/Game.cpp src/HttpScoreService.cpp src/main.cpp -static -o tetris.exe -lws2_32
if errorlevel 1 goto game_fail

rem SQLite is one big C file. It takes about 30 seconds, so it is compiled only once.
if not exist build mkdir build
if exist build\sqlite3.o goto sqlite_done
echo [2/3] Compiling SQLite (first time only, about 30 seconds)...
gcc -O2 -c -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION server/third_party/sqlite3.c -o build/sqlite3.o
if errorlevel 1 goto sqlite_fail
:sqlite_done

echo [3/3] Building the server: tetris_server.exe
g++ -std=c++17 -O2 -Wall -Wextra server/Database.cpp server/ApiServer.cpp server/main.cpp build/sqlite3.o -static -o tetris_server.exe -lws2_32
if errorlevel 1 goto server_fail

echo.
echo Build finished: tetris.exe and tetris_server.exe
pause
exit /b 0

:no_gpp
echo g++ was not found. Install MSYS2 and its UCRT64 toolchain, or add
echo C:\msys64\ucrt64\bin to your PATH, then run this file again.
pause
exit /b 1

:game_fail
echo.
echo The game did not compile. See the errors above.
pause
exit /b 1

:sqlite_fail
echo.
echo SQLite did not compile. Check that gcc is installed (MSYS2: pacman -S mingw-w64-ucrt-x86_64-gcc).
pause
exit /b 1

:server_fail
echo.
echo The server did not compile. See the errors above.
pause
exit /b 1

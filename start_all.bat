@echo off
rem ---------------------------------------------------------------
rem  Starts the leaderboard server in its own window, then the game.
rem  The API key is typed here and kept only in this session's
rem  environment variable. It is never written to a file.
rem  Scores are stored in the file tetris.db next to this script.
rem ---------------------------------------------------------------
setlocal
cd /d "%~dp0"

if not exist tetris.exe goto needbuild
if not exist tetris_server.exe goto needbuild

rem Skipped when the variable is already set in this terminal.
if "%API_KEY%"=="" set /p API_KEY=Choose an API key (any secret text, no spaces): 

rem The game must use the same key as the server.
set "SCORE_API_KEY=%API_KEY%"

echo Starting the leaderboard server in a new window...
start "Tetris leaderboard server" cmd /k tetris_server.exe

rem Give the server a moment to start listening.
ping -n 3 127.0.0.1 >nul

tetris.exe

echo.
echo Game closed. Stopping the leaderboard server...
taskkill /im tetris_server.exe /f >nul 2>nul
echo You can close the server window.
pause
exit /b 0

:needbuild
echo tetris.exe or tetris_server.exe is missing. Run build.bat first.
pause
exit /b 1

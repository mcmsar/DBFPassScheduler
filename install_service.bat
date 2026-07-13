@echo off
:: Run this script as Administrator on the remote machine

set EXE_PATH=C:\Program Files (x86)\EMS Technologies\LUT Server\Run\x64\TSIDBFPassScheduler.exe

echo Creating TSIDBFPassScheduler service...
sc create TSIDBFPassScheduler ^
    binPath= "%EXE_PATH%" ^
    DisplayName= "TSI DBF Pass Scheduler Service" ^
    start= auto ^
    obj= LocalSystem

if %ERRORLEVEL% EQU 0 (
    echo Service created successfully.
) else if %ERRORLEVEL% EQU 1073 (
    echo Service already exists - updating path...
    sc config TSIDBFPassScheduler binPath= "%EXE_PATH%"
) else (
    echo ERROR: sc create failed with code %ERRORLEVEL%
)

echo.
echo Importing x64 registry entries...
regedit /s "%~dp0emsdbfpasssched_x64.reg"
echo Done. Verify with: sc query TSIDBFPassScheduler
pause

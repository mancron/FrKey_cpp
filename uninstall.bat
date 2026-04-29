@echo off
chcp 65001 > nul

:: ── 관리자 권한 자동 상승 ──────────────────────────────────────
net session > nul 2>&1
if %errorlevel% neq 0 (
    echo 관리자 권한이 필요합니다. 권한 상승 중...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

:: ── 제거 ───────────────────────────────────────────────────────
echo.
echo  FrKey IME 제거 중...
echo.

:: 설치 시 복사된 고정 경로에서 제거
set "INSTALLED=%ProgramData%\FrKey\FrKey.dll"

if not exist "%INSTALLED%" (
    echo [오류] 설치된 FrKey.dll을 찾을 수 없습니다.
    echo        이미 제거되었거나 설치된 적이 없습니다.
    pause
    exit /b 1
)

regsvr32 /s /u "%INSTALLED%"
if %errorlevel% neq 0 (
    echo [오류] 제거 실패 ^(오류 코드: %errorlevel%^)
    pause
    exit /b 1
)

echo [완료] FrKey IME가 제거되었습니다.
echo.
pause

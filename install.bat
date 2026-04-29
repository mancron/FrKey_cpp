@echo off
chcp 65001 > nul

:: ── 관리자 권한 자동 상승 ──────────────────────────────────────
net session > nul 2>&1
if %errorlevel% neq 0 (
    echo 관리자 권한이 필요합니다. 권한 상승 중...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

:: ── 설치 ───────────────────────────────────────────────────────
echo.
echo  FrKey IME 설치 중...
echo.

set "DLL=%~dp0FrKey.dll"

if not exist "%DLL%" (
    echo [오류] FrKey.dll을 찾을 수 없습니다.
    echo        install.bat과 FrKey.dll을 같은 폴더에 놓으세요.
    pause
    exit /b 1
)

regsvr32 /s "%DLL%"
if %errorlevel% neq 0 (
    echo [오류] 등록 실패 ^(오류 코드: %errorlevel%^)
    pause
    exit /b 1
)

echo [완료] FrKey IME가 설치되었습니다.
echo.
echo  사용 방법:
echo   - 언어 표시줄에서 'French Accent IME' 선택
echo   - 영문자 입력 후 한자 키를 누르면 악센트 팔레트 표시
echo   - 숫자 키로 악센트 선택, ESC로 취소
echo.
pause

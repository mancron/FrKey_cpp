@echo off
chcp 65001 > nul

:: ── 관리자 권한 자동 상승 ──────────────────────────────────────

net session > nul 2>&1

if %errorlevel% neq 0 (
    echo 관리자 권한이 필요합니다. 권한 상승 중...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

:: 경로 변수 설정
set "INSTALLED_DIR=%ProgramData%\FrKey"
set "INSTALLED_FILE=%INSTALLED_DIR%\FrKey.dll"

:: ── 2단계: 재부팅 후 자동 실행되는 삭제 로직 ───────────────────
if "%~1"=="/phase2" (
    echo.
    echo [재부팅 후 2단계 작업] FrKey IME 잔여 파일 삭제 중...
    
    del /f /q "%INSTALLED_FILE%" > nul 2>&1
    rmdir /q "%INSTALLED_DIR%" > nul 2>&1
    
    echo [완료] FrKey IME 잔여 폴더와 파일이 완전히 제거되었습니다.
    pause
    exit /b
)

:: ── 1단계: 초기 실행 로직 (등록 해제 및 삭제 시도) ───────────────
echo.
echo FrKey IME 1단계 제거 중...
echo.

if not exist "%INSTALLED_FILE%" (
    echo [안내] 설치된 FrKey.dll을 찾을 수 없습니다. 이미 제거되었거나 설치된 적이 없습니다.
    pause
    exit /b 0
)

:: 1. 시스템 레지스트리에서 DLL 등록 해제
regsvr32 /s /u "%INSTALLED_FILE%"

:: 2. 1차 파일 삭제 시도 (사용 중이 아닐 경우 즉시 삭제됨)
del /f /q "%INSTALLED_FILE%" > nul 2>&1

:: 3. 삭제 여부 확인 및 RunOnce 등록
if exist "%INSTALLED_FILE%" (
    echo [안내] 파일이 현재 시스템에서 사용 중입니다.
    echo 재부팅 직후 스크립트가 자동 실행되어 삭제를 마무리하도록 예약합니다.
    
    :: 윈도우 RunOnce 키에 현재 스크립트를 /phase2 인자와 함께 등록
    reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\RunOnce" /v "FrKeyUninstall" /t REG_SZ /d "\"%~f0\" /phase2" /f > nul
    
    echo.
    echo [조치 필요] PC를 다시 시작해 주세요. 재부팅 완료 후 자동으로 삭제가 진행됩니다.
) else (
    :: 파일이 즉시 삭제되었다면 빈 폴더도 정리
    rmdir /q "%INSTALLED_DIR%" > nul 2>&1
    echo [완료] FrKey IME가 즉시 완전히 제거되었습니다.
)

echo.
pause

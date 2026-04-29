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

set "INSTALLED_DIR=%ProgramData%\FrKey"
set "INSTALLED_FILE=%INSTALLED_DIR%\FrKey.dll"

if not exist "%INSTALLED_FILE%" (
    echo [안내] 설치된 FrKey.dll을 찾을 수 없습니다. 이미 제거되었거나 설치된 적이 없습니다.
    pause
    exit /b 0
)

:: 1. 시스템 레지스트리에서 DLL 등록 해제
regsvr32 /s /u "%INSTALLED_FILE%"

:: 2. 1차 파일 삭제 시도 (사용 중이 아닐 경우 즉시 삭제됨)
del /f /q "%INSTALLED_FILE%" > nul 2>&1

:: 3. 삭제 여부 확인 및 재부팅 시 삭제 예약 로직
if exist "%INSTALLED_FILE%" (
    echo [안내] 파일이 현재 사용 중입니다. 재부팅 시 삭제되도록 예약합니다.
    
    :: PowerShell을 통해 Win32 API MoveFileEx (MOVEFILE_DELAY_UNTIL_REBOOT = 4) 호출
    powershell -Command "Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class Win32 { [DllImport(\"kernel32.dll\", CharSet=CharSet.Unicode)] public static extern bool MoveFileEx(string lpExistingFileName, string lpNewFileName, int dwFlags); }'; [Win32]::MoveFileEx('%INSTALLED_FILE%', $null, 4)"
    
    echo [완료] 다음 PC 재부팅 시 FrKey IME가 시스템에서 완전히 제거됩니다.
) else (
    :: 파일이 즉시 삭제되었다면 빈 폴더도 정리
    rmdir /q "%INSTALLED_DIR%" > nul 2>&1
    echo [완료] FrKey IME가 즉시 완전히 제거되었습니다.
)

echo.
pause

#include <windows.h>
#include "PaletteWindow.h"
// 시스템 전역에서 사용할 현재 DLL의 인스턴스 핸들
// (나중에 UI 창을 띄우거나 아이콘을 불러올 때 반드시 필요합니다)
HINSTANCE g_hInst = nullptr;

/// <summary>
/// DLL이 프로세스(메모장, 크롬 등)에 로드되거나 언로드될 때 최초로 실행되는 진입점입니다.
/// </summary>
BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // DLL이 로드될 때 핸들 저장
        g_hInst = hInstance;

        // 스레드 생성 시마다 호출되는 것을 막아 성능을 최적화합니다.
        DisableThreadLibraryCalls(hInstance);
        CPaletteWindow::Initialize(hInstance);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        // DLL이 메모리에서 내려갈 때 필요한 정리 작업을 수행합니다.
        CPaletteWindow::Uninitialize();
        break;
    }
    return TRUE; // 초기화 성공
}
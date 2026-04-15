#pragma once
#include <windows.h>
#include <string>

class CPaletteWindow
{
public:
    static bool Initialize(HINSTANCE hInstance);
    static void Uninitialize();

    CPaletteWindow();
    ~CPaletteWindow();

    // 캐럿 좌표(x, y)에 창을 띄우고 표시할 텍스트 설정
    void Show(int x, int y, const std::wstring& options);
    void Hide();
    bool IsVisible() const;

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    HWND _hWnd;
    std::wstring _displayOptions;
};
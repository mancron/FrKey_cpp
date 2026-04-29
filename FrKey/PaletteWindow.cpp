#include "PaletteWindow.h"

const wchar_t* WINDOW_CLASS_NAME = L"FrKeyPaletteClass";
HINSTANCE g_hInstPalette = nullptr;

bool CPaletteWindow::Initialize(HINSTANCE hInstance)
{
    g_hInstPalette = hInstance;
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW; // �׸��� ȿ��
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    return RegisterClassExW(&wcex) != 0;
}

void CPaletteWindow::Uninitialize()
{
    UnregisterClassW(WINDOW_CLASS_NAME, g_hInstPalette);
}

CPaletteWindow::CPaletteWindow() : _hWnd(nullptr) {}

CPaletteWindow::~CPaletteWindow() { Hide(); }

void CPaletteWindow::Show(int x, int y, const std::wstring& options)
{
    _displayOptions = options;

    // 텍스트 폭을 측정해 창 크기를 동적으로 결정합니다.
    int winW = 150, winH = 40;
    {
        HDC hdcScreen = GetDC(nullptr);
        if (hdcScreen)
        {
            SIZE sz = {};
            if (GetTextExtentPoint32W(hdcScreen, options.c_str(), (int)options.length(), &sz))
            {
                winW = sz.cx + 16;  // 좌우 여백
                winH = sz.cy + 16;  // 상하 여백
            }
            ReleaseDC(nullptr, hdcScreen);
        }
    }

    if (!_hWnd)
    {
        _hWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            WINDOW_CLASS_NAME, nullptr,
            WS_POPUP | WS_BORDER,
            x, y + 20, winW, winH,
            nullptr, nullptr, g_hInstPalette, this);
    }

    SetWindowPos(_hWnd, HWND_TOPMOST, x, y + 20, winW, winH, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    InvalidateRect(_hWnd, nullptr, TRUE);
}

void CPaletteWindow::Hide()
{
    if (_hWnd) DestroyWindow(_hWnd);
    _hWnd = nullptr;
}

bool CPaletteWindow::IsVisible() const { return _hWnd != nullptr; }

LRESULT CALLBACK CPaletteWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // �� �߰�: â�� ������ �� ��ü �����͸� â�� �޸𸮿� �����մϴ�.
    if (message == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCreate->lpCreateParams);
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    if (message == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // �����ص� �����͸� ������ ���ڸ� ����մϴ�.
        CPaletteWindow* pThis = (CPaletteWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (pThis && !pThis->_displayOptions.empty())
        {
            // �� �߰�: ����� �����ϰ�, ���ڻ��� ���������� ���� ����
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));

            TextOutW(hdc, 5, 10, pThis->_displayOptions.c_str(), pThis->_displayOptions.length());
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
#include "PaletteWindow.h"

const wchar_t* WINDOW_CLASS_NAME = L"FrKeyPaletteClass";
HINSTANCE g_hInstPalette = nullptr;

bool CPaletteWindow::Initialize(HINSTANCE hInstance)
{
    g_hInstPalette = hInstance;
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW; // 그림자 효과
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

    if (!_hWnd)
    {
        // ★ 팩트 체크: WS_EX_NOACTIVATE 플래그가 없으면 포커스를 뺏겨 TSF 세션이 붕괴됩니다.
        _hWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            WINDOW_CLASS_NAME, nullptr,
            WS_POPUP | WS_BORDER,
            x, y + 20, 150, 40, // 폰트 크기 고려한 대략적인 크기
            nullptr, nullptr, g_hInstPalette, this);
    }

    // SW_SHOWNOACTIVATE로 띄워야 포커스 유지가 보장됨
    SetWindowPos(_hWnd, HWND_TOPMOST, x, y + 20, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
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
    // ★ 추가: 창이 생성될 때 객체 포인터를 창의 메모리에 저장합니다.
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

        // 저장해둔 포인터를 가져와 글자를 출력합니다.
        CPaletteWindow* pThis = (CPaletteWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (pThis && !pThis->_displayOptions.empty())
        {
            // ★ 추가: 배경을 투명하게, 글자색을 검은색으로 강제 지정
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));

            TextOutW(hdc, 5, 10, pThis->_displayOptions.c_str(), pThis->_displayOptions.length());
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
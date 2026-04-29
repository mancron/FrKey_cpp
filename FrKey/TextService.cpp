#include "TextService.h"
#include "Globals.h"
#include "EditSession.h"
#include "ReadSession.h"


CTextService::CTextService() : _cRef(1), _pThreadMgr(nullptr), _tid(TF_CLIENTID_NULL), _lastChar(0)
{
    InterlockedIncrement(&g_cRefDll);
}

CTextService::~CTextService()
{
    InterlockedDecrement(&g_cRefDll);
}

STDMETHODIMP CTextService::QueryInterface(REFIID riid, void** ppvObj)
{
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor))
        *ppvObj = (ITfTextInputProcessor*)this;
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
        *ppvObj = (ITfKeyEventSink*)this; // 키 이벤트 싱크 인터페이스 노출

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CTextService::AddRef() { return InterlockedIncrement(&_cRef); }

STDMETHODIMP_(ULONG) CTextService::Release()
{
    LONG cr = InterlockedDecrement(&_cRef);
    if (cr == 0) delete this;
    return cr;
}

// 입력기가 선택되었을 때 OS가 호출
STDMETHODIMP CTextService::Activate(ITfThreadMgr* ptim, TfClientId tid)
{
    _pThreadMgr = ptim;
    _pThreadMgr->AddRef();
    _tid = tid;

    // 키보드 가로채기 기능 등록
    if (!_InitKeyEventSink())
        return E_FAIL;

    return S_OK;
}

// 입력기가 해제될 때 OS가 호출
STDMETHODIMP CTextService::Deactivate()
{
    // 키보드 가로채기 기능 해제
    _UninitKeyEventSink();

    if (_pThreadMgr)
    {
        _pThreadMgr->Release();
        _pThreadMgr = nullptr;
    }
    _tid = TF_CLIENTID_NULL;

    return S_OK;
}


// ──────────────────────────────────────
// 키보드 이벤트 싱크 등록 / 해제 로직
// ──────────────────────────────────────
BOOL CTextService::_InitKeyEventSink()
{
    ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
    HRESULT hr = _pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr);

    if (SUCCEEDED(hr))
    {
        // 시스템에 이 객체를 키보드 이벤트 수신기로 등록
        hr = pKeystrokeMgr->AdviseKeyEventSink(_tid, (ITfKeyEventSink*)this, TRUE);

        // 텐키 . 키를 PreserveKey로 등록
        // OnKeyDown 경로는 한국어 입력 프레임워크에 가로채여 VK_DECIMAL이 도달 안 할 수 있으므로
        // PreserveKey를 쓰면 TSF가 OnPreservedKey를 통해 확실하게 전달해 줌
        if (SUCCEEDED(hr))
        {
            TF_PRESERVEDKEY tfKey = { VK_DECIMAL, TF_MOD_IGNORE_ALL_MODIFIER };
            const WCHAR szDesc[] = L"IME Switch (Numpad .)";
            pKeystrokeMgr->PreserveKey(_tid, GUID_KEY_DECIMAL_SWITCH,
                                       &tfKey, szDesc, (ULONG)wcslen(szDesc));
        }

        pKeystrokeMgr->Release();
    }
    return SUCCEEDED(hr);
}

void CTextService::_UninitKeyEventSink()
{
    if (!_pThreadMgr) return;

    ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
    HRESULT hr = _pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr);

    if (SUCCEEDED(hr))
    {
        TF_PRESERVEDKEY tfKey = { VK_DECIMAL, TF_MOD_IGNORE_ALL_MODIFIER };
        pKeystrokeMgr->UnpreserveKey(GUID_KEY_DECIMAL_SWITCH, &tfKey);
        pKeystrokeMgr->UnadviseKeyEventSink(_tid);
        pKeystrokeMgr->Release();
    }
}

// ──────────────────────────────────────
// 실제 키보드 이벤트 처리부 (핵심)
// ──────────────────────────────────────
STDMETHODIMP CTextService::OnSetFocus(BOOL fForeground) { return S_OK; }

STDMETHODIMP CTextService::OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    *pfEaten = FALSE;


    if (wParam == 0x15)
    {
        *pfEaten = TRUE;
        return S_OK;
    }

    if (_palette.IsVisible())
    {
        if ((wParam >= '1' && wParam <= '9') || wParam == VK_ESCAPE)
            *pfEaten = TRUE;
    }

    return S_OK;
}

STDMETHODIMP CTextService::OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    *pfEaten = FALSE;

    // ── 0. 한/영 키(VK_HANGUL) 감지 → Win+Space 스푸핑으로 IME 전환 ──
    // 텐키 . 키는 PreserveKey(OnPreservedKey)에서 악센트 팔레트로 처리
    if (wParam == 0x15)
    {
        *pfEaten = TRUE;

        struct AsyncSend {
            static VOID CALLBACK Proc(HWND, UINT, UINT_PTR id, DWORD) {
                KillTimer(nullptr, id);
                INPUT ins[4] = {};
                ins[0].type = INPUT_KEYBOARD; ins[0].ki.wVk = VK_LWIN;
                ins[1].type = INPUT_KEYBOARD; ins[1].ki.wVk = VK_SPACE;
                ins[2].type = INPUT_KEYBOARD; ins[2].ki.wVk = VK_SPACE;  ins[2].ki.dwFlags = KEYEVENTF_KEYUP;
                ins[3].type = INPUT_KEYBOARD; ins[3].ki.wVk = VK_LWIN;   ins[3].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(4, ins, sizeof(INPUT));
            }
        };
        SetTimer(nullptr, 0, 0, AsyncSend::Proc);

        return S_OK;
    }

    // ── 1. 팔레트가 열려있을 때 숫자 키(1~9) 선택 처리 ──
    if (_palette.IsVisible())
    {
        if (wParam >= '1' && wParam <= '9')
        {
            int index = wParam - '1'; // '1'을 누르면 배열의 0번째 인덱스 접근

            // 유효한 숫자 범위인지 확인
            if (index >= 0 && index < _currentAccents.length())
            {
                WCHAR chAccent = _currentAccents[index]; // 선택된 악센트 문자

                _palette.Hide();

                CEditSession* pEditSession = new CEditSession(pic, chAccent);
                HRESULT hrSession;
                pic->RequestEditSession(_tid, pEditSession, TF_ES_READWRITE | TF_ES_SYNC, &hrSession);
                pEditSession->Release();

                *pfEaten = TRUE;
                return S_OK;
            }
        }
        else if (wParam == VK_ESCAPE)
        {
            _palette.Hide();
            *pfEaten = TRUE;
            return S_OK;
        }

        // 다른 키 누르면 창 닫기
        _palette.Hide();
    }

    // ── 2. 트리거 키(한자 키: 0x19) 분기 처리 ──
    if (wParam == 0x19)
    {
        std::wstring options;

        // 방금 입력한 문자에 따라 악센트 후보와 UI 텍스트 세팅
        switch (_lastChar)
        {
        case L'a': _currentAccents = L"àâäæ"; options = L"1:à 2:â 3:ä 4:æ"; break;
        case L'e': _currentAccents = L"éèêëœ"; options = L"1:é 2:è 3:ê 4:ë 5:œ"; break;
        case L'i': _currentAccents = L"îï"; options = L"1:î 2:ï"; break;
        case L'o': _currentAccents = L"ôöœ"; options = L"1:ô 2:ö 3:œ"; break;
        case L'u': _currentAccents = L"ùûü"; options = L"1:ù 2:û 3:ü"; break;
        case L'c': _currentAccents = L"ç"; options = L"1:ç"; break;
        case L'A': _currentAccents = L"ÀÂÄÆ"; options = L"1:À 2:Â 3:Ä 4:Æ"; break;
        case L'E': _currentAccents = L"ÉÈÊËŒ"; options = L"1:É 2:È 3:Ê 4:Ë 5:Œ"; break;
        case L'I': _currentAccents = L"ÎÏ"; options = L"1:Î 2:Ï"; break;
        case L'O': _currentAccents = L"ÔÖŒ"; options = L"1:Ô 2:Ö 3:Œ"; break;
        case L'U': _currentAccents = L"ÙÛÜ"; options = L"1:Ù 2:Û 3:Ü"; break;
        case L'C': _currentAccents = L"Ç"; options = L"1:Ç"; break;
        default:
            // 매칭되는 문자가 없으면 (예: 'k' 뒤에 한자) 원래 한자키 기능 통과
            return S_OK;
        }

        // 일치하는 문자가 있어서 options가 채워졌다면 좌표 구해서 팔레트 호출
        if (!options.empty())
        {
            RECT rc = { 0 };
            CReadSession* pReadSession = new CReadSession(pic, &rc);
            HRESULT hrSession;
            pic->RequestEditSession(_tid, pReadSession, TF_ES_READ | TF_ES_SYNC, &hrSession);
            pReadSession->Release();

            if (rc.left != 0 || rc.bottom != 0)
            {
                _palette.Show(rc.left, rc.bottom, options);
                *pfEaten = TRUE;
            }
        }
        return S_OK;
    }

    // ── 3. 영문자 추적 (Last Char) ──
    if (wParam >= 'A' && wParam <= 'Z')
    {
        bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool isCaps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        bool isUpper = isShift ^ isCaps;

        _lastChar = (WCHAR)(isUpper ? wParam : (wParam + 32));
    }
    else if (wParam != 0x19)
    {
        // 알파벳이나 한자키가 아닌 백스페이스, 스페이스바 등을 누르면 기억 리셋
        _lastChar = 0;
    }

    return S_OK;
}

STDMETHODIMP CTextService::OnTestKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten)
{
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CTextService::OnPreservedKey(ITfContext* pic, REFGUID rguid, BOOL* pfEaten)
{
    *pfEaten = FALSE;

    if (IsEqualGUID(rguid, GUID_KEY_DECIMAL_SWITCH))
    {
        // 텐키 . = 한자 키 대체 → 악센트 팔레트 표시 (OnKeyDown의 0x19 로직과 동일)
        std::wstring options;
        switch (_lastChar)
        {
        case L'a': _currentAccents = L"àâäæ";  options = L"1:à 2:â 3:ä 4:æ"; break;
        case L'e': _currentAccents = L"éèêëœ"; options = L"1:é 2:è 3:ê 4:ë 5:œ"; break;
        case L'i': _currentAccents = L"îï";    options = L"1:î 2:ï"; break;
        case L'o': _currentAccents = L"ôöœ";   options = L"1:ô 2:ö 3:œ"; break;
        case L'u': _currentAccents = L"ùûü";   options = L"1:ù 2:û 3:ü"; break;
        case L'c': _currentAccents = L"ç";     options = L"1:ç"; break;
        case L'A': _currentAccents = L"ÀÂÄÆ";  options = L"1:À 2:Â 3:Ä 4:Æ"; break;
        case L'E': _currentAccents = L"ÉÈÊËŒ"; options = L"1:É 2:È 3:Ê 4:Ë 5:Œ"; break;
        case L'I': _currentAccents = L"ÎÏ";    options = L"1:Î 2:Ï"; break;
        case L'O': _currentAccents = L"ÔÖŒ";   options = L"1:Ô 2:Ö 3:Œ"; break;
        case L'U': _currentAccents = L"ÙÛÜ";   options = L"1:Ù 2:Û 3:Ü"; break;
        case L'C': _currentAccents = L"Ç";     options = L"1:Ç"; break;
        default:
            return S_OK;  // 매칭 없으면 키 통과
        }

        if (!options.empty())
        {
            int posX = 0, posY = 0;

            // 텍스트 커서 위치 시도 (pic이 유효할 때만)
            if (pic)
            {
                RECT rc = { 0 };
                CReadSession* pReadSession = new CReadSession(pic, &rc);
                HRESULT hrSession;
                pic->RequestEditSession(_tid, pReadSession, TF_ES_READ | TF_ES_SYNC, &hrSession);
                pReadSession->Release();

                if (rc.left != 0 || rc.bottom != 0)
                {
                    posX = rc.left;
                    posY = rc.bottom;
                }
            }

            // 커서 위치를 못 구하면 마우스 포인터 위치에 팔레트 표시
            if (posX == 0 && posY == 0)
            {
                POINT pt;
                GetCursorPos(&pt);
                posX = pt.x;
                posY = pt.y;
            }

            _palette.Show(posX, posY, options);
            *pfEaten = TRUE;
        }
    }

    return S_OK;
}


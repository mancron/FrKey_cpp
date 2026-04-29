#include "EditSession.h"

CEditSession::CEditSession(ITfContext* pContext, WCHAR chAccent) : _cRef(1)
{
    _pContext = pContext;
    _pContext->AddRef();
    _chAccent = chAccent;
}

CEditSession::~CEditSession()
{
    _pContext->Release();
}

STDMETHODIMP CEditSession::QueryInterface(REFIID riid, void** ppvObj)
{
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession))
    {
        *ppvObj = (ITfEditSession*)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEditSession::AddRef() { return InterlockedIncrement(&_cRef); }

STDMETHODIMP_(ULONG) CEditSession::Release()
{
    LONG cr = InterlockedDecrement(&_cRef);
    if (cr == 0) delete this;
    return cr;
}

// ──────────────────────────────────────
// 실제 텍스트 편집 로직 (OS가 동기화 락을 걸고 안전하게 호출해 줌)
// ──────────────────────────────────────
STDMETHODIMP CEditSession::DoEditSession(TfEditCookie ec)
{
    TF_SELECTION tfSelection;
    ULONG cFetched;

    // 1. 현재 커서(캐럿)의 위치를 가져옵니다.
    if (FAILED(_pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &cFetched)) || cFetched != 1)
        return S_OK;

    ITfRange* pRange = tfSelection.range;
    LONG cch = 0;

    // 2. 선택 범위를 커서 바로 앞의 1글자(원본 영문자)를 포함하도록 뒤로 1칸 확장합니다.
    // (예: "a|" 상태에서 범위 시작점을 왼쪽으로 옮겨 "[a]|" 형태로 만듦)
    pRange->ShiftStart(ec, -1, &cch, nullptr);

    if (cch == 0)
    {
        pRange->Release();
        return S_OK;
    }

    // 3. 확장된 범위의 텍스트(원본 1글자)를 새로운 악센트 문자 1글자로 덮어씁니다.
    pRange->SetText(ec, 0, &_chAccent, 1);

    // 4. 교체가 끝난 후, 커서 위치를 새로 삽입된 문자 바로 뒤로 이동시킵니다.
    pRange->Collapse(ec, TF_ANCHOR_END);
    tfSelection.range = pRange;
    _pContext->SetSelection(ec, 1, &tfSelection);

    pRange->Release();
    return S_OK;
}
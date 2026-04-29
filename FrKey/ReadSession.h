#pragma once
#include <msctf.h>

// 커서(캐럿)의 화면 좌표만 안전하게 읽어오는 전용 세션 객체
class CReadSession : public ITfEditSession
{
public:
    CReadSession(ITfContext* pContext, RECT* pRect) : _cRef(1), _pContext(pContext), _pRect(pRect)
    {
        _pContext->AddRef();
    }
    ~CReadSession() { _pContext->Release(); }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) {
        if (ppvObj == nullptr) return E_INVALIDARG;
        *ppvObj = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
            *ppvObj = (ITfEditSession*)this; AddRef(); return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_cRef); }
    STDMETHODIMP_(ULONG) Release() { LONG cr = InterlockedDecrement(&_cRef); if (cr == 0) delete this; return cr; }

    STDMETHODIMP DoEditSession(TfEditCookie ec) {
        ITfContextView* pView = nullptr;
        if (SUCCEEDED(_pContext->GetActiveView(&pView))) {
            TF_SELECTION tfSel;
            ULONG cFetched;
            // 여기서 발급받은 정식 쿠키(ec)를 사용하여 에러 없이 읽기 성공
            if (SUCCEEDED(_pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSel, &cFetched)) && cFetched == 1) {
                BOOL fClipped;
                pView->GetTextExt(ec, tfSel.range, _pRect, &fClipped);
                tfSel.range->Release();
            }
            pView->Release();
        }
        return S_OK;
    }
private:
    LONG _cRef;
    ITfContext* _pContext;
    RECT* _pRect; // 결과값을 담을 포인터
};
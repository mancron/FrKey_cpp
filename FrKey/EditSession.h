#pragma once
#include <msctf.h>

/// <summary>
/// 대상 프로그램의 텍스트 버퍼를 직접 수정하기 위한 동기화 객체입니다.
/// </summary>
class CEditSession : public ITfEditSession
{
public:
    // 생성자: 수정할 문서의 컨텍스트(Context)와 삽입할 악센트 문자를 받습니다.
    CEditSession(ITfContext* pContext, WCHAR chAccent);
    ~CEditSession();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfEditSession 인터페이스의 핵심 (권한을 획득했을 때 OS가 호출함)
    STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
    LONG _cRef;
    ITfContext* _pContext;
    WCHAR _chAccent; // 삽입할 최종 악센트 문자 (예: 'à')
};
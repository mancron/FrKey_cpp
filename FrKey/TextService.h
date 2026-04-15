#pragma once
#include <msctf.h> // TSF 핵심 헤더
#include "PaletteWindow.h"


class CTextService : public ITfTextInputProcessor,
    public ITfKeyEventSink
{
public:
    CTextService();
    ~CTextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfTextInputProcessor
    STDMETHODIMP Activate(ITfThreadMgr* ptim, TfClientId tid);
    STDMETHODIMP Deactivate();

    // ──────────────────────────────────────
    // ITfKeyEventSink (키보드 이벤트 가로채기)
    // ──────────────────────────────────────
    STDMETHODIMP OnSetFocus(BOOL fForeground);
    STDMETHODIMP OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
    STDMETHODIMP OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
    STDMETHODIMP OnTestKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
    STDMETHODIMP OnKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
    STDMETHODIMP OnPreservedKey(ITfContext* pic, REFGUID rguid, BOOL* pfEaten);

private:
    BOOL _InitKeyEventSink();
    void _UninitKeyEventSink();

    LONG _cRef;
    ITfThreadMgr* _pThreadMgr;
    TfClientId _tid;

    // 시스템에 키보드 싱크를 등록할 때 발급받는 고유 번호
    DWORD _dwKeyEventSinkCookie;
    CPaletteWindow _palette;
    WCHAR _lastChar;

    std::wstring _currentAccents;
};
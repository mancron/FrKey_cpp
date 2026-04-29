#pragma once
#include <msctf.h> // TSF �ٽ� ���
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

    // ����������������������������������������������������������������������������
    // ITfKeyEventSink (Ű���� �̺�Ʈ ����ä��)
    // ����������������������������������������������������������������������������
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

    CPaletteWindow _palette;
    WCHAR _lastChar;

    std::wstring _currentAccents;
};
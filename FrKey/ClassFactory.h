#pragma once
#include <unknwn.h>

class CClassFactory : public IClassFactory
{
public:
    CClassFactory();
    ~CClassFactory();

    // IUnknown 인터페이스 구현 (메모리 및 객체 수명 관리)
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory 인터페이스 구현 (실제 객체 생성)
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObj);
    STDMETHODIMP LockServer(BOOL fLock);

private:
    LONG _cRef; // 팩토리 자체의 참조 카운트
};
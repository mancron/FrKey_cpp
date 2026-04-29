#include "ClassFactory.h"
#include "Globals.h"
#include "TextService.h"
#include <msctf.h>

LONG g_cRefDll = 0; // ���� ���� ī��Ʈ �ʱ�ȭ

// Globals.h�� ������ ���� ID�� ���� �� �Ҵ� (�ݵ�� ������ GUID�� ����)
const CLSID CLSID_FrKeyIME = { 0x9b6b5af4, 0x7ef1, 0x4238, { 0xbe, 0xaf, 0x11, 0x4f, 0x32, 0xa7, 0x73, 0xcf } };


// ����������������������������������������������������������������������������
// CClassFactory ����
// ����������������������������������������������������������������������������
CClassFactory::CClassFactory() : _cRef(1)
{
    InterlockedIncrement(&g_cRefDll);
}

CClassFactory::~CClassFactory()
{
    InterlockedDecrement(&g_cRefDll);
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppvObj)
{
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObj = (IClassFactory*)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef() { return InterlockedIncrement(&_cRef); }

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    LONG cr = InterlockedDecrement(&_cRef);
    if (cr == 0) delete this;
    return cr;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObj)
{
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;
    if (pUnkOuter != nullptr) return CLASS_E_NOAGGREGATION;

    // TODO ���� �κ��� ��� ���� ���� ���� ��ü ���� �������� ��ü
    CTextService* pTextService = new CTextService();
    if (pTextService == nullptr) return E_OUTOFMEMORY;

    HRESULT hr = pTextService->QueryInterface(riid, ppvObj);
    pTextService->Release();

    return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock) InterlockedIncrement(&g_cRefDll);
    else InterlockedDecrement(&g_cRefDll);
    return S_OK;
}

// ����������������������������������������������������������������������������
// DLL �������� �Լ� (OS�� ȣ��)
// ����������������������������������������������������������������������������
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppvObj)
{
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;

    // OS�� ��û�ϴ� Ŭ������ �츮�� ���� ����ġ �Ǽ�Ʈ �Է±Ⱑ �´��� Ȯ��
    if (!IsEqualCLSID(rclsid, CLSID_FrKeyIME))
        return CLASS_E_CLASSNOTAVAILABLE;

    CClassFactory* pFactory = new CClassFactory();
    if (pFactory == nullptr) return E_OUTOFMEMORY;

    HRESULT hr = pFactory->QueryInterface(riid, ppvObj);
    pFactory->Release();
    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    // ��� ���� ��ü�� 0���� ���� DLL�� �޸𸮿��� ���� ���
    return (g_cRefDll == 0) ? S_OK : S_FALSE;
}


// Globals.h�� ������ ������ GUID ���� �� �Ҵ� (�ݵ�� ���θ��� GUID�� ����)
const GUID GUID_PROFILE_FRKEY = { 0x4e928bee, 0x5c6b, 0x46a3, { 0x83, 0x3b, 0x22, 0xd6, 0x29, 0xb7, 0xe, 0xbf } };


// ������Ʈ�� Ű ���� ���� �Լ�
BOOL SetRegKey(HKEY hKeyRoot, LPCWSTR pszSubKey, LPCWSTR pszValueName, LPCWSTR pszData)
{
    HKEY hKey;
    if (RegCreateKeyExW(hKeyRoot, pszSubKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return FALSE;

    BOOL bRet = (RegSetValueExW(hKey, pszValueName, 0, REG_SZ, (const BYTE*)pszData, (lstrlenW(pszData) + 1) * sizeof(WCHAR)) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return bRet;
}

// ����������������������������������������������������������������������������
// ���� �Է±� ��ġ (regsvr32 �� �����)
// ����������������������������������������������������������������������������
STDAPI DllRegisterServer(void)
{
    WCHAR szModulePath[MAX_PATH];
    GetModuleFileNameW(g_hInst, szModulePath, MAX_PATH);

    // 1. COM ������Ʈ�� ��� (�ý����� DLL ��θ� ã�� �� �ְ� ��)
    // ����: CLSID ���ڿ� ��ȯ�� ����ȭ�� ���� �ϵ��ڵ� ���·� ���� (�����δ� StringFromGUID2 �� ��� ����)
    // ���⼭�� ���ڿ� ���������� ó���մϴ�.
    WCHAR szCLSID[100];
    StringFromGUID2(CLSID_FrKeyIME, szCLSID, 100);

    std::wstring keyPath = std::wstring(L"CLSID\\") + szCLSID;
    SetRegKey(HKEY_CLASSES_ROOT, keyPath.c_str(), nullptr, L"French Accent IME");
    SetRegKey(HKEY_CLASSES_ROOT, (keyPath + L"\\InprocServer32").c_str(), nullptr, szModulePath);
    SetRegKey(HKEY_CLASSES_ROOT, (keyPath + L"\\InprocServer32").c_str(), L"ThreadingModel", L"Apartment");

    // 2. TSF ������ �� ī�װ��� ���
    ITfInputProcessorProfiles* pProfiles = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void**)&pProfiles)))
    {
        // �ؽ�Ʈ ���񽺷� ���
        pProfiles->Register(CLSID_FrKeyIME);

        // ��������(0x040C) ���� �����ʷ� ���
        WCHAR szDesc[] = L"French Accent IME";
        pProfiles->AddLanguageProfile(CLSID_FrKeyIME, 0x040C, GUID_PROFILE_FRKEY, szDesc, (ULONG)wcslen(szDesc), szModulePath, (ULONG)wcslen(szModulePath), 0);
        pProfiles->EnableLanguageProfile(CLSID_FrKeyIME, 0x040C, GUID_PROFILE_FRKEY, TRUE);
        pProfiles->Release();
    }

    ITfCategoryMgr* pCategoryMgr = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&pCategoryMgr)))
    {
        // Ű����(�Է±�) ī�װ����� ����
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIP_KEYBOARD, CLSID_FrKeyIME);

        // �ȷ�Ʈ UI ������ �ʼ� (�̰� ������ Ű �̺�Ʈ �� �޴� ���� ����)
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_UIELEMENTENABLED, CLSID_FrKeyIME);

        // Win8 �̻� Modern/����� ��, Edge ��� �����Ϸ��� �ʼ�
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_COMLESS, CLSID_FrKeyIME);
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT, CLSID_FrKeyIME);
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT, CLSID_FrKeyIME);

        // ���ȭ��/UAC������ ������
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_SECUREMODE, CLSID_FrKeyIME);

        // ��/�� ���� �Է¸�� ȣȯ
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT, CLSID_FrKeyIME);
        pCategoryMgr->Release();
    }

    return S_OK;
}

// ����������������������������������������������������������������������������
// ���� �Է±� ����
// ����������������������������������������������������������������������������
STDAPI DllUnregisterServer(void)
{
    // 1. TSF ������ �� ī�װ��� ����
    ITfInputProcessorProfiles* pProfiles = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void**)&pProfiles)))
    {
        pProfiles->Unregister(CLSID_FrKeyIME);
        pProfiles->Release();
    }

    ITfCategoryMgr* pCategoryMgr = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&pCategoryMgr)))
    {
        pCategoryMgr->UnregisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIP_KEYBOARD, CLSID_FrKeyIME);
        pCategoryMgr->UnregisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_UIELEMENTENABLED, CLSID_FrKeyIME);
        pCategoryMgr->UnregisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_COMLESS, CLSID_FrKeyIME);
        pCategoryMgr->UnregisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT, CLSID_FrKeyIME);
        pCategoryMgr->UnregisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT, CLSID_FrKeyIME);
        pCategoryMgr->UnregisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_SECUREMODE, CLSID_FrKeyIME);
        pCategoryMgr->UnregisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT, CLSID_FrKeyIME);
        pCategoryMgr->Release();
    }

    // 2. COM ������Ʈ�� ����
    WCHAR szCLSID[100];
    StringFromGUID2(CLSID_FrKeyIME, szCLSID, 100);
    std::wstring keyPath = std::wstring(L"CLSID\\") + szCLSID;

    RegDeleteKeyW(HKEY_CLASSES_ROOT, (keyPath + L"\\InprocServer32").c_str());
    RegDeleteKeyW(HKEY_CLASSES_ROOT, keyPath.c_str());

    return S_OK;
}
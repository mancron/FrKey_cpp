#include "ClassFactory.h"
#include "Globals.h"
#include "TextService.h"
#include <msctf.h>
#include <shlobj.h>   // SHGetKnownFolderPath
#include <strsafe.h>  // StringCchCopyW

LONG g_cRefDll = 0; // 전역 참조 카운트 초기화

// Globals.h에 선언한 고유 ID의 실제 값 할당 (반드시 본인의 GUID로 변경)
const CLSID CLSID_FrKeyIME = { 0x9b6b5af4, 0x7ef1, 0x4238, { 0xbe, 0xaf, 0x11, 0x4f, 0x32, 0xa7, 0x73, 0xcf } };


// ──────────────────────────────────────
// CClassFactory 구현
// ──────────────────────────────────────
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

    // TODO 였던 부분을 방금 만든 메인 서비스 객체 생성 로직으로 교체
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

// ──────────────────────────────────────
// DLL 내보내기 함수 (OS가 호출)
// ──────────────────────────────────────
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppvObj)
{
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;

    // OS가 요청하는 클래스가 우리가 만든 프렌치 악센트 입력기가 맞는지 확인
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
    // 사용 중인 객체가 0개일 때만 DLL을 메모리에서 해제 허용
    return (g_cRefDll == 0) ? S_OK : S_FALSE;
}


// Globals.h에 선언한 프로필 GUID 실제 값 할당 (반드시 본인만의 GUID로 변경)
const GUID GUID_PROFILE_FRKEY = { 0x4e928bee, 0x5c6b, 0x46a3, { 0x83, 0x3b, 0x22, 0xd6, 0x29, 0xb7, 0xe, 0xbf } };

// 텐키 . 키를 PreserveKey로 등록할 때 사용하는 고유 식별자
const GUID GUID_KEY_DECIMAL_SWITCH = { 0xa1b2c3d4, 0xe5f6, 0x7890, { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90 } };


// 레지스트리 키 쓰기 헬퍼 함수
BOOL SetRegKey(HKEY hKeyRoot, LPCWSTR pszSubKey, LPCWSTR pszValueName, LPCWSTR pszData)
{
    HKEY hKey;
    if (RegCreateKeyExW(hKeyRoot, pszSubKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return FALSE;

    BOOL bRet = (RegSetValueExW(hKey, pszValueName, 0, REG_SZ, (const BYTE*)pszData, (lstrlenW(pszData) + 1) * sizeof(WCHAR)) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return bRet;
}

// ──────────────────────────────────────
// 정식 입력기 설치 (regsvr32 로 실행됨)
// ──────────────────────────────────────
STDAPI DllRegisterServer(void)
{
    // ── DLL을 고정 경로(%ProgramData%\FrKey\)에 복사 ──
    // 이렇게 하면 원본 DLL을 어디로 옮겨도 재등록이 불필요합니다.
    WCHAR szModulePath[MAX_PATH] = {};  // 복사 대상 경로 (고정)
    {
        PWSTR pszProgramData = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &pszProgramData)))
        {
            // %ProgramData%\FrKey\ 폴더 생성
            std::wstring destDir = std::wstring(pszProgramData) + L"\\FrKey";
            CreateDirectoryW(destDir.c_str(), nullptr);

            // 복사 대상 전체 경로
            std::wstring destPath = destDir + L"\\FrKey.dll";
            StringCchCopyW(szModulePath, MAX_PATH, destPath.c_str());

            // 현재 DLL → 고정 위치로 복사 (덮어쓰기)
            WCHAR szSrcPath[MAX_PATH];
            GetModuleFileNameW(g_hInst, szSrcPath, MAX_PATH);
            CopyFileW(szSrcPath, szModulePath, FALSE);

            CoTaskMemFree(pszProgramData);
        }
    }
    if (szModulePath[0] == L'\0') return E_FAIL;  // 복사 실패 시 중단

    // 1. COM 레지스트리 등록 — 고정 경로로 등록하므로 이후 원본을 옮겨도 무관
    WCHAR szCLSID[100];
    StringFromGUID2(CLSID_FrKeyIME, szCLSID, 100);

    std::wstring keyPath = std::wstring(L"CLSID\\") + szCLSID;
    SetRegKey(HKEY_CLASSES_ROOT, keyPath.c_str(), nullptr, L"French Accent IME");
    SetRegKey(HKEY_CLASSES_ROOT, (keyPath + L"\\InprocServer32").c_str(), nullptr, szModulePath);
    SetRegKey(HKEY_CLASSES_ROOT, (keyPath + L"\\InprocServer32").c_str(), L"ThreadingModel", L"Apartment");

    // 2. TSF 프로필 및 카테고리 등록
    ITfInputProcessorProfiles* pProfiles = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void**)&pProfiles)))
    {
        // 텍스트 서비스로 등록
        pProfiles->Register(CLSID_FrKeyIME);

        // 한국어(0x0412) 하위 프로필로 등록 — QWERTY 레이아웃을 유지하면서 프랑스어 악센트 입력
        WCHAR szDesc[] = L"French Accent IME";
        pProfiles->AddLanguageProfile(CLSID_FrKeyIME, 0x0412, GUID_PROFILE_FRKEY, szDesc, (ULONG)wcslen(szDesc), szModulePath, (ULONG)wcslen(szModulePath), 0);
        pProfiles->EnableLanguageProfile(CLSID_FrKeyIME, 0x0412, GUID_PROFILE_FRKEY, TRUE);
        pProfiles->Release();
    }

    ITfCategoryMgr* pCategoryMgr = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&pCategoryMgr)))
    {
        // 키보드(입력기) 카테고리에 편입
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIP_KEYBOARD, CLSID_FrKeyIME);

        // 팔레트 UI 띄우려면 필수 (이게 빠지면 키 이벤트 못 받는 앱이 많음)
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_UIELEMENTENABLED, CLSID_FrKeyIME);

        // Win8 이상 Modern/스토어 앱, Edge 등에서 동작하려면 필수
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_COMLESS, CLSID_FrKeyIME);
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT, CLSID_FrKeyIME);
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT, CLSID_FrKeyIME);

        // 잠금화면/UAC에서도 쓰려면
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_SECUREMODE, CLSID_FrKeyIME);

        // 한/영 같은 입력모드 호환
        pCategoryMgr->RegisterCategory(CLSID_FrKeyIME, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT, CLSID_FrKeyIME);
        pCategoryMgr->Release();
    }

    return S_OK;
}

// ──────────────────────────────────────
// 정식 입력기 삭제
// ──────────────────────────────────────
STDAPI DllUnregisterServer(void)
{
    // 1. TSF 프로필 및 카테고리 해제
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

    // 2. COM 레지스트리 삭제
    WCHAR szCLSID[100];
    StringFromGUID2(CLSID_FrKeyIME, szCLSID, 100);
    std::wstring keyPath = std::wstring(L"CLSID\\") + szCLSID;

    RegDeleteKeyW(HKEY_CLASSES_ROOT, (keyPath + L"\\InprocServer32").c_str());
    RegDeleteKeyW(HKEY_CLASSES_ROOT, keyPath.c_str());

    // 3. %ProgramData%\FrKey\에 복사해 둔 DLL 및 폴더 삭제
    PWSTR pszProgramData = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &pszProgramData)))
    {
        std::wstring destDir  = std::wstring(pszProgramData) + L"\\FrKey";
        std::wstring destFile = destDir + L"\\FrKey.dll";
        DeleteFileW(destFile.c_str());
        RemoveDirectoryW(destDir.c_str());  // 폴더가 비어있을 때만 삭제됨
        CoTaskMemFree(pszProgramData);
    }

    return S_OK;
}
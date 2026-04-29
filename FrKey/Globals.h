#pragma once
#include <windows.h>

// ★ 팩트 체크: 이 GUID는 시스템에 등록되는 고유 식별자입니다.
// Visual Studio의 [도구] -> [GUID 만들기] 기능을 사용해 새 GUID를 발급받아 교체해야 충돌이 발생하지 않습니다.
// 예시용 CLSID: {11111111-2222-3333-4444-555555555555}
extern const CLSID CLSID_FrKeyIME;

// DLL 진입점에서 저장한 인스턴스 핸들
extern HINSTANCE g_hInst;

// DLL이 메모리에서 내려가도 되는지 판단하기 위한 전역 참조 카운트
extern LONG g_cRefDll;


extern const GUID GUID_PROFILE_FRKEY;
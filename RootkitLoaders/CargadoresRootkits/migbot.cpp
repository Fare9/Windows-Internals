#include "stdafx.h"
#include "migbot.h"


typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE
{
	UNICODE_STRING ModuleName;
} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE;

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0) 

typedef NTSTATUS(__stdcall *ZWSETSYSTEMINFORMATION)(
	DWORD SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength
	);

typedef VOID(__stdcall *RTLINITUNICODESTRING)(
	PUNICODE_STRING DestinationString,
	PCWSTR SourceString
	);


// variables
ZWSETSYSTEMINFORMATION ZwSetSystemInformation;
RTLINITUNICODESTRING RtlInitUnicodeString;

BOOL load_sysfile(WCHAR *name)
/*
*	Load_sysfile:
*		Loader for the file sys, it uses an undocumented method.
*		This method is not safe because it loads the driver in
*		pageable memory, so the driver must have the code in 
*		no pageable memory.
*		Usert must be admin or system.
*		More information about ZwSetSystemInformation: http://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/ex/sysinfo/set.htm
*	Parameters:
*		WCHAR name: path to the example driver L"\\??\\C:\\driver.sys"  (file in C:\driver.sys)
*	Return:
*		Bool return if everything was okay or not.
*/
{
	SYSTEM_LOAD_AND_CALL_IMAGE	GregsImage;

	//////////////////////////////////////////////////
	// Get the function from ntdll.dll
	/////////////////////////////////////////////////
	if (!(RtlInitUnicodeString = (RTLINITUNICODESTRING)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlInitUnicodeString")))
	{
		return FALSE;
	}

	if (!(ZwSetSystemInformation = (ZWSETSYSTEMINFORMATION)GetProcAddress(GetModuleHandleA("ntdll.dll"), "ZwSetSystemInformation")))
	{
		return FALSE;
	}

	RtlInitUnicodeString(&(GregsImage.ModuleName), name);

	if (!NT_SUCCESS(ZwSetSystemInformation(SystemLoadAndCallImage, &GregsImage, sizeof(SYSTEM_LOAD_AND_CALL_IMAGE))))
	{
		return FALSE;
	}

	return TRUE;
	
}
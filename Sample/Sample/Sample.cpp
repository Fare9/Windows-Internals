#include <ntddk.h>


void
SampleUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	KdPrint(("Sample driver Unload called\n"));
}

extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	// Unferenced parameters
	UNREFERENCED_PARAMETER(RegistryPath);
	// Variables
	OSVERSIONINFOEXW osversioninfoexw;

	// Assignment of variables
	DriverObject->DriverUnload = SampleUnload;
	RtlZeroMemory(&osversioninfoexw, sizeof(OSVERSIONINFOEXW));
	osversioninfoexw.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

	// Function execution
	
	RtlGetVersion((PRTL_OSVERSIONINFOW)&osversioninfoexw);

	// Exercise Code
	KdPrint(("Sample driver initialized successfully\n"));
	KdPrint(("OS System information:\n"));
	KdPrint(("\t[Major Version]     ===> %x\n", osversioninfoexw.dwMajorVersion));
	KdPrint(("\t[Minor Version]     ===> %x\n", osversioninfoexw.dwMinorVersion));
	KdPrint(("\t[Build Number]      ===> %x\n", osversioninfoexw.dwBuildNumber));
	KdPrint(("\t[Platform ID]       ===> %x\n", osversioninfoexw.dwPlatformId));
	KdPrint(("\t[Major ServicePack] ===> %x\n", osversioninfoexw.wServicePackMajor));
	KdPrint(("\t[Minor ServicePack] ===> %x\n", osversioninfoexw.wServicePackMinor));
	KdPrint(("\t[Suite Mask]        ===> %x\n", osversioninfoexw.wSuiteMask));
	KdPrint(("\t[Product Type]      ===> %x\n", osversioninfoexw.wProductType));

	return STATUS_SUCCESS;
}



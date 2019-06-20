#include <ntddk.h>

#define DRIVER_TAG 'dcba'

UNICODE_STRING g_RegistryPath;

void
SampleUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	
	ExFreePool(g_RegistryPath.Buffer);
	KdPrint(("Freed allocated pool\n"));

	KdPrint(("Sample driver Unload called\n"));
}



extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	DriverObject->DriverUnload = SampleUnload;

	// Allocate paged pool with the Length of the RegistryPath string
	// this pool can be paged out from RAM.
	g_RegistryPath.Buffer = (WCHAR*)ExAllocatePoolWithTag(PagedPool,
														  RegistryPath->Length,
														  DRIVER_TAG);

	if (g_RegistryPath.Buffer == nullptr)
	{
		KdPrint(("Failed to allocate memory\n"));
		// Some Status error
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Maximum length will be set to the length of registry path
	g_RegistryPath.MaximumLength = RegistryPath->Length;
	// Rtl function for copying UNICODE_STRING functions
	RtlCopyUnicodeString(&g_RegistryPath, (PCUNICODE_STRING)RegistryPath);

	// %wZ is for UNICODE_STRING objects
	KdPrint(("Copied registry path: %wZ\n", &g_RegistryPath));

	return STATUS_SUCCESS;
}
#include "common.h"
#include "filters.h"

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID Unload(IN PDRIVER_OBJECT DriverObject);

DRIVER_INITIALIZE DriverEntry;


VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	DbgPrint("%s\n", "Driver unloaded.");

	return;
}


NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(RegistryPath);

	DbgPrint("%s\n", "Driver has been loaded.");

	DriverObject->DriverUnload = Unload;

	status = PsSetLoadImageNotifyRoutine(NotifyDriverOnLoad);
	
	if (NT_SUCCESS(status))
		DbgPrint("%s\n", "Filter installed properly.");
	else
		DbgPrint("%s\n", "Problem installing filter.");

	return status;
}
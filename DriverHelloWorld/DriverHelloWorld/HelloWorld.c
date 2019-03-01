#include <ntddk.h>

// declaration of functions to use
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID Unload(IN PDRIVER_OBJECT DriverObject);

// driver entry, needed to stablish an entry point for the driver
DRIVER_INITIALIZE DriverEntry;

VOID Unload(IN PDRIVER_OBJECT DriverObject)
/**
*	Unload:
*		Funtion that will be executed once the driver is unloaded
*		from the system.
*	Parameters:
*		:PDRIVER_OBJECT	DriverObject: represents the image of a loaded kernel-mode driver
*	Return:
*		VOID
*/
{
	// necessary if we are not going to use the parameter
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrint("%s\n", "Driver unloaded.");
	return;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
/**
*	DriverEntry:
*		Function that will be executed at the start of the driver
*	Parameters:
*		:PDRIVER_OBJECT	DriverObject: represents the image of a loaded kernel-mode driver, necessary for callback functions
*		:PUNICODE_STRING: registry path of the driver
*	Return:
*		NTSTATUS: status de retorno
*/
{
	// we are not going to use RegistryPath...
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("%s\n", "Driver has been loaded.");

	// callback function to execute on Driver unload
	DriverObject->DriverUnload = Unload;

	return status;
}
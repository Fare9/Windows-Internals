#include <ntifs.h>
#include <ntddk.h>
#include "PriorityBoosterCommon.h"

#define DEVICE_NAME L"\\Device\\PriorityBooster"
#define SYMLIN_NAME L"\\??\\PriorityBooster"

// Function Prototypes
void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS PriorityBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp);
NTSTATUS PriorityBoosterDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);


extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status;

	DriverObject->DriverUnload							= PriorityBoosterUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE]			= PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]			= PriorityBoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]	= PriorityBoosterDeviceControl;
	KdPrint(("Set Major Functions for Create, Close and Device Control\n"));

	// Driver's name
	UNICODE_STRING devName = RTL_CONSTANT_STRING(DEVICE_NAME);

	// Create device object from driver object
	status = IoCreateDevice(
		DriverObject,				// our driver object
		0,							// no need for extra bytes
		&devName,					// the device name
		FILE_DEVICE_UNKNOWN,		// device type, FILE_DEVICE_UNKNOWN used for software drivers
		0,							// characteristics flags, 0 for software drivers
		FALSE,						// not exclusive driver
		&DeviceObject				// resulting pointer
	);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		return status;
	}

	KdPrint(("Created Device\n"));
	
	// Finally create symbolic link for user mode access
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMLIN_NAME);
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);
		return status;
	}
	
	KdPrint(("Created Symbolic Link\n"));

	return STATUS_SUCCESS;
}


void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMLIN_NAME);

	// delete the symbolic link
	IoDeleteSymbolicLink(&symLink);
	KdPrint(("Deleted symbolic link\n"));

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);
	KdPrint(("Deleted Device object\n"));

	KdPrint(("Unloaded booster driver\n"));
}


NTSTATUS PriorityBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	/*
	*	Every dispatch routine accepts device object and an I/O Request packet.
	*	We have to dispatch routine, and set as success in case, of IRP was
	*	correctly handled.
	*	With the IRP we can figure out the type and details of the request.
	*	IRP comes with one or more IO_STACK_LOCATION structures.
	*/
	auto stack = IoGetCurrentIrpStackLocation(Irp);

	if (stack->MajorFunction == IRP_MJ_CREATE)
	{
		auto security_context = stack->Parameters.Create.SecurityContext;
		KdPrint(("Client called Create for this driver\n"));

		KdPrint(("Create to PriorityBoosterCreateClose: GenericWrite=%s GenericRead=%s \n",
			security_context->DesiredAccess & GENERIC_WRITE ? "TRUE" : "FALSE",
			security_context->DesiredAccess & GENERIC_READ ? "TRUE" : "FALSE"));
	}
	else if (stack->MajorFunction == IRP_MJ_CLOSE)
	{
		KdPrint(("Client called Close for this driver\n"));
	}
	
	
	Irp->IoStatus.Status		= STATUS_SUCCESS;
	Irp->IoStatus.Information	= 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS PriorityBoosterDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	KdPrint(("Client called DeviceIoControl\n"));
	/*
	*	Get IO_STACK_LOCATION
	*	From here we will be able to access for example MajorFunction, 
	*	or even call parameters
	*/
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_PRIORITY_BOOSTER_SET_PRIORITY:
	{ // brackets are necessary for declaring variables

		// stuff in here
		auto len = stack->Parameters.DeviceIoControl.InputBufferLength;
		if (len < sizeof(ThreadData))
		{
			KdPrint(("Invalid Input Buffer Length: %d", len));
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		auto data = (ThreadData*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		// check data is correct
		if (data == nullptr)
		{
			KdPrint(("Invalid input buffer\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (data->Priority < 1 || data->Priority > 31)
		{
			KdPrint(("Invalid thread priority: %d\n", data->Priority));
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		PETHREAD Thread;

		/*
		*	We give thread id as a handle because of the way process and thread IDs
		*	are generated, these are generated from a global private kernel handle table,
		*	so handle "values" are the IDs.
		*	As thread is now referenced by Thread the reference from HANDLE table is
		*	incremented and thread can't die.
		*/
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &Thread);

		if (!NT_SUCCESS(status))
			break;

		// Set the Priority
		KeSetPriorityThread((PKTHREAD)Thread, data->Priority);
		
		// Decrement the thread object's reference
		ObDereferenceObject(Thread);
		KdPrint(("Thread Priority change for %d to %d succeeded!\n", data->ThreadId, data->Priority));

		break;
	}

	default:
		KdPrint(("Invalid control code: %d\n", stack->Parameters.DeviceIoControl.IoControlCode));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}
#include "pch.h"
#include "zero_common.h"

#define DRIVER_PREFIX "Zero: "
#define DEVICE_NAME L"\\Device\\Zero"
#define SYMBOLIC_NAME L"\\??\\Zero"

void ZeroUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS ZeroCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp);
NTSTATUS ZeroRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp);
NTSTATUS ZeroWrite(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp);
NTSTATUS ZeroDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
// completion IRP function helper
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);


// global variables
ULONGLONG nBytesRead = 0;
ULONGLONG nBytesWrite = 0;

// DriverEntry

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	// Unload function
	DriverObject->DriverUnload = ZeroUnload;
	// Major functions
	DriverObject->MajorFunction[IRP_MJ_CREATE] = \
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = ZeroCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = ZeroRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = ZeroWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ZeroDeviceControl;

	// now we need to create device and symbolic objects
	UNICODE_STRING devName = RTL_CONSTANT_STRING(DEVICE_NAME);
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMBOLIC_NAME);

	PDEVICE_OBJECT DeviceObject = nullptr;
	auto status = STATUS_SUCCESS;

	do
	{
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
		
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
			break;
		}

		/***
		*	We will use Direct I/O, so I/O Manager check user's buffer
		*	is valid and faults it into physical memory.
		*	Locks the buffer in memory and cannot be paged out, so accessing
		*	buffer in any IRQL is safe.
		*	I/O Manager builds a MDL structure knows how buffer is mapped
		*	to RAM, address of structure is in "MdlAddress" of IRP structure.
		*	With that if we wanna use this buffer, we have to map it to
		*	a system address (we will have two maps to same buffer).
		*	The function is "MmGetSystemAddressForMdlSafe).
		*	After all, I/O manager removes second maping, frees MDL and
		*	unlocks user's buffer.
		*/
		DeviceObject->Flags |= DO_DIRECT_IO;

		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to create symbolic link (0x%08X)\n", status));
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status))
	{
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
	}

	return status;
}


void ZeroUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMBOLIC_NAME);

	// delete the symbolic link
	IoDeleteSymbolicLink(&symLink);
	KdPrint(("Deleted symbolic link\n"));

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);
	KdPrint(("Deleted Device object\n"));

	KdPrint(("Unloaded booster driver\n"));
}


NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status, ULONG_PTR info)
{
	// just assign status and complete request
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}


/*
*	User buffer read function, we have to check for
*	user buffer size.
*/
NTSTATUS ZeroRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	if (len == 0)
		return CompleteIrp(Irp, STATUS_INVALID_BUFFER_SIZE);

	nBytesRead += len;
	/*
	*	Second enum could be:
		typedef enum _MM_PAGE_PRIORITY {
			LowPagePriority,
			NormalPagePriority = 16,
			HighPagePriority = 32
		} MM_PAGE_PRIORITY;
	*
	*/
	auto buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!buffer)
		return CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES);

	// what are we gonna do is just to fill buffer with zeros
	memset(buffer, 0, len);

	// important to set information field as will be used by client.

	return CompleteIrp(Irp, STATUS_SUCCESS, len);
}


NTSTATUS ZeroWrite(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Write.Length;

	nBytesWrite += len;

	return CompleteIrp(Irp, STATUS_SUCCESS, len);
}


NTSTATUS ZeroCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	CompleteIrp(Irp, STATUS_SUCCESS);
	return STATUS_SUCCESS;
}


// part of exercise of chapter 7
NTSTATUS ZeroDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
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
	ULONG infoLength = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_ZERO_COUNT_BYTES:
	{
		__try
		{
			auto len = stack->Parameters.DeviceIoControl.InputBufferLength;
			if (len < sizeof(NumberOfBytes))
			{
				KdPrint(("Invalid input buffer\n"));
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			auto data = (PNumberOfBytes)Irp->AssociatedIrp.SystemBuffer;

			if (data == nullptr)
			{
				KdPrint(("Invalid input buffer\n"));
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			data->nBytesRead = nBytesRead;
			data->nBytesWrite = nBytesWrite;
			infoLength = sizeof(NumberOfBytes);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			status = STATUS_ACCESS_VIOLATION;
		}
		break;

	}
	default:
		KdPrint(("Invalid control code: %d\n", stack->Parameters.DeviceIoControl.IoControlCode));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	CompleteIrp(Irp, status, infoLength);
	return status;
}
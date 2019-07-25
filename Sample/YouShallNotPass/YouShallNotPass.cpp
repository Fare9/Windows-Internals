#include "pch.h"

// Headers
#include "YouShallNotPassCommon.h"
#include "YouShallNotPass.h"
#include "AutoLock.h"

// PROTOTYPES
DRIVER_UNLOAD DriverUnload;
DRIVER_DISPATCH DriverCreateClose, DriverDeviceControl;
void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo);
void PushPath(LIST_ENTRY* entry);

// Globals

Globals g_Globals;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	KdPrint((DRIVER_PREFIX "we are starting our journey\n"));

	auto status = STATUS_SUCCESS;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\" DEVICE_NAME);
	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\" DEVICE_NAME);
	PDEVICE_OBJECT DeviceObject = nullptr;
	bool symlinkCreated = false;

	g_Globals.Init();

	do
	{
		// Create the device
		status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &DeviceObject);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
			break;
		}

		status = IoCreateSymbolicLink(&symName, &deviceName);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to create sym link (0x%08X)\n", status));
			break;
		}
		symlinkCreated = true;

		// register for notifications of each process
		// we will use PsSetCreateProcessNotifyRoutineEx
		status = PsSetCreateProcessNotifyRoutineEx(&OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to register process callback (0x%08X)\n", status));
			break;
		}

		break;
	} while (false);

	if (!NT_SUCCESS(status))
	{
		if (symlinkCreated)
			IoDeleteSymbolicLink(&symName);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
	}

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;

	KdPrint((DRIVER_PREFIX "first journey is over\n"));

	return status;
}


void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	// unregister callbacks
	PsSetCreateProcessNotifyRoutineEx(&OnProcessNotify, TRUE);

	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\" DEVICE_NAME);
	IoDeleteSymbolicLink(&symName);
	IoDeleteDevice(DriverObject->DeviceObject);

	while (!IsListEmpty(&g_Globals.ItemsHead))
	{
		auto entry = RemoveHeadList(&g_Globals.ItemsHead);
		ExFreePool(CONTAINING_RECORD(entry, FullItem<BYTE>, Entry));
	}
	g_Globals.ItemCount = 0;

	return;
}


NTSTATUS DriverCreateClose(_In_ PDEVICE_OBJECT, _In_ PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


NTSTATUS DriverDeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp)
{
	auto IrpStack	= IoGetCurrentIrpStackLocation(Irp);
	auto status		= STATUS_SUCCESS;
	auto len		= 0;

	switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_YOU_SHALL_NOT_PASS:
	{
		ULONG allocSize = sizeof(FullItem<BYTE>);
		auto inputBufferSize = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
		auto inputBuffer = (WCHAR*)Irp->AssociatedIrp.SystemBuffer;

		if (inputBufferSize == 0 || inputBuffer == nullptr)
		{
			KdPrint((DRIVER_PREFIX "those input paths you try to walk are not correct\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		allocSize += inputBufferSize;

		auto info = (FullItem<BYTE>*)ExAllocatePoolWithTag(PagedPool, allocSize, DRIVER_TAG);

		if (info == nullptr)
		{
			KdPrint((DRIVER_PREFIX "it wasn't possible the allocation\n"));
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RtlZeroMemory(info, allocSize);

		auto& item = info->Data;
		memcpy((UCHAR*)&item, inputBuffer, inputBufferSize - sizeof(WCHAR));

		PushPath(&info->Entry);

		break;
	}
	case IOCTL_YOU_CAN_PASS_AGAIN:
	{
		auto inputBufferSize = IrpStack->Parameters.DeviceIoControl.InputBufferLength - sizeof(WCHAR);
		auto inputBuffer = (WCHAR*)Irp->AssociatedIrp.SystemBuffer;

		if (inputBufferSize == 0 || inputBuffer == nullptr)
		{
			KdPrint((DRIVER_PREFIX "those input paths you try to walk are not correct\n"));
			status = STATUS_INVALID_PARAMETER;
			break;
		}


		for (auto i = 0; i < g_Globals.ItemCount; i++)
		{
			auto entry = RemoveHeadList(&g_Globals.ItemsHead);
			auto info = CONTAINING_RECORD(entry, FullItem<WCHAR*>, Entry);
			auto item = (WCHAR*)&info->Data;

			if (::wcsncmp(inputBuffer, item, inputBufferSize / sizeof(WCHAR)) == 0)
			{
				g_Globals.ItemCount--;
				ExFreePool(CONTAINING_RECORD(entry, FullItem<BYTE>, Entry));
				break;
			}

			InsertTailList(&g_Globals.ItemsHead, entry);
		}

		status = STATUS_INVALID_PARAMETER;
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}


	// complete the request
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = len;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}


void OnProcessNotify(_Inout_ PEPROCESS, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	UNREFERENCED_PARAMETER(ProcessId);
	if (CreateInfo)
	{
		if (CreateInfo->FileOpenNameAvailable && CreateInfo->ImageFileName)
		{
			KdPrint((DRIVER_PREFIX "trying to pass the application %ws with PID %X\n", CreateInfo->ImageFileName->Buffer, ProcessId));

			AutoLock<FastMutex> lock(g_Globals.Mutex);

			bool itemFound = false;
			
			for (auto i = 0; i < g_Globals.ItemCount; i++)
			{
				auto entry = RemoveHeadList(&g_Globals.ItemsHead);
				auto info = CONTAINING_RECORD(entry, FullItem<WCHAR*>, Entry);
				auto item = (WCHAR*)&info->Data;

				if (wcsstr(CreateInfo->ImageFileName->Buffer, item) != nullptr)
				{
					InsertTailList(&g_Globals.ItemsHead, entry);
					itemFound = true;
					break;
				}

				InsertTailList(&g_Globals.ItemsHead, entry);
			}
		
			if (itemFound)
			{
				CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;
			}
		}
	}
}


void PushPath(LIST_ENTRY* entry)
{
	AutoLock<FastMutex> lock(g_Globals.Mutex); // till now to the end of the function we will have Mutex
											   // and will be freed on destructor at the end of the function

	if (g_Globals.ItemCount > 10)
	{
		auto head = RemoveHeadList(&g_Globals.ItemsHead);
		g_Globals.ItemCount--;
		ExFreePool(CONTAINING_RECORD(head, FullItem<WCHAR*>, Entry));
	}

	InsertTailList(&g_Globals.ItemsHead, entry);
	g_Globals.ItemCount++;
}
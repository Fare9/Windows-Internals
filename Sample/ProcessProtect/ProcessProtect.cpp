#include "pch.h"

#include "ProcessProtectCommon.h"
#include "ProcessProtect.h"
#include "AutoLock.h"
// PROTOTYPES

DRIVER_UNLOAD ProcessProtectUnload;
DRIVER_DISPATCH ProcessProtectCreateClose, ProcessProtectDeviceControl;

OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION Info);

bool FindProcess(ULONG pid);
bool AddProcess(ULONG pid);
bool RemoveProcess(ULONG pid);

// GLOBALS

Globals g_Data;
SIZE_T index;

// DriverEntry


extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	KdPrint((DRIVER_PREFIX "DriverEntry entered\n"));

	g_Data.Init();

	// array of OB_OPERATION_REGISTRATION structures to
	// set callbacks for handle create or duplicate
	OB_OPERATION_REGISTRATION operations[] =
	{
		{
			PsProcessType,													// object type this case only ProcessType
			OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,		// handle creation and duplication
			OnPreOpenProcess,												// callback pre function 
			nullptr															// callback post function
		}
	};

	// Registration of callbacks
	OB_CALLBACK_REGISTRATION reg =
	{
		OB_FLT_REGISTRATION_VERSION,
		1,									// operation count
		RTL_CONSTANT_STRING(L"12345.6515"),	// altitude
		nullptr,							// context
		operations
	};

	auto status = STATUS_SUCCESS;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\" PROCESS_PROTECT_NAME);
	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\" PROCESS_PROTECT_NAME);
	PDEVICE_OBJECT DeviceObject = nullptr;

	do
	{
		status = ObRegisterCallbacks(&reg, &g_Data.RegHandle);

		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "Error registering Object callback status = %08X (¡Dios mío que calvario!)\n", status));
			break;
		}

		status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to create device object (status=%08X)\n", status));
			break;
		}

		status = IoCreateSymbolicLink(&symName, &deviceName);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to create symbolic link (status=%08X)\n", status));
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status))
	{
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
		if (g_Data.RegHandle)
			ObUnRegisterCallbacks(&g_Data.RegHandle);
	}

	DriverObject->DriverUnload = ProcessProtectUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = ProcessProtectCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProcessProtectDeviceControl;

	KdPrint((DRIVER_PREFIX "DriverEntry completed successfully\n"));

	return status;
}


void ProcessProtectUnload(PDRIVER_OBJECT DriverObject) 
{
	ObUnRegisterCallbacks(g_Data.RegHandle);

	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\" PROCESS_PROTECT_NAME);
	IoDeleteSymbolicLink(&symName);
	IoDeleteDevice(DriverObject->DeviceObject);
}


NTSTATUS ProcessProtectCreateClose(PDEVICE_OBJECT, PIRP Irp)
{
	index = 0;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


NTSTATUS ProcessProtectDeviceControl(PDEVICE_OBJECT, PIRP Irp)
{
	auto stack	= IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;
	auto len = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_PROCESS_PROTECT_BY_PID:
	{
		auto size = stack->Parameters.DeviceIoControl.InputBufferLength;
		if (size % sizeof(ULONG) != 0)
		{
			// if size does not correspond to
			// an array of PIDs, error
			KdPrint((DRIVER_PREFIX "Error input buffer length does not correspond to array of PIDs\n"));
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		auto data = (ULONG*)Irp->AssociatedIrp.SystemBuffer;

		AutoLock<FastMutex> locker(g_Data.Lock);

		for (size_t i = 0; i < size / sizeof(ULONG); i++)
		{
			auto pid = data[i];

			if (pid == 0)
			{
				KdPrint((DRIVER_PREFIX "Error PID can't be 0\n"));
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (FindProcess(pid)) // pid already in list
				continue;

			if (g_Data.PidsCount == MaxPids) // max of PIDs
			{
				status = STATUS_TOO_MANY_CONTEXT_IDS;
				break;
			}

			if (!AddProcess(pid))
			{
				status = STATUS_UNSUCCESSFUL;
				break;
			}

			len += sizeof(ULONG);
		}

		break;
	}
	case IOCTL_PROCESS_UNPROTECT_BY_PID:
	{
		auto size = stack->Parameters.DeviceIoControl.InputBufferLength;

		if (size % sizeof(ULONG) != 0)
		{
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		auto data = (ULONG*)Irp->AssociatedIrp.SystemBuffer;

		AutoLock<FastMutex> locker(g_Data.Lock);

		for (size_t i = 0; i < size / sizeof(ULONG); i++)
		{
			auto pid = data[i];
			if (pid == 0)
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (!RemoveProcess(pid))
				continue;

			len += sizeof(ULONG);

			if (g_Data.PidsCount == 0)
				break;
		}

		break;
	}
	case IOCTL_PROCESS_PROTECT_CLEAR:
	{
		AutoLock<FastMutex> locker(g_Data.Lock);
		::memset(&g_Data.Pids, 0, sizeof(g_Data.Pids));
		g_Data.PidsCount = 0;
		break;
	}
	case IOCTL_PROCESS_QUERY_PIDS:
	{
		auto size = stack->Parameters.DeviceIoControl.OutputBufferLength;

		if (g_Data.PidsCount == 0)
			break;

		if (size < (g_Data.PidsCount * sizeof(ULONG)))
		{
			status = STATUS_BUFFER_TOO_SMALL;

			if (size == sizeof(ULONG))
			{
				ULONG pid_array_size = (g_Data.PidsCount * sizeof(ULONG));
				len = sizeof(ULONG);
				auto data = (ULONG*)Irp->AssociatedIrp.SystemBuffer;
				*data = pid_array_size;
				status = STATUS_SUCCESS;
			}
			
			
			break;
		}

		if (size % sizeof(ULONG) != 0)
		{
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		auto data = (ULONG*)Irp->AssociatedIrp.SystemBuffer;
		
		AutoLock<FastMutex> locker(g_Data.Lock);

		for (size_t i = 0; i < size / sizeof(ULONG); i++)
		{
			if (index == g_Data.PidsCount) // if the value is the same then break
				break;

			data[i] = g_Data.Pids[index++];
			len += sizeof(ULONG);
		}

		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	// complete the request
	Irp->IoStatus.Status		= status;
	Irp->IoStatus.Information	= len;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}


bool AddProcess(ULONG pid)
{
	for (size_t i = 0; i < MaxPids; i++)
	{
		if (g_Data.Pids[i] == 0)
		{
			// empty slot
			g_Data.Pids[i] = pid;
			g_Data.PidsCount++;
			return true;
		}
	}

	return false;
}


bool RemoveProcess(ULONG pid)
{
	for (size_t i = 0; i < MaxPids; i++)
	{
		if (g_Data.Pids[i] == pid)
		{
			g_Data.Pids[i] = 0;
			g_Data.PidsCount--;
			return true;
		}
	}

	return false;
}


bool FindProcess(ULONG pid)
{
	for (size_t i = 0; i < MaxPids; i++)
	{
		if (g_Data.Pids[i] == pid)
		{
			return true;
		}
	}

	return false;
}


OB_PREOP_CALLBACK_STATUS OnPreOpenProcess(PVOID, POB_PRE_OPERATION_INFORMATION Info)
{
	if (Info->KernelHandle) // if it is a Kernel Handle, everything okay
		return OB_PREOP_SUCCESS;

	auto process = (PEPROCESS)Info->Object;
	auto pid = HandleToUlong(PsGetProcessId(process));

	KdPrint((DRIVER_PREFIX "handling pre open process for PID: %X(%d)\n", pid, pid));

	AutoLock<FastMutex> locker(g_Data.Lock);

	if (FindProcess(pid))
	{
		// fond in list, remove terminate access
		Info->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
	}

	return OB_PREOP_SUCCESS;
}
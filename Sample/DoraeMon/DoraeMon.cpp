#include "pch.h"

// Headers
#include "DoraeMonCommon.h"
#include "DoraeMon.h"
#include "AutoLock.h"

// PROTOTYPES
void DoraeMonUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS DoraeMonCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp);
NTSTATUS DoraeMonRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp);
void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo);
void OnThreadNotify(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create);
void OnLoadImageNotify(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo);
NTSTATUS OnRegistryNotify(PVOID context, PVOID arg1, PVOID arg2);
void PushItem(LIST_ENTRY* entry);

// Globals

Globals g_Globals;
static const WCHAR machine[] = L"\\REGISTRY\\MACHINE\\";

extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	auto status = STATUS_SUCCESS;

	// Initialize linked list head
	InitializeListHead(&g_Globals.ItemsHead);
	// Initialize Fastmutex
	g_Globals.Mutex.Init();

	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMBOLIC_NAME);
	bool symLinkCreated = false;

	do
	{
		UNICODE_STRING devName = RTL_CONSTANT_STRING(DEVICE_NAME);
		// Create the device
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, TRUE, &DeviceObject);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
			break;
		}
		// set way for securing user mode input buffers
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
			KdPrint((DRIVER_PREFIX "failed to create sym link (0x%08X)\n", status));
			break;
		}
		symLinkCreated = true;

		// register for notifications of each process
		// we will use PsSetCreateProcessNotifyRoutineEx
		// but we could use PsSetCreateProcessNotifyRoutineEx2
		// to get from Linux processes
		status = PsSetCreateProcessNotifyRoutineEx(&OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to register process callback (0x%08X)\n", status));
			break;
		}

		// register for notifications of each thread
		status = PsSetCreateThreadNotifyRoutine(&OnThreadNotify);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to set thread callbacks (0x%08X)\n", status));
			break;
		}

		status = PsSetLoadImageNotifyRoutine(&OnLoadImageNotify);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to set image load notify callbacks (0x%08X)\n", status));
			break;
		}


		// Callback for registers operations
		UNICODE_STRING altitude = RTL_CONSTANT_STRING(L"7657.4846");
		status = CmRegisterCallbackEx(OnRegistryNotify, &altitude, DriverObject, nullptr, &g_Globals.RegCookie, nullptr);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DRIVER_PREFIX "failed to set registry callback (%08X)\n", status));
			break;
		}

		break;
	} while (false);

	if (!NT_SUCCESS(status))
	{
		if (symLinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
	}

	DriverObject->DriverUnload = DoraeMonUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = DoraeMonCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = DoraeMonRead;

	return status;
}


NTSTATUS DoraeMonCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


void PushItem(LIST_ENTRY* entry)
{
	// Function to modify the process list
	// this must have a mutex to avoid thread
	// problems
	AutoLock<FastMutex> lock(g_Globals.Mutex); // till now to the end of the function we will have Mutex
	                                           // and will be freed on destructor at the end of the function

	if (g_Globals.ItemCount > 1024)
	{
		// big problem, remove oldest file from FIFO (So First one)
		// first remove header from list
		auto head = RemoveHeadList(&g_Globals.ItemsHead);
		g_Globals.ItemCount--;
		// we use the macro CONTAINING_RECORD to get the item
		// from the LINKED_LIST structure
		auto item = CONTAINING_RECORD(head, FullItem<ItemHeader>, Entry);
		ExFreePool(item);
	}
	InsertTailList(&g_Globals.ItemsHead, entry);
	g_Globals.ItemCount++;

}


NTSTATUS DoraeMonRead(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP  Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;

	auto status = STATUS_SUCCESS;
	auto count = 0;
	NT_ASSERT(Irp->MdlAddress); // as we are using Direct I/O we will use Mdl

	auto buffer = (UCHAR*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

	if (!buffer)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		AutoLock<FastMutex> lock(g_Globals.Mutex);

		while (true)
		{
			if (g_Globals.ItemCount == 0)
				break;

			// get first entry
			auto entry = RemoveHeadList(&g_Globals.ItemsHead);
			auto info = CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry);
			auto size = info->Data.size;
			if (len < size)
			{
				// no more space, insert item back to List by the tail
				InsertHeadList(&g_Globals.ItemsHead, entry);
				break;
			}
			g_Globals.ItemCount--;
			memcpy(buffer, &info->Data, size);

			len -= size;
			buffer += size;
			count += size;

			// once data is copied to user buffer, free our pool
			ExFreePool(info);
		}
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = count;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}


void DoraeMonUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	// unregister process, thread and image load notifications
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
	PsRemoveCreateThreadNotifyRoutine(&OnThreadNotify);
	PsRemoveLoadImageNotifyRoutine(&OnLoadImageNotify);
	CmUnRegisterCallback(g_Globals.RegCookie);

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMBOLIC_NAME);
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(DriverObject->DeviceObject);

	while (!IsListEmpty(&g_Globals.ItemsHead))
	{
		auto entry = RemoveHeadList(&g_Globals.ItemsHead);
		ExFreePool(CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry));
	}
	g_Globals.ItemCount = 0;

	return;
}


void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	UNREFERENCED_PARAMETER(Process);

	if (CreateInfo)
	{
		// process create
		USHORT allocSize = sizeof(FullItem<ProcessCreateInfo>);
		USHORT commandLineSize = 0;
		USHORT imageFileNameSize = 0;
		USHORT offset = 0;
		if (CreateInfo->CommandLine)
		{
			// if we have command line, increase the size to allocate
			// to copy command line to the pool
			commandLineSize = CreateInfo->CommandLine->Length;
			allocSize += commandLineSize;
		}

		if (CreateInfo->FileOpenNameAvailable && CreateInfo->ImageFileName)
		{
			imageFileNameSize = CreateInfo->ImageFileName->Length;
			allocSize += imageFileNameSize;
		}

		auto info = (FullItem<ProcessCreateInfo>*)ExAllocatePoolWithTag(PagedPool, allocSize, DRIVER_TAG);
		if (info == nullptr)
		{
			KdPrint((DRIVER_PREFIX "failed allocation\n"));
			return;
		}
		auto& item = info->Data;
#if (NTDDI_VERSION >= NTDDI_WIN8)
		KeQuerySystemTimePrecise(&item.Time);
#else
		KeQuerySystemTime(&item.Time);
#endif // WIN8_OR_UPPER
		item.Type = ItemType::ProcessCreate;
		item.size = sizeof(ProcessCreateInfo) + commandLineSize + imageFileNameSize;
		item.ProcessId = HandleToULong(ProcessId);
		item.ParentProcessId = HandleToULong(CreateInfo->ParentProcessId);

		offset = sizeof(item);
		if (commandLineSize > 0)
		{
			memcpy((UCHAR*)&item + offset, CreateInfo->CommandLine->Buffer, commandLineSize);
			item.CommandLineLength = commandLineSize / sizeof(WCHAR);
			item.CommandLineOffset = offset;

			offset += commandLineSize;
		}
		else
		{
			item.CommandLineLength = 0;
			item.CommandLineOffset = 0;
		}


		if (imageFileNameSize > 0)
		{
			memcpy((UCHAR*)&item + offset, CreateInfo->ImageFileName->Buffer, imageFileNameSize);
			item.ImageFileNameLength = imageFileNameSize / sizeof(WCHAR);
			item.ImageFileNameOffset = offset;
		}
		else
		{
			item.ImageFileNameLength = 0;
			item.ImageFileNameOffset = 0;
		}

		PushItem(&info->Entry);
	}
	else
	{
		// process exit
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePoolWithTag(PagedPool, sizeof(FullItem<ProcessExitInfo>), DRIVER_TAG);
		if (info == nullptr)
		{
			KdPrint((DRIVER_PREFIX "failed allocation\n"));
			return;
		}

		auto& item = info->Data;
#if (NTDDI_VERSION >= NTDDI_WIN8)
		KeQuerySystemTimePrecise(&item.Time);
#else
		KeQuerySystemTime(&item.Time);
#endif // WIN8_OR_UPPER


		item.Type = ItemType::ProcessExit;
		item.ProcessId = HandleToULong(ProcessId);
		item.size = sizeof(ProcessExitInfo);

		PushItem(&info->Entry);
	}
}


void OnThreadNotify(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create)
{
	auto size = sizeof(FullItem<ThreadCreateExitInfo>);
	auto info = (FullItem<ThreadCreateExitInfo>*)ExAllocatePoolWithTag
	(
		PagedPool,
		size,
		DRIVER_TAG
	);

	if (info == nullptr)
	{
		KdPrint((DRIVER_PREFIX "Failed to allocate memory\n"));
		return;
	}

	auto &item = info->Data;
#if (NTDDI_VERSION >= NTDDI_WIN8)
	KeQuerySystemTimePrecise(&item.Time);
#else
	KeQuerySystemTime(&item.Time);
#endif // WIN8_OR_UPPER

	item.size = sizeof(item);
	item.Type = Create ? ItemType::ThreadCreate : ItemType::ThreadExit;
	item.ProcessId = HandleToULong(ProcessId);
	item.ThreadId = HandleToULong(ThreadId);

	PushItem(&info->Entry);
}


void OnLoadImageNotify(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo)
{
	NTSTATUS status = STATUS_SUCCESS;
	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	USHORT fileNameSize = 0;
	USHORT allocSize = sizeof(FullItem<ImageLoadedInfo>);

	if (ImageInfo->SystemModeImage || HandleToUlong(ProcessId) == 0)
		return;

	// check if there's extended info
	if (ImageInfo->ExtendedInfoPresent)
	{
		auto exinfo = CONTAINING_RECORD(ImageInfo, IMAGE_INFO_EX, ImageInfo);

		status = FltGetFileNameInformationUnsafe(exinfo->FileObject, NULL, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
	}
	
	if (nameInfo && NT_SUCCESS(status))
	{
		fileNameSize = nameInfo->Name.Length;
		allocSize += nameInfo->Name.Length;
	}
	else
	{
		if (FullImageName)
		{
			fileNameSize = FullImageName->Length;
			allocSize += FullImageName->Length;
		}
	}

	auto info = (FullItem<ImageLoadedInfo>*)ExAllocatePoolWithTag(PagedPool, allocSize, DRIVER_TAG);
	if (info == nullptr)
	{
		KdPrint((DRIVER_PREFIX "failed allocation\n"));
		return;
	}
	
	// generic information
	auto& item = info->Data;
#if (NTDDI_VERSION >= NTDDI_WIN8)
	KeQuerySystemTimePrecise(&item.Time);
#else
	KeQuerySystemTime(&item.Time);
#endif // WIN8_OR_UPPER
	item.Type = ItemType::ImageLoaded;
	item.size = (USHORT)sizeof(ImageLoadedInfo) + fileNameSize;
	
	// information about image loading
	item.ImageBase = ImageInfo->ImageBase;
	item.ProcessId = HandleToULong(ProcessId);

	if (fileNameSize)
	{
		USHORT offset = sizeof(item);

		item.FullImageNameOffset = offset;
		item.FullImageNameLength = fileNameSize / sizeof(WCHAR);
		if (nameInfo)
		{
			memcpy((UCHAR*)&item + offset, nameInfo->Name.Buffer, nameInfo->Name.Length);
			FltReleaseFileNameInformation(nameInfo);
		}
		else
		{
			memcpy((UCHAR*)&item + offset, FullImageName->Buffer, nameInfo->Name.Length);
		}
	}
	else
	{
		item.FullImageNameLength = 0;
		item.FullImageNameOffset = 0;
	}

	PushItem(&info->Entry);
}


NTSTATUS OnRegistryNotify(PVOID context, PVOID arg1, PVOID arg2)
{
	UNREFERENCED_PARAMETER(context);

	switch ((REG_NOTIFY_CLASS)(ULONG_PTR)arg1)
	{
	case RegNtPostSetValueKey:
	{
		auto args = (REG_POST_OPERATION_INFORMATION*)arg2;
		if (!NT_SUCCESS(args->Status)) // check status of operation was correct
			break;

		PCUNICODE_STRING name;

#if (NTDDI_VERSION >= NTDDI_WIN8)
		if (NT_SUCCESS(CmCallbackGetKeyObjectIDEx(&g_Globals.RegCookie, args->Object, nullptr, &name, 0)))
		{
#else
		if (NT_SUCCESS(CmCallbackGetKeyObjectID(&g_Globals.RegCookie, args->Object, nullptr, &name)))
		{
#endif
			// filter out those are not HKLM writes
			if (::wcsncmp(name->Buffer, machine, ARRAYSIZE(machine) - 1) == 0)
			{
				auto preInfo = (REG_SET_VALUE_KEY_INFORMATION*)args->PreInformation;
				NT_ASSERT(preInfo);

				auto size = sizeof(FullItem<RegistrySetValueInfo>);
				auto info = (FullItem<RegistrySetValueInfo>*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);

				if (info == nullptr)
					break;

				// zero out structure to make sure strings are null-terminated when copied
				RtlZeroMemory(info, size);

				// fill standard data
				auto& item = info->Data;
#if (NTDDI_VERSION >= NTDDI_WIN8)
				KeQuerySystemTimePrecise(&item.Time);
#else
				KeQuerySystemTime(&item.Time);
#endif // WIN8_OR_UPPER
				item.size = sizeof(item);
				item.Type = ItemType::RegistrySetValue;

				// get client PID/TID (this is our caller)
				item.ProcessId = HandleToULong(PsGetCurrentProcessId());
				item.ThreadId = HandleToULong(PsGetCurrentThread());

				// get specific key/value data
				::wcsncpy_s(item.KeyName, name->Buffer, name->Length / sizeof(WCHAR) - 1);
				::wcsncpy_s(item.ValueName, preInfo->ValueName->Buffer, preInfo->ValueName->Length / sizeof(WCHAR) - 1);
				item.DataType = preInfo->Type;
				item.DataSize = preInfo->DataSize;
				::memcpy(item.Data, preInfo->Data, min(item.DataSize, sizeof(item.Data)));

				PushItem(&info->Entry);
			}
#if (NTDDI_VERSION >= NTDDI_WIN8)
			CmCallbackReleaseKeyObjectIDEx(name);
#endif
		}
	}
	default:
		break;
	}

	return STATUS_SUCCESS;
}
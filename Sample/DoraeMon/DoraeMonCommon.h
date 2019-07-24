#pragma once
#ifndef DORAEMONCOMMON_H
#define DORAEMONCOMMON_H

#define MAX_KEY_NAME 256
#define VALUE_NAME 64
#define MAX_DATA 128

// short enumeration for processes
enum class ItemType : short
{
	None,
	ProcessCreate,
	ProcessExit,
	ThreadCreate,
	ThreadExit,
	ImageLoaded,
	RegistrySetValue
};

// Main Base struct
struct ItemHeader
{
	ItemType Type;
	USHORT size;
	LARGE_INTEGER Time;
};

struct ProcessExitInfo : ItemHeader
{
	ULONG ProcessId;
};


struct ProcessCreateInfo : ItemHeader
{
	ULONG ProcessId;
	ULONG ParentProcessId;
	USHORT CommandLineLength;
	USHORT CommandLineOffset;
	USHORT ImageFileNameLength;
	USHORT ImageFileNameOffset;
};


struct ThreadCreateExitInfo : ItemHeader
{
	ULONG ThreadId;
	ULONG ProcessId;
};


struct ImageLoadedInfo : ItemHeader
{
	ULONG ProcessId;
	PVOID ImageBase;
	USHORT FullImageNameLength;
	USHORT FullImageNameOffset;
};


struct RegistrySetValueInfo : ItemHeader
{
	ULONG ProcessId;
	ULONG ThreadId;
	WCHAR KeyName[MAX_KEY_NAME];	// full key name
	WCHAR ValueName[VALUE_NAME];	// value name
	ULONG DataType;					// REG_xxx
	UCHAR Data[MAX_DATA];			// data
	ULONG DataSize;					// size of data
};

#endif // !DORAEMONCOMMON_H

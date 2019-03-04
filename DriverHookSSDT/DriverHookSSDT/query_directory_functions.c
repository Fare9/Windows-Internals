#include "query_directory_structs.h"


/*
*	Function to get the name of a file from any
*	of the structures from query_directory_struct.h
*	So this function will be a generic way to get
*	the name of a file.
*/
PVOID get_dir_entry_filename(IN PVOID FileInformation, IN FILE_INFORMATION_CLASS FileInfoClass)
{
	PVOID result = 0;
	switch (FileInfoClass)
	{
	case FileDirectoryInformation:
		result = (PVOID)&((PFILE_DIRECTORY_INFORMATION)FileInformation)->FileName;
		break;
	case FileFullDirectoryInformation:
		result = (PVOID)&((PFILE_FULL_DIR_INFORMATION)FileInformation)->FileName;
		break;
	case FileIdFullDirectoryInformation:
		result = (PVOID)&((PFILE_ID_FULL_DIR_INFORMATION)FileInformation)->FileName;
		break;
	case FileIdBothDirectoryInformation:
		result = (PVOID)&((PFILE_ID_BOTH_DIR_INFORMATION)FileInformation)->FileName;
		break;
	case FileNamesInformation:
		result = (PVOID)&((PFILE_NAMES_INFORMATION)FileInformation)->FileName;
		break;
	default:
		break;
	}
	return result;
}

/*
*	Function to get the next offset from a file
*	we use any of the structures, this offset
*	will be necessary to remove the file from
*	the list later.
*/
ULONG get_next_entry_offset(IN PVOID FileInformation, IN FILE_INFORMATION_CLASS FileInfoClass)
{
	ULONG result = 0;
	switch (FileInfoClass)
	{
	case FileDirectoryInformation:
		result = (ULONG)((PFILE_DIRECTORY_INFORMATION)FileInformation)->NextEntryOffset;
		break;
	case FileFullDirectoryInformation:
		result = (ULONG)((PFILE_FULL_DIR_INFORMATION)FileInformation)->NextEntryOffset;
		break;
	case FileIdFullDirectoryInformation:
		result = (ULONG)((PFILE_ID_FULL_DIR_INFORMATION)FileInformation)->NextEntryOffset;
		break;
	case FileIdBothDirectoryInformation:
		result = (ULONG)((PFILE_ID_BOTH_DIR_INFORMATION)FileInformation)->NextEntryOffset;
		break;
	case FileNamesInformation:
		result = (ULONG)((PFILE_NAMES_INFORMATION)FileInformation)->NextEntryOffset;
		break;
	default:
		break;
	}
	return result;
}


void set_next_entry_offset(IN PVOID FileInformation, IN FILE_INFORMATION_CLASS FileInfoClass, IN ULONG newValue)
{
	switch (FileInfoClass)
	{
	case FileDirectoryInformation:
		((PFILE_DIRECTORY_INFORMATION)FileInformation)->NextEntryOffset = newValue;
		break;
	case FileFullDirectoryInformation:
		((PFILE_FULL_DIR_INFORMATION)FileInformation)->NextEntryOffset = newValue;
		break;
	case FileIdFullDirectoryInformation:
		((PFILE_ID_FULL_DIR_INFORMATION)FileInformation)->NextEntryOffset = newValue;
		break;
	case FileIdBothDirectoryInformation:
		((PFILE_ID_BOTH_DIR_INFORMATION)FileInformation)->NextEntryOffset = newValue;
		break;
	case FileNamesInformation:
		((PFILE_NAMES_INFORMATION)FileInformation)->NextEntryOffset = newValue;
		break;
	default:
		break;
	}
}
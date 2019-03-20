#pragma once

#ifndef FILTERS_H
#define FILTERS_H

#include "common.h"
#include "structures.h"

/*
*	This function using PsSetImageLoadNotifyRoutine 
*	will be called each time that a process is loaded
*	this kind of function has three parameters:
*		+ FullImageName: path of loaded image in unicode_string. 
*		we will use this parameter to filter those processes we
*		want to interact with.
*		+ ProcessId: handle with the PID of the loaded image.
*		+ ImageInfo: a structure of the type IMAGE_INFO.
		typedef struct _IMAGE_INFO {
			union {
				ULONG Properties;
				struct {
					ULONG ImageAddressingMode  : 8;  // Code addressing mode
					ULONG SystemModeImage      : 1;  // System mode image
					ULONG ImageMappedToAllPids : 1;  // Image mapped into all processes
					ULONG ExtendedInfoPresent  : 1;  // IMAGE_INFO_EX available
					ULONG MachineTypeMismatch  : 1;  // Architecture type mismatch
					ULONG ImageSignatureLevel  : 4;  // Signature level
					ULONG ImageSignatureType   : 3;  // Signature type
					ULONG ImagePartialMap      : 1;  // Nonzero if entire image is not mapped
					ULONG Reserved             : 12;
				};
			};
			PVOID       ImageBase;
			ULONG       ImageSelector;
			SIZE_T      ImageSize;
			ULONG       ImageSectionNumber;
		} IMAGE_INFO, *PIMAGE_INFO;
*/
VOID NotifyDriverOnLoad(IN PUNICODE_STRING FullImageName, IN HANDLE ProcessId, IN PIMAGE_INFO ImageInfo);
/*
*	Function to hook imports from the process we
*	selected on the  previous function, we will 
*	have to parse the PE structure to get into 
*	the imports and modify the function we want.
*/
NTSTATUS HookImportsOfImage(PIMAGE_DOS_HEADER image_addr, HANDLE h_proc);


#endif // !FILTERS_H

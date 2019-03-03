#pragma once

#ifndef SSDT_STRUCTURES_H
#define SSDT_STRUCTURES_H

#include "common.h"
#include "user_defined_types.h"

/*
*	Useful structures to manage the SSDT
*	these represent different structures
*	from the OS
*/


/*
*	Structure which emulate the KeServiceDescriptorTable
*	this structure contains a pointer to the SSDT base,
*	also a pointer to the SSPT base, pointer to service
*	counter table and the number of services.
*/
#pragma pack(1)
typedef struct ServiceDescriptorEntry 
{
	PUINT ServiceTableBase;
	PUINT ServiceCounterTableBase;
	UINT NumberOfServices;
	PUCHAR ParamTableBase;
} SSDT_Entry, *PSSDT_Entry;
#pragma pack()

#endif // !SSDT_STRUCTURES_H

#pragma once
#ifndef ZERO_COMMON_H
#define ZERO_COMMON_H

typedef struct _NumberOfBytes_
{
	ULONGLONG nBytesRead;
	ULONGLONG nBytesWrite;
} NumberOfBytes, *PNumberOfBytes;


#define ZERO_COUNTER_DEVICE 0x8000
#define IOCTL_ZERO_COUNT_BYTES CTL_CODE(ZERO_COUNTER_DEVICE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)


#endif // !ZERO_COMMON_H

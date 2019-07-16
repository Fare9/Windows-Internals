#include "pch.h"
#include "DriverCommunication.h"
#include "ScreenPrinter.h"

int main()
{
	PrintHeader();

	Sleep(3000);

	if (!OpenCommunicationWithDriver(L"\\\\.\\DoraeMon"))
	{
		return 1;
	}



	while (true)
	{
		if (!Monitorize())
		{
			CloseCommunicationWithDriver();
			return 1;
		}
	}
	return 0;
}


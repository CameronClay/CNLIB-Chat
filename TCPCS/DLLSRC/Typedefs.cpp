#include "Typedefs.h"

void WaitAndCloseHandle(HANDLE& hnd)
{
	if(hnd)
	{
		WaitForSingleObject(hnd, INFINITE);
		CloseHandle(hnd);
		hnd = NULL;
	}
}
#include "Typedefs.h"

void WaitAndCloseHandle(HANDLE& hnd)
{
	WaitForSingleObject(hnd, INFINITE);
	CloseHandle(hnd);
	hnd = NULL;
}
#include "StdAfx.h"
#include "CNLIB/Typedefs.h"

void CAMSNETLIB WaitAndCloseHandle(HANDLE& hnd)
{
	if(hnd)
	{
		WaitForSingleObject(hnd, INFINITE);
		CloseHandle(hnd);
		hnd = NULL;
	}
}

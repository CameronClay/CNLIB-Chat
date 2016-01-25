#pragma once

/*
LPCTSTR LocationErrorOccured
Could be name of Function, Member::Function or any section of code that
programmer can identify to find bug

Uses GetLastError()

If there was an error a message is displayed and program terminates after
clicking OK.
*/
inline BOOL CheckForError(LPCTSTR LocationErrorOccured)
{
	DWORD err = GetLastError();

	if (err > 0)
	{
		LIB_TCHAR errMsg[128] = {};
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, NULL, errMsg, 128, nullptr);
		MessageBox(NULL, errMsg, LocationErrorOccured, MB_OK);
		PostQuitMessage(err);
	}

	return ERROR_SUCCESS;
}

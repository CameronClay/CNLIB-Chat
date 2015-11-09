#include "VerifyPing.h"

DWORD CALLBACK Verify(LPVOID info)
{
	typedef VerifyPing::VerifyData VerifyData;
	VerifyData& data = *(VerifyData*)info;

	HANDLE timer = data.verify->GetTimer();

	while (true)
	{
		const DWORD ret = MsgWaitForMultipleObjects(1, &timer, FALSE, INFINITE, QS_ALLINPUT);
		if (ret == WAIT_OBJECT_0)
			data.client.GetHost().Disconnect();

		else if (ret == WAIT_OBJECT_0 + 1)
		{
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
			{
				if (msg.message == WM_QUIT)
					return 0;
			}
		}

		else
			return 0;
	}

	return 0;
}

VerifyPing::VerifyPing(TCPClient& client)
	:
	timer(CreateWaitableTimer(NULL, FALSE, NULL)),
	thread(NULL),
	threadID(NULL),
	verifyData(construct<VerifyData>(client, this))
{}

VerifyPing::VerifyPing(VerifyPing&& verifyPing)
	:
	timer(verifyPing.timer),
	thread(verifyPing.thread),
	threadID(verifyPing.threadID),
	verifyData(verifyPing.verifyData)
{
	verifyData->verify = this;
	ZeroMemory(&verifyPing, sizeof(VerifyPing));
}

VerifyPing::~VerifyPing()
{
	if (timer)
	{
		CancelWaitableTimer(timer);
		CloseHandle(timer);
		timer = NULL;
	}

	if (thread)
	{
		PostThreadMessage(threadID, WM_QUIT, 0, 0);
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		thread = NULL;
	}

	destruct(verifyData);
}


void VerifyPing::SetTimer(float interval)
{
	LARGE_INTEGER LI;
	LI.QuadPart = (LONGLONG)(interval * -10000000.0f);
	SetWaitableTimer(timer, &LI, (LONG)(interval * 1000.0f), NULL, NULL, FALSE);
	if (!thread)
		thread = CreateThread(NULL, 0, Verify, verifyData, NULL, &threadID);
}


HANDLE VerifyPing::GetTimer() const
{
	return timer;
}

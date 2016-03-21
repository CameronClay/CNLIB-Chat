#pragma once

class RecvObserverI
{
public:
	virtual void OnNotify(char*, DWORD, void*) = 0;
};
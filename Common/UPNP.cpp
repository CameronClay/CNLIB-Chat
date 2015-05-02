#include "Socket.h"
#include "UPNP.h"
#include <assert.h>
#include <comdef.h>

UPNP::UPNP()
	:
	spmc(nullptr)
{
	CoInitialize(NULL);
}

UPNP::~UPNP()
{
	if (spmc != nullptr)
	{
		spmc->Release();
		spmc = nullptr;
	}
	CoUninitialize();
}

bool UPNP::Initialize()
{
	bool upnpS = false, collectionS = false;
	IUPnPNAT *upnp = nullptr;
	if ((upnpS = SUCCEEDED(CoCreateInstance(CLSID_UPnPNAT, NULL, CLSCTX_INPROC_SERVER, IID_IUPnPNAT, (void**)&upnp)) && upnp != nullptr))
		collectionS = SUCCEEDED(upnp->get_StaticPortMappingCollection(&spmc)) && spmc != nullptr;
	if (upnpS)
	{
		upnp->Release();
		upnp = nullptr;
	}
	return upnpS && collectionS;
}

bool UPNP::PortMapExists(USHORT port, const TCHAR* protocal) const
{
	IStaticPortMapping* spm = nullptr;
	bool result = false;
	if ((result = SUCCEEDED(spmc->get_Item(port, bstr_t(protocal), &spm)) && spm != nullptr))
	{
		spm->Release();
		spm = nullptr;
	}
	return result;
}

bool UPNP::RemovePortMap(USHORT port, const TCHAR* protocal)
{
	return SUCCEEDED(spmc->Remove(port, bstr_t(protocal)));
}

bool UPNP::AddPortMap(USHORT port, const TCHAR* protocal, const TCHAR* ip, const TCHAR* description, bool state)
{
	IStaticPortMapping* spm = nullptr;
	VARIANT_BOOL b = (state ? VARIANT_TRUE : VARIANT_FALSE);
	bool result = false;
	if ((result = SUCCEEDED(spmc->Add(port, bstr_t(protocal), port, bstr_t(ip), b, bstr_t(description), &spm)) && spm != nullptr))
	{
		spm->Release();
		spm = nullptr;
	}
	return result;
}

long UPNP::GetPortMapCount()
{
	long n = 0;
	if (!SUCCEEDED(spmc->get_Count(&n)))
		n = 0;
	return n;
}

void MapPort(USHORT port, const TCHAR* protocal, const TCHAR* name)
{
	UPNP upnp;
	bool b = false;
	if (upnp.Initialize())
	{
		TCHAR IP[16] = {};
		Socket::GetLocalIP(IP);
		_tprintf(_T("Your IP: %s\n"), IP);
		if (upnp.PortMapExists(port, protocal))
			b = upnp.RemovePortMap(port, protocal);

		b = upnp.AddPortMap(port, protocal, IP, name, true);
	}
	if (b)
		_tprintf(_T("Port %d mapped\n"), port);
	else
		_tprintf(_T("Port mapping failed( make sure upnp is enabled on your router and port is not already forwarded/triggered(if port is already triggered/forwarded its fine) )\n"));
}
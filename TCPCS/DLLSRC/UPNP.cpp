#include "Socket.h"
#include "UPNP.h"
#include <assert.h>
#include <comdef.h>

UPNP::UPNP()
	:
	spmc(nullptr)
{}

UPNP::~UPNP()
{
	if (spmc != nullptr)
	{
		spmc->Release();
		spmc = nullptr;
	}
}

bool UPNP::Initialize()
{
	bool upnpS = false, collectionS = false;
	IUPnPNAT *upnp = nullptr;
	if (upnpS = (SUCCEEDED(CoCreateInstance(CLSID_UPnPNAT, NULL, CLSCTX_INPROC_SERVER, IID_IUPnPNAT, (void**)&upnp)) && (upnp != nullptr)))
		collectionS = (SUCCEEDED(upnp->get_StaticPortMappingCollection(&spmc)) && (spmc != nullptr));

	if (upnpS)
	{
		upnp->Release();
		upnp = nullptr;
	}
	return upnpS && collectionS;
}

bool UPNP::PortMapExists(USHORT port, const LIB_TCHAR* protocal) const
{
	IStaticPortMapping* spm = nullptr;
	bool result = false;
	if (result = (SUCCEEDED(spmc->get_Item(port, bstr_t(protocal), &spm)) && spm != nullptr))
	{
		spm->Release();
		spm = nullptr;
	}
	return result;
}

bool UPNP::RemovePortMap(USHORT port, const LIB_TCHAR* protocal)
{
	return SUCCEEDED(spmc->Remove(port, bstr_t(protocal)));
}

bool UPNP::AddPortMap(USHORT port, const LIB_TCHAR* protocal, const LIB_TCHAR* ip, const LIB_TCHAR* description, bool state)
{
	IStaticPortMapping* spm = nullptr;
	VARIANT_BOOL b = (state ? VARIANT_TRUE : VARIANT_FALSE);
	bool result = false;
	if (result = (SUCCEEDED(spmc->Add(port, bstr_t(protocal), port, bstr_t(ip), b, bstr_t(description), &spm)) && spm != nullptr))
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

bool MapPort(USHORT port, const LIB_TCHAR* protocal, const LIB_TCHAR* name)
{
	UPNP upnp;
	bool result = false;

	if (upnp.Initialize())
	{
		LIB_TCHAR IP[16] = {};
		Socket::GetLocalIP(IP);
		if (upnp.PortMapExists(port, protocal))
			result = upnp.RemovePortMap(port, protocal);

		result = upnp.AddPortMap(port, protocal, IP, name, true);
	}
	return result;
}
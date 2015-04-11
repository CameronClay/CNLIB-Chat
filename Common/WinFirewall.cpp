#include "WinFirewall.h"
#include <comdef.h>

WinFirewall::WinFirewall()
:
	nfrs(nullptr),
	curProfs(0)
{
	CoInitialize(NULL);
}

WinFirewall::~WinFirewall()
{
	if (nfrs != nullptr)
	{
		nfrs->Release();
		nfrs = nullptr;
	}
	CoUninitialize();
}

bool WinFirewall::Initialize()
{
	INetFwPolicy2 *nfp = nullptr;
	bool result = false;
	if ((result = SUCCEEDED(CoCreateInstance(CLSID_NetFwPolicy2, NULL, CLSCTX_INPROC_SERVER, IID_INetFwPolicy2, (void**)&nfp)) && nfp != nullptr))
	{
		nfp->get_Rules(&nfrs);
		nfp->get_CurrentProfileTypes(&curProfs);
		if ((curProfs & NET_FW_PROFILE2_PUBLIC) && (curProfs != NET_FW_PROFILE2_PUBLIC))
		{
			curProfs ^= NET_FW_PROFILE2_PUBLIC;
		}
		nfp->Release();
		nfp = nullptr;
	}
	return result;
}

bool WinFirewall::AddException(const TCHAR* exceptionName, const TCHAR* description, const TCHAR* appName, const TCHAR* serviceName, NET_FW_IP_PROTOCOL protocal, const TCHAR* port, NET_FW_ACTION action, bool enabled)
{
	INetFwRule *nfr = nullptr;
	bool result = false;
	if ((result = SUCCEEDED(CoCreateInstance(CLSID_NetFwRule, NULL, CLSCTX_INPROC_SERVER, IID_INetFwRule, (void**)&nfr)) && nfr != nullptr))
	{
		nfr->put_Name(bstr_t(exceptionName));
		nfr->put_Description(bstr_t(description));
		nfr->put_ApplicationName(bstr_t(appName));
		nfr->put_ServiceName(bstr_t(serviceName));
		nfr->put_Protocol(protocal);
		nfr->put_LocalPorts(bstr_t(port));
		nfr->put_Profiles(curProfs);
		nfr->put_Action(action);
		nfr->put_Enabled((enabled ? VARIANT_TRUE : VARIANT_FALSE));
		if ((result = SUCCEEDED(nfrs->Add(nfr))))
		{
			nfr->Release();
			nfr = nullptr;
		}
	}
	return result;
}

bool WinFirewall::ItemExists(const TCHAR* name)
{
	INetFwRule *nfr = nullptr;
	bool result = false;
	if ((result = SUCCEEDED(nfrs->Item(bstr_t(name), &nfr)) && nfr != nullptr))
	{
		nfr->Release();
		nfr = nullptr;
	}
	return result;
}

bool WinFirewall::RemoveItem(const TCHAR* name)
{
	return SUCCEEDED(nfrs->Remove(bstr_t(name)));
}
#pragma once

#include <windows.h>
#include <wlanapi.h>
#pragma comment(lib, "wlanapi.lib")
#include <utils/u8string.hpp>
#include <utils/ResourceRelease.hpp>


namespace utils::windows::wifi
{
	//连接WiFi
	//note: 此函数会立刻返回，wifi是否连接成功，需要后续继续判断
	inline void connect(const std::u8string & ssid, const std::u8string & password)
	{
		utils::ResourceRelease<HANDLE> hClient{[](auto & hClient)
		{
			if (hClient)
			{
				utils_assert_abort(WlanCloseHandle(hClient, NULL) == ERROR_SUCCESS);
				hClient = NULL;
			}
		}};

		DWORD negotiatedVersion{};

		utils_assert(WlanOpenHandle(WLAN_API_VERSION, NULL, &negotiatedVersion, &hClient.get()) == ERROR_SUCCESS);
		
		utils::ResourceRelease<WLAN_INTERFACE_INFO_LIST*> interface_list{[](auto & interface_list)
		{
			if (interface_list)
			{
				WlanFreeMemory(interface_list);
				interface_list = nullptr;
			}
		}};

		utils_assert(WlanEnumInterfaces(hClient, NULL, &interface_list.get()) == ERROR_SUCCESS);

		utils_assert(interface_list->dwNumberOfItems != 0);

		const GUID interfaceGuid = interface_list->InterfaceInfo[0].InterfaceGuid;

		std::u8string profile_xml =
			u8"<?xml version=\"1.0\" encoding=\"US-ASCII\"?>"
			u8"<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">"
			u8"<name>" + ssid + u8"</name>"
			u8"<SSIDConfig>"
			u8"<SSID>"
			u8"<name>" + ssid + u8"</name>"
			u8"</SSID>"
			u8"</SSIDConfig>"
			u8"<connectionType>ESS</connectionType>"
			u8"<connectionMode>manual</connectionMode>"
			u8"<MSM>"
			u8"<security>"
			u8"<authEncryption>"
			u8"<authentication>WPA2PSK</authentication>"
			u8"<encryption>AES</encryption>"
			u8"<useOneX>false</useOneX>"
			u8"</authEncryption>"
			u8"<sharedKey>"
			u8"<keyType>passPhrase</keyType>"
			u8"<protected>false</protected>"
			u8"<keyMaterial>" + password + u8"</keyMaterial>"
			u8"</sharedKey>"
			u8"</security>"
			u8"</MSM>"
			u8"</WLANProfile>";

		auto profile_xml_u16 = utils::text::encoding::unicode::utf8_to_utf16(profile_xml);
		DWORD reason{};
		utils_assert(ERROR_SUCCESS == WlanSetProfile(hClient, &interfaceGuid, 0,
			(LPCWSTR)profile_xml_u16.c_str(), NULL, TRUE, NULL, &reason));
		
		auto ssid_u16 = utils::text::encoding::unicode::utf8_to_utf16(ssid);

		WLAN_CONNECTION_PARAMETERS connParams{};
		connParams.wlanConnectionMode = wlan_connection_mode_profile;
		connParams.strProfile = (LPCWSTR)ssid_u16.c_str();
		connParams.pDot11Ssid = NULL;
		connParams.pDesiredBssidList = NULL;
		connParams.dot11BssType = dot11_BSS_type_any;
		connParams.dwFlags = 0;

		utils_assert(WlanConnect(hClient, &interfaceGuid, &connParams, NULL) == ERROR_SUCCESS);
	}
}
#include "app.h"

#include "utils.h"
#include "resources.h"

#include <windows.h>
#include <format>

namespace settings::app {

constexpr const auto REG_PATH_APP = L"Software\\PowerTray";
constexpr const auto REG_KEY_MEDIACONTROL_ENABLED = L"EnableMediaKeyControl";
constexpr const auto REG_POWER = L"SYSTEM\\CurrentControlSet\\Control\\Power";
constexpr const auto REG_ENERGY_SAVER = L"EnergySaverState";


constexpr const auto REG_PATH_STARTUP = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

constexpr const auto REG_KEY_PSR_FEATURE = L"DalPSRFeatureEnable";
constexpr const auto REG_PATH_PSR_FEATURE = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000";


bool is_energy_saver_enabled(){
	HKEY key; 

	LONG result = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, REG_POWER, 0, KEY_READ, &key);
	if (result != ERROR_SUCCESS)
		return false;

	DWORD value = 0;
	DWORD size = sizeof(value);
	DWORD type = 0;

	result = ::RegQueryValueExW(key, REG_ENERGY_SAVER, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size);

	if (result != ERROR_SUCCESS || type != REG_DWORD)
		return false;

	return value == 1; // 1 is definitely on. Can be 2 which I've no idea what that means. 
}

bool is_mediakey_control_enabled(){
	DWORD disposition;
	HKEY key;

	LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH_APP, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, nullptr, &key, &disposition);

	if (result == ERROR_SUCCESS){
		if (disposition == REG_CREATED_NEW_KEY){
			auto value = 1;
			result = RegSetValueExW(key, REG_KEY_MEDIACONTROL_ENABLED, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));
			::RegCloseKey(key);
			return true;
		}
		else{
			DWORD value = 0;
			DWORD size = sizeof(value);
			DWORD type = 0;

			result = ::RegQueryValueExW(key, REG_KEY_MEDIACONTROL_ENABLED, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size);

			if (result != ERROR_SUCCESS || type != REG_DWORD)
				return false;

			return value != 0;
		}
	}
	::RegCloseKey(key);
	return false;
}

void set_mediakey_control(const bool &enabled){
	HKEY key;

	LONG result = ::RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH_APP, 0, KEY_WRITE, &key);
	if (result != ERROR_SUCCESS)
		return;

	auto value = enabled ? 1 : 0;
	::RegSetValueEx(key, REG_KEY_MEDIACONTROL_ENABLED, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&value), sizeof(DWORD));

	::RegCloseKey(key);
}

bool is_auto_start()
{
	HKEY key;

	LONG result = ::RegOpenKeyEx(HKEY_CURRENT_USER, REG_PATH_STARTUP, 0, KEY_READ, &key);
	if (result != ERROR_SUCCESS)
		return false;

	LPBYTE filename[MAX_PATH];
	DWORD len = sizeof(filename);
	auto app_name = utils::strings::to_wstring(APP_NAME);
	result = ::RegQueryValueEx(key, app_name.c_str(), nullptr, nullptr, (LPBYTE)filename, &len);

	::RegCloseKey(key);

	return result == ERROR_SUCCESS;
}

void set_auto_start(const bool &enabled)
{
	HKEY key;

	LONG result = ::RegOpenKeyEx(HKEY_CURRENT_USER, REG_PATH_STARTUP, 0, KEY_WRITE, &key);
	if (result != ERROR_SUCCESS)
		return;

	auto app_name = utils::strings::to_wstring(APP_NAME);

	if (enabled)
	{
		wchar_t filename[MAX_PATH];
		auto len = ::GetModuleFileName(nullptr, filename, sizeof(filename));
		::RegSetValueEx(key, app_name.c_str(), 0, REG_SZ, (const BYTE *)filename, len * sizeof(wchar_t));
	}
	else
	{
		::RegDeleteValue(key, app_name.c_str());
	}

	::RegCloseKey(key);
}

bool is_psr_enabled()
{
	HKEY key;

	LONG result = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, REG_PATH_PSR_FEATURE, 0, KEY_READ, &key);
	if (result != ERROR_SUCCESS)
		return false;

	DWORD value = 0;
	DWORD size = sizeof(value);
	DWORD type = 0;

	result = ::RegQueryValueExW(key, REG_KEY_PSR_FEATURE, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size);
	::RegCloseKey(key);

	if (result != ERROR_SUCCESS || type != REG_DWORD)
		return false;

	return value != 0;
}

void set_psr_enabled(const bool &enabled)
{
	HKEY key;

	LONG result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PATH_PSR_FEATURE, 0, KEY_WRITE, &key);
	if (result != ERROR_SUCCESS)
		return;

	auto value = enabled ? 1 : 0;
	::RegSetValueEx(key, REG_KEY_PSR_FEATURE, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&value), sizeof(DWORD));

	::RegCloseKey(key);
}

} // namespace settings::app

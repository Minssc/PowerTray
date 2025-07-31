#include "power.h"

#include "utils.h"

#include <powrprof.h>
#include <windows.h>

namespace api {

constexpr auto &DLL_NAME = "powrprof.dll";

const power::mode power::mode::MODE_BEST_PERFORMANCE(L"최고의 성능", {0xded574b5, 0x45a0, 0x4f42, {0x87, 0x37, 0x46, 0x34, 0x5c, 0x09, 0xc2, 0x38}});
const power::mode power::mode::MODE_BALANCE(L"균형", {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}});
const power::mode power::mode::MODE_BETTER_BATTERY(L"최고의 전력 효율", {0x961cc777, 0x2547, 0x4f9d, {0x81, 0x74, 0x7d, 0x86, 0x18, 0x1b, 0x8a, 0x7a}});

const std::vector<power::mode> power::mode::MODES{
	MODE_BEST_PERFORMANCE,
	MODE_BALANCE,
	MODE_BETTER_BATTERY,
};

void power::cycle_power_mode(){ // cycle the power mode to current + 1 according to MODES
	auto &current = power::get_power_mode();

	for (size_t i = 0; i < power::mode::MODES.size(); ++i){
		const auto &mode = power::mode::MODES[i];
		if (current.guid == mode.guid){
			power::apply_power_mode(power::mode::MODES[(i + 1)%power::mode::MODES.size()]);
			return;
		}
	}

	//send notification perhaps?
}

const power::mode &power::get_power_mode()
{
	auto func = utils::dll::get<DWORD(WINAPI *)(GUID *)>(DLL_NAME, "PowerGetEffectiveOverlayScheme");

	GUID guid;
	func(&guid);

	for (const auto &mode : power::mode::MODES)
	{
		if (mode.guid == guid)
			return mode;
	}

	return power::mode::MODE_BALANCE;
}

void power::apply_power_mode(const power::mode &mode)
{
	auto func = utils::dll::get<DWORD(WINAPI *)(GUID *)>(DLL_NAME, "PowerSetActiveOverlayScheme");

	GUID guid = mode.guid;
	func(&guid);
}

std::vector<power::profile> power::get_power_profiles()
{
	std::vector<power::profile> nodes;

	for (;;)
	{
		DWORD guid_buffer_size = 16;
		UCHAR guid_buffer[guid_buffer_size];

		DWORD result = ::PowerEnumerate(nullptr, nullptr, nullptr, ACCESS_SCHEME, nodes.size(), guid_buffer, &guid_buffer_size);
		if (result == ERROR_SUCCESS)
		{
			GUID guid;
			memcpy(&guid, guid_buffer, sizeof(GUID));

			DWORD length = 0;
			::PowerReadFriendlyName(nullptr, &guid, nullptr, nullptr, nullptr, &length);

			UCHAR name_buffer[length];
			::PowerReadFriendlyName(nullptr, &guid, nullptr, nullptr, name_buffer, &length);

			GUID *current_guid;
			::PowerGetActiveScheme(nullptr, &current_guid);

			nodes.push_back({
				utils::strings::to_wstring(name_buffer),
				guid,
				*current_guid == guid,
			});

			::LocalFree(current_guid);
			continue;
		}

		break;
	}

	return nodes;
}

void power::apply_power_profile(const power::profile &profile)
{
	::PowerSetActiveScheme(nullptr, &profile.guid);
}

} // namespace api

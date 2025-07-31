#ifndef API_POWER_H
#define API_POWER_H

#include <guiddef.h>

#include <string>
#include <vector>

namespace api::power {

struct mode
{
	static const mode MODE_BEST_PERFORMANCE;
	static const mode MODE_BALANCE;
	static const mode MODE_BETTER_BATTERY;

	static const std::vector<mode> MODES;

	std::wstring name;
	GUID guid;

	mode() = default;
	~mode() = default;

	mode(const std::wstring &name, const GUID &guid)
		: name(name), guid(guid) {}
};

struct profile
{
	std::wstring name;
	GUID guid;
	bool enabled;

	profile() = default;
	~profile() = default;

	profile(const std::wstring &name, const GUID &guid, const bool &enabled)
		: name(name), guid(guid), enabled(enabled) {}
};

} // namespace api::power

namespace api::power {

void cycle_power_mode();
const api::power::mode &get_power_mode();
void apply_power_mode(const api::power::mode &mode);

std::vector<profile> get_power_profiles();
void apply_power_profile(const api::power::profile &profile);

} // namespace api::power

#endif

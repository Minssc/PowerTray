#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

namespace settings::app {

bool is_mediakey_control_enabled();
void set_mediakey_control(const bool &enabled);

bool is_auto_start();
void set_auto_start(const bool &enabled);

bool is_psr_enabled();
void set_psr_enabled(const bool &enabled);

} // namespace settings::app

#endif

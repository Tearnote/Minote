// Minote - global/settings.h
// Loads and provides settings from various sources

#ifndef GLOBAL_SETTINGS_H
#define GLOBAL_SETTINGS_H

#include <stdbool.h>

enum settingLabel {
	SettingNone,
	SettingFullscreen,
	SettingNoSync,
	SettingSize
};

void initSettings(void);
void cleanupSettings(void);
void loadSwitchSettings(int argc, char *argv[]);

int getSettingInt(enum settingLabel label);
bool getSettingBool(enum settingLabel label);

#endif //GLOBAL_SETTINGS_H

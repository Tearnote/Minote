// Minote - settings.h
// Loads and provides settings from various sources

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

enum settingLabel {
	SettingNone,
	SettingFullscreen,
	SettingInitialState,
	SettingSize
};

void initSettings(void);
void cleanupSettings(void);
void loadSwitchSettings(int argc, char *argv[]);

int getSettingInt(enum settingLabel label);
bool getSettingBool(enum settingLabel label);

#endif // SETTINGS_H
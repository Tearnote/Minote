// Minote - settings.c

#include "settings.h"

#include "state.h"
#include "log.h"
#include "main.h"
#include "thread.h"

mutex settingsMutex = newMutex;

enum settingType {
	SettingTypeNone,
	SettingTypeInt,
	SettingTypeBool,
	SettingTypeSize
};

struct setting {
	enum settingType type;
	union {
		int intValue;
		bool boolValue;
	};
	union {
		int intDefaultValue;
		bool boolDefaultValue;
	};
};

static struct setting settings[SettingSize] = {
	{ .type = SettingTypeNone }, // SettingNone
	{ .type = SettingTypeBool, .boolDefaultValue = false }, // SettingFullscreen
	{ .type = SettingTypeInt, .intDefaultValue = 0 } // SettingInitialState
};

int getSettingInt(enum settingLabel label)
{
	lockMutex(&settingsMutex);
	if (settings[label].type != SettingTypeInt) {
		logError("Wrong type queried for setting #%d", label);
		return 0;
	}
	int value = settings[label].intValue;
	unlockMutex(&settingsMutex);
	return value;
}

bool getSettingBool(enum settingLabel label)
{
	lockMutex(&settingsMutex);
	if (settings[label].type != SettingTypeBool) {
		logError("Wrong type queried for setting #%d", label);
		return 0;
	}
	bool value = settings[label].boolValue;
	unlockMutex(&settingsMutex);
	return value;
}

static void setSettingInt(enum settingLabel label, int value)
{
	if (settings[label].type != SettingTypeInt) {
		logError("Wrong type queried for setting #%d", label);
		return;
	}
	settings[label].intValue = value;
}

static void setSettingBool(enum settingLabel label, bool value)
{
	if (settings[label].type != SettingTypeBool) {
		logError("Wrong type queried for setting #%d", label);
		return;
	}
	settings[label].boolValue = value;
}

void initSettings(void)
{
	for (int i = 0; i < SettingSize; i++) {
		switch (settings[i].type) {
		case SettingTypeInt:
			settings[i].intValue = settings[i].intDefaultValue;
			break;
		case SettingTypeBool:
			settings[i].boolValue = settings[i].boolDefaultValue;
			break;
		default:
			break;
		}
	}
}

void cleanupSettings(void)
{
	;
}

void loadSwitchSettings(int argc, char *argv[])
{
	if (argc <= 1)
		return;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--replay") == 0) {
			setSettingInt(SettingInitialState, 1);
		} else if (strcmp(argv[i], "--fullscreen") == 0) {
			setSettingBool(SettingFullscreen, true);
		} else if (strcmp(argv[i], "--help") == 0) {
			printUsage(NULL);
			exit(0);
		} else {
			printUsage(argv[i]);
			exit(1);
		}
	}
}
#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include <nlohmann/json.hpp>
#include <utils.h> 

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

struct ChatMessage1
{
	void* PRI;
	void* Team;
	wchar_t* PlayerName;
	uint8_t PlayerNamePadding[0x8];
	wchar_t* Message;
	uint8_t MessagePadding[0x8];
	uint8_t ChatChannel;
	unsigned long bPreset : 1;
};

struct ChatMessage2 {
	int32_t Team;
	class FString PlayerName;
	class FString Message;
	uint8_t ChatChannel;
	bool bLocalPlayer : 1;
	unsigned char IDPadding[0x48];
	uint8_t MessageType;
};

class trnslt: public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase
{
	void onLoad() override;
	void onUnload() override;

public:

	void logTranslation(ChatMessage1* message);
	void HookChat();
	void UnHookChat();
	void RenderSettings() override;
	void googleTranslate(ChatMessage1* message);
	void cancelMsg();
};

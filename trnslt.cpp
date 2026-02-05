#include "pch.h"
#include "trnslt.h"
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include "Languages.h"
#include <regex>
#include "Translate.h"
#include <iostream>
#include <regex>
#include <string>
#include <fstream>
#include <atomic>

using namespace nlohmann;

BAKKESMOD_PLUGIN(trnslt, "trnslt", plugin_version, PERMISSION_ALL)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void trnslt::onLoad() {
    _globalCvarManager = cvarManager;

    this->RegisterCvars();

    cvarManager->registerNotifier("trnslt_set_language_to", [this](std::vector<std::string> params) {
        if (params.size() < 2) {
            LOG("usage: trnslt_set_language_to <language_code>");
            return;
        }

        std::string langCode = params[1];
        auto it = std::find_if(LanguageCodes.begin(), LanguageCodes.end(), [&langCode](const auto& pair) { return pair.second == langCode; });

        if (it != LanguageCodes.end()) {
            cvarManager->getCvar("trnslt_language_to").setValue(langCode);
        } else {
            cvarManager->getCvar("trnslt_language_to").setValue("en");
            LOG("Language code: \"{}\" not found", langCode);
        }

    }, "set transltate to language", PERMISSION_ALL);

    this->HookGameStart();
    this->AlterMsg();
}

void trnslt::HookGameStart() {
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame", [this](std::string eventName) {
        this->CancelQueue.clear();
        this->LogMessages.clear();
        this->FixQueue.clear();
    });
}

void trnslt::onUnload() {
    this->ReleaseHooks();
}

// https://github.com/JulienML/BetterChat/blob/fd0650ae30c12c11c70302045cfd9d4b6e181759/BetterChat.cpp#L495
void trnslt::AlterMsg () {
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_Chat_TA.OnChatMessage", [this](ActorWrapper Caller, void* params, ...) {
        FGFxChatMessage* message = (FGFxChatMessage*)params;

        static std::atomic<bool> ignoreNextMessage = false;

        // Avoid reprocessing our own messages
        if (ignoreNextMessage)
        {
            ignoreNextMessage = false;
            return;
        }

        if (!message)
            return;

        // Ignore system messages
        if (message->Team == -1) { return; }

        // Ignore non-regular messages, such as from plugins (including ours)
        if (message->MessageType != 0) { return; }
        
        // Ignore bakkesmod messages
        //if (message->PlayerName.ToString() == "BAKKESMOD") { return; }

        if (message->PlayerName.ToString() == gameWrapper->GetPlayerName().ToString() && !cvarManager->getCvar("trnslt_translate_own").getBoolValue()) { return; }

        if (message->ChatChannel == 0 && !cvarManager->getCvar("trnslt_translate_0").getBoolValue()) { return; }
        if (message->ChatChannel == 1 && !cvarManager->getCvar("trnslt_translate_1").getBoolValue()) { return; }
        if (message->ChatChannel == 2 && !cvarManager->getCvar("trnslt_translate_2").getBoolValue()) { return; }

        if (cvarManager->getCvar("trnslt_remove_message").getBoolValue()) {
            this->CancelQueue.push_back({ message->Message.ToString(), "", message->ChatChannel, message->PlayerName.ToString() });
        }

        ignoreNextMessage = true;
        LogTranslation(message);
        LOG("Translated message from user: {}", message->PlayerName.ToString());

        if (cvarManager->getCvar("trnslt_remove_message").getBoolValue()) {
            // check for message queue todo
            auto it = std::find_if(this->CancelQueue.begin(), this->CancelQueue.end(), [message](LogMessage& logMessage) {
				return logMessage.OriginalMessage == message->Message.ToString() && logMessage.PlayerName == message->PlayerName.ToString();
			});

            if (it != this->CancelQueue.end()) {
                message->Message = FS("");
                message->PlayerName = FS("");
                message->TimeStamp = FS("");
                message->ChatChannel = 0;

				this->CancelQueue.erase(it);
                return;
            }
        }

        auto it = std::find_if(this->FixQueue.begin(), this->FixQueue.end(), [message](LogMessage& logMessage) {
            return logMessage.TranslatedMessage == message->Message.ToString() && logMessage.PlayerName == message->PlayerName.ToString();
        });

		if (it != this->FixQueue.end()) {
            message->ChatChannel = it->ChatChannel;
            message->Team = it->Team;

			this->FixQueue.erase(it);
		}
    });
}

void trnslt::ReleaseHooks() {
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
	gameWrapper->UnhookEvent("Function TAGame.HUDBase_TA.OnChatMessage");
    gameWrapper->UnhookEvent("Function TAGame.GFxData_Chat_TA.OnChatMessage");
}

void trnslt::RenderSettings() {

    bool trnslt_transliterate = cvarManager->getCvar("trnslt_should_transliterate").getBoolValue();

    ImGui::BeginChild("Info", ImVec2(250, 85), true);
    {
        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Info").x / 2);
        ImGui::Text("Info");

        ImGui::Separator();

        ImGui::Text("Language: %s", GetLanguageFromCode(cvarManager->getCvar("trnslt_language_to").getStringValue()));
        ImGui::Text("Translation API: %s", translateApis[cvarManager->getCvar("trnslt_translate_api").getIntValue()].c_str());

        ImGui::Text("Transliterate: %s", trnslt_transliterate ? "Enabled" : "Disabled");
    }
    ImGui::EndChild();

    float baseHeight = ImGui::GetContentRegionAvail().y;
    float baseWidth = ImGui::GetContentRegionAvail().x;

    ImGui::BeginChild("Language", ImVec2(250, baseHeight), true);
    {
        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Language").x / 2);
        ImGui::Text("Language");

        ImGui::Separator();

        if (ImGui::BeginCombo("API", translateApis[cvarManager->getCvar("trnslt_translate_api").getIntValue()].c_str()))
        {
            for (int i = 0; i < translateApis.size(); i++)
            {
                bool isSelected = (cvarManager->getCvar("trnslt_translate_api").getIntValue() == i);

                if (ImGui::Selectable(translateApis[i].c_str(), isSelected))
                {
                    cvarManager->getCvar("trnslt_translate_api").setValue(i);
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::Separator();

        ImGui::InputTextWithHint("##xxSearch", "Search", &SearchBuffer);

        ImGui::BeginChild("##xxLangChild");
        {
            for (const auto& [name, code] : LanguageCodes)
            {
                if (SearchBuffer.empty() || toLower(std::string(name)).find(toLower(SearchBuffer)) != std::string::npos)
                {
                    bool isSelected = (cvarManager->getCvar("trnslt_language_to").getStringValue() == code);

                    if (ImGui::Selectable(std::string(name).c_str(), isSelected))
                    {
                        cvarManager->getCvar("trnslt_language_to").setValue(code);
                        this->SearchBuffer = "";
                    }

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
            }
        }ImGui::EndChild();
    }ImGui::EndChild();

    ImGui::SameLine();

    ImGui::SetCursorPosY(21);
    ImGui::BeginChild("Options", ImVec2(250, 89 + baseHeight), true);
    {
        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Options").x / 2);
        ImGui::Text("Options");

        ImGui::Separator();

        bool trnslt_remove_msg = cvarManager->getCvar("trnslt_remove_message").getBoolValue();
        bool trnslt_show_timestamp = cvarManager->getCvar("trnslt_display_timestamp").getBoolValue();
        bool trnslt_local_msgs = cvarManager->getCvar("trnslt_translate_own").getBoolValue();

        if (ImGui::Checkbox("Transliterate", &trnslt_transliterate)) cvarManager->getCvar("trnslt_should_transliterate").setValue(trnslt_transliterate);
        if (ImGui::Checkbox("Remove Messages", &trnslt_remove_msg)) cvarManager->getCvar("trnslt_remove_message").setValue(trnslt_remove_msg);
        if (ImGui::Checkbox("Show TimeStamp", &trnslt_show_timestamp)) cvarManager->getCvar("trnslt_display_timestamp").setValue(trnslt_show_timestamp);
        if (ImGui::Checkbox("Translate My Messages", &trnslt_local_msgs)) cvarManager->getCvar("trnslt_translate_own").setValue(trnslt_local_msgs);

        ImGui::Separator();

        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Translate Chats").x / 2);
        ImGui::Text("Translate Chats");

        ImGui::Separator();

        bool trnslt_public = cvarManager->getCvar("trnslt_translate_0").getBoolValue();
        bool trnslt_team = cvarManager->getCvar("trnslt_translate_1").getBoolValue();
        bool trnslt_party = cvarManager->getCvar("trnslt_translate_2").getBoolValue();

        if (ImGui::Checkbox("Public", &trnslt_public)) cvarManager->getCvar("trnslt_translate_0").setValue(trnslt_public);
        if (ImGui::Checkbox("Team", &trnslt_team)) cvarManager->getCvar("trnslt_translate_1").setValue(trnslt_team);
        if (ImGui::Checkbox("Party", &trnslt_party)) cvarManager->getCvar("trnslt_translate_2").setValue(trnslt_party);

        ImGui::Separator();

        ImGui::SetCursorPosX((250 / 2) - ImGui::CalcTextSize("Extra").x / 2);
        ImGui::Text("Extra");

        ImGui::Separator();

        if (ImGui::Button("Clear Message Log")) 
        {
            this->LogMessages.clear();
        }

        if (ImGui::Button("Reset settings")) 
        {
            cvarManager->getCvar("trnslt_translate_api").setValue(0);
            cvarManager->getCvar("trnslt_language_to").setValue("en");
            cvarManager->getCvar("trnslt_translate_0").setValue(true);
            cvarManager->getCvar("trnslt_translate_1").setValue(true);
            cvarManager->getCvar("trnslt_translate_2").setValue(true);
            cvarManager->getCvar("trnslt_remove_message").setValue(true);
            cvarManager->getCvar("trnslt_display_timestamp").setValue(true);
            cvarManager->getCvar("trnslt_should_transliterate").setValue(false);
            cvarManager->getCvar("trnslt_translate_own").setValue(false);
        }
    }ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Message Log", ImVec2(baseWidth - ImGui::GetCursorPosX(), 89 + baseHeight), true);
    {
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() / 2 - (ImGui::CalcTextSize("Chat Message Log").x / 2));
        ImGui::Text("Chat Message Log");

        ImGui::Separator();

        for (auto& msg : this->LogMessages) {
            ImGui::Text(std::format("[{}] {}", msg.PlayerName, msg.OriginalMessage).c_str());
        }
    }ImGui::EndChild();
}

void trnslt::RegisterCvars()
{
    cvarManager->registerCvar("trnslt_should_transliterate", "0");

    cvarManager->registerCvar("trnslt_should_show_match_log", "0");

    cvarManager->registerCvar("trnslt_remove_message", "1");
    cvarManager->registerCvar("trnslt_translate_own", "0");
    cvarManager->registerCvar("trnslt_display_timestamp", "1");

    cvarManager->registerCvar("trnslt_translate_0", "1");
    cvarManager->registerCvar("trnslt_translate_1", "1");
    cvarManager->registerCvar("trnslt_translate_2", "1");

    cvarManager->registerCvar("trnslt_translate_api", "0", "transltae api index", false, true, 0, true, translateApis.size() - 1, true);

    cvarManager->registerCvar("trnslt_language_to", "en", "language to translate to", true, false, 0, false, 0, true);
}

void trnslt::LogTranslation(FGFxChatMessage* message) {
    switch (cvarManager->getCvar("trnslt_translate_api").getIntValue()) {
    case 0:
        GoogleTranslate(message);
        break;
    default:
        break;
    }
}
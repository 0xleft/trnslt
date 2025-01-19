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

BAKKESMOD_PLUGIN(trnslt, "trnslt", plugin_version, PERMISSION_ALL)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void trnslt::onLoad() {
	_globalCvarManager = cvarManager;

    cvarManager->registerCvar("trnslt_should_transliterate", "0");

    cvarManager->registerCvar("trnslt_should_show_match_log", "0");

    cvarManager->registerCvar("trnslt_remove_message", "1");
    cvarManager->registerCvar("trnslt_translate_own", "0");

    cvarManager->registerCvar("trnslt_translate_0", "1");
    cvarManager->registerCvar("trnslt_translate_1", "1");
    cvarManager->registerCvar("trnslt_translate_2", "1");

    cvarManager->registerCvar("trnslt_translate_api", "0", "transltae api index", false, true, 0, true, translateApis.size() - 1, true);

    cvarManager->registerCvar("trnslt_language_to", "en", "language to translate to", true, false, 0, false, 0, true);
    cvarManager->registerNotifier("trnslt_set_language_to", [this](std::vector<std::string> params) {
        if (params.size() < 2) {
            LOG("usage: trnslt_set_language_to <language_code>");
            return;
        }

        std::string langCode = params[1];
        auto it = std::find_if(languageCodes.begin(), languageCodes.end(), [&langCode](const auto& pair) { return pair.second == langCode; });

        if (it != languageCodes.end()) {
            cvarManager->getCvar("trnslt_language_to").setValue(langCode);
        } else {
            LOG("Language code: \"{}\" not found", langCode);
        }

    }, "set transltate to language", PERMISSION_ALL);

    this->HookChat();
    this->HookGameStart();
}

void trnslt::HookGameStart() {
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame", [this](std::string eventName) {
        this->logMessages.clear();
    });
}

void trnslt::UnhookGameStart() {
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
}

void trnslt::onUnload() {
    this->UnHookChat();
    this->UnhookGameStart();
}

void trnslt::HookChat() {
    // https://github.com/JulienML/BetterChat/blob/fd0650ae30c12c11c70302045cfd9d4b6e181759/BetterChat.cpp#L509
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.HUDBase_TA.OnChatMessage", [this](ActorWrapper Caller, void* params, ...) {
        if (params) {
            ChatMessage1* message = (ChatMessage1*)params;
            if (message->PlayerName == nullptr) return;
            if (message->PlayerName == gameWrapper->GetPlayerName().ToWideString() && !cvarManager->getCvar("trnslt_translate_own").getBoolValue()) { return; }

            if (message->ChatChannel == 0 && !cvarManager->getCvar("trnslt_translate_0").getBoolValue()) { return; }
            if (message->ChatChannel == 1 && !cvarManager->getCvar("trnslt_translate_1").getBoolValue()) { return; }
            if (message->ChatChannel == 2 && !cvarManager->getCvar("trnslt_translate_2").getBoolValue()) { return; }

            if (message->Message == nullptr) return;
            // quickchats
            std::regex quickChatPattern("Group\\d+Message\\d+");
            if (std::regex_match(wToString(message->Message), quickChatPattern)) {
                return;
            }

            if (cvarManager->getCvar("trnslt_remove_message").getBoolValue()) {
                cancelMsg();
			}

            logTranslation(message);
        }

        gameWrapper->HookEventWithCallerPost<ActorWrapper>("Function TAGame.HUDBase_TA.OnChatMessage", [this](ActorWrapper caller, void* params, ...) {
            gameWrapper->UnhookEvent("Function TAGame.GFxData_Chat_TA.OnChatMessage");
        });
    });
}

// https://github.com/JulienML/BetterChat/blob/fd0650ae30c12c11c70302045cfd9d4b6e181759/BetterChat.cpp#L495
void trnslt::cancelMsg() {
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxData_Chat_TA.OnChatMessage", [this](ActorWrapper Caller, void* params, ...) {
        ChatMessage2* Params = (ChatMessage2*)params;
        Params->Message = FS("");
        Params->PlayerName = FS("");
        Params->ChatChannel = 0;
    });
}

void trnslt::UnHookChat() {
    gameWrapper->UnhookEvent("Function TAGame.GFxData_Chat_TA.OnChatMessage");
}

void trnslt::RenderSettings() {
    float availWidth = ImGui::GetContentRegionAvail().x;

    ImGui::BeginChild("Select test", ImVec2(availWidth / 2, 0));
    ImGui::Text("Info:");
    ImGui::Separator();

    ImGui::Text(std::format("Selected language: {}", cvarManager->getCvar("trnslt_language_to").getStringValue()).c_str());

    ImGui::InvisibleButton("1", ImVec2(10, 10));

    ImGui::Text("Select api being used");
    ImGui::Separator();
    for (int i = 0; i < translateApis.size(); i++) {
        if (ImGui::Selectable(translateApis.at(i).c_str())) {
            cvarManager->getCvar("trnslt_translate_api").setValue(i);
        }
    }

    ImGui::InvisibleButton("2", ImVec2(10, 10));

    ImGui::Text("Choose if chat type should be translated");
    ImGui::Separator();
    bool publicChat = cvarManager->getCvar("trnslt_translate_0").getBoolValue();
    bool* publicChat_p = &publicChat;
    if (ImGui::Checkbox("Public chat", publicChat_p)) {
        cvarManager->getCvar("trnslt_translate_0").setValue(!cvarManager->getCvar("trnslt_translate_0").getBoolValue());
    }

    bool teamChat = cvarManager->getCvar("trnslt_translate_1").getBoolValue();
    bool* teamChat_p = &teamChat;
    if (ImGui::Checkbox("Team chat", teamChat_p)) {
        cvarManager->getCvar("trnslt_translate_1").setValue(!cvarManager->getCvar("trnslt_translate_1").getBoolValue());
    }

    bool partyChat = cvarManager->getCvar("trnslt_translate_2").getBoolValue();
    bool* partyChat_p = &partyChat;
    if (ImGui::Checkbox("Party chat", partyChat_p)) {
        cvarManager->getCvar("trnslt_translate_2").setValue(!cvarManager->getCvar("trnslt_translate_2").getBoolValue());
    }

    ImGui::InvisibleButton("3", ImVec2(10, 10));

    bool removeMessage = cvarManager->getCvar("trnslt_remove_message").getBoolValue();
    bool* removeMessage_p = &removeMessage;
    if (ImGui::Checkbox("Remove message", removeMessage_p)) {
		cvarManager->getCvar("trnslt_remove_message").setValue(!cvarManager->getCvar("trnslt_remove_message").getBoolValue());
	}

    bool translateOwn = cvarManager->getCvar("trnslt_translate_own").getBoolValue();
    bool* translateOwn_p = &translateOwn;
    if (ImGui::Checkbox("Translate own messages", translateOwn_p)) {
		cvarManager->getCvar("trnslt_translate_own").setValue(!cvarManager->getCvar("trnslt_translate_own").getBoolValue());
	}

    bool transliterate = cvarManager->getCvar("trnslt_should_transliterate").getBoolValue();
    bool* transliterate_p = &transliterate;
    if (ImGui::Checkbox("Transliterate text", transliterate_p)) {
        cvarManager->getCvar("trnslt_should_transliterate").setValue(!cvarManager->getCvar("trnslt_should_transliterate").getBoolValue());
    }

    ImGui::InvisibleButton("3", ImVec2(10, 10));

    if (ImGui::Button(cvarManager->getCvar("trnslt_should_show_match_log").getBoolValue() ? "Close match log" : "Open match log")) {
        cvarManager->getCvar("trnslt_should_show_match_log").setValue(!cvarManager->getCvar("trnslt_should_show_match_log").getBoolValue());
    }

    if (ImGui::Button("Reset match log")) {
		this->logMessages.clear();
	}

    if (cvarManager->getCvar("trnslt_should_show_match_log").getBoolValue()) {
		this->drawMessageLog();
	}

    if (ImGui::Button("Reset settings")) {
        cvarManager->getCvar("trnslt_translate_0").setValue(true);
        cvarManager->getCvar("trnslt_translate_1").setValue(true);
        cvarManager->getCvar("trnslt_translate_2").setValue(true);
        cvarManager->getCvar("trnslt_translate_api").setValue(0);
        cvarManager->getCvar("trnslt_language_to").setValue("en");
        cvarManager->getCvar("trnslt_remove_message").setValue(true);
        cvarManager->getCvar("trnslt_translate_own").setValue(false);
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Select language");
    
    ImGui::Text("Select language to translate to");
    ImGui::Separator();
    for (std::pair<std::string, std::string> item : languageCodes) {
        if (ImGui::Selectable(item.first.c_str())) {
            cvarManager->getCvar("trnslt_language_to").setValue(item.second);
        }
    }
    ImGui::EndChild();
}

void trnslt::drawMessageLog() {
    bool shouldShow = cvarManager->getCvar("trnslt_should_show_match_log").getBoolValue();
    bool* shouldShow_p = &shouldShow;
    ImGui::Begin("Match log", shouldShow_p, ImGuiWindowFlags_AlwaysAutoResize);
    for (auto& msg : this->logMessages) {
		ImGui::Text(std::format("[{}] {}", msg.playerName, msg.originalMessage).c_str());
	}
    ImGui::End();
}

void trnslt::logTranslation(ChatMessage1* message) {

    switch (cvarManager->getCvar("trnslt_translate_api").getIntValue()) {
    case 0:
        // google
        googleTranslate(message);
        break;
    default:
        break;
    }
}
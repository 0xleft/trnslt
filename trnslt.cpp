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
    cvarManager->registerCvar("trnslt_translate_0", "1");
    cvarManager->registerCvar("trnslt_translate_1", "1");
    cvarManager->registerCvar("trnslt_translate_2", "1");

    cvarManager->registerCvar("trnslt_translate_api", "0", "transltae api index", false, true, 0, true, translateApis.size() - 1, true);

	cvarManager->registerNotifier("trnslt_test", [this](std::vector<std::string> params) {
		logTranslation("labas as krabas");
	}, "", PERMISSION_ALL);

    cvarManager->registerNotifier("trnslt_translate", [this](std::vector<std::string> params) {
        if (params.size() < 2) {
            LOG("usage: trnslt_translate <sentence>");
            return;
        }

        params.erase(params.begin());

        std::stringstream ss;
        for (size_t i = 0; i < params.size(); ++i) { if (i != 0) { ss << " "; } ss << params[i]; } // yeah :)

        std::regex quickChatPattern("Group\\d+Message\\d+");
        if (std::regex_match(ss.str(), quickChatPattern)) {
            return;
        }

        logTranslation(ss.str());
    }, "translate", PERMISSION_ALL);

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
}

void trnslt::onUnload() {
    this->UnHookChat();
}

void trnslt::HookChat() {
    // https://github.com/Sei4or/QuickVoice/blob/master/QuickVoice/QuickVoice.cpp by @Sei4or
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.HUDBase_TA.OnChatMessage", [this](ActorWrapper caller, void* params, std::string eventName) {
        if (params) {
            ChatMessage* chatMessage = static_cast<ChatMessage*>(params);
            if (chatMessage->PlayerName == nullptr) return;
            std::wstring playerName(chatMessage->PlayerName);
            if (playerName == gameWrapper->GetPlayerName().ToWideString()) { return; }
            if (chatMessage->Message == nullptr) return;
            std::wstring message(chatMessage->Message);
            std::string bMessage(message.begin(), message.end());

            if (chatMessage->ChatChannel == 0 && !cvarManager->getCvar("trnslt_translate_0").getBoolValue()) { return; }
            if (chatMessage->ChatChannel == 1 && !cvarManager->getCvar("trnslt_translate_1").getBoolValue()) { return; }
            if (chatMessage->ChatChannel == 2 && !cvarManager->getCvar("trnslt_translate_2").getBoolValue()) { return; }

            cvarManager->executeCommand(std::format("trnslt_translate {}", bMessage));
        }
    });
}

void trnslt::UnHookChat() {
    gameWrapper->UnhookEvent("Function TAGame.HUDBase_TA.OnChatMessage");
}

void trnslt::RenderSettings() {
    float availWidth = ImGui::GetContentRegionAvail().x;

    ImGui::BeginChild("Select test", ImVec2(availWidth / 2, 0));
    ImGui::Text("Info:");
    ImGui::Separator();

    ImGui::Text(std::format("Selected language: {}", cvarManager->getCvar("trnslt_language_to").getStringValue()).c_str());
    ImGui::Text(std::format("Last translated language: {}", this->transSrc).c_str());
    ImGui::Text(std::format("Last translated message: {}", this->trans).c_str());

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


    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Select language");
    
    ImGui::Text("Select language to translate to");
    ImGui::Separator();
    for (auto item : languageCodes) {
        if (ImGui::Selectable(item.first.c_str())) {
            cvarManager->getCvar("trnslt_language_to").setValue(item.second);
        }
    }
    ImGui::EndChild();
}

void trnslt::logTranslation(std::string text) {
    switch (cvarManager->getCvar("trnslt_translate_api").getIntValue()) {
    case 0:
        // google
        googleTranslate(text);
        break;

    default:
        break;
    }
}
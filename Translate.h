#pragma once

#include <vector>
#include <string>
#include "trnslt.h"

std::vector<std::string> translateApis = {
	"Google translate",
    "dont select this one",
};

std::string urlEncode(std::string str) {
    // https://stackoverflow.com/questions/154536/encode-decode-urls-in-ced by @tormuto
    std::string new_str = "";
    char c;
    int ic;
    const char* chars = str.c_str();
    char bufHex[10];
    int len = strlen(chars);

    for (int i = 0; i < len; i++) {
        c = chars[i];
        ic = c;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') new_str += c;
        else {
            sprintf(bufHex, "%X", c);
            if (ic < 16)
                new_str += "%0";
            else
                new_str += "%";
            new_str += bufHex;
        }
    }
    return new_str;
}

// https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
bool stringReplace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void trnslt::googleTranslate(std::string text) {
    CurlRequest req;
    req.url = std::format("https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl={}&dt=t&dt=bd&dj=1&q={}", cvarManager->getCvar("trnslt_language_to").getStringValue(), urlEncode(text));

    HttpWrapper::SendCurlRequest(req, [this](int code, std::string result) {
        try {
            json data = json::parse(result);
            json sentences = data["sentences"];
            for (int i = 0; i < sentences.size(); i++) {
                json sentence = sentences.at(i);
                std::string trans(sentence["trans"]);
                std::string orig(sentence["orig"]);
                LOG("Trans: {}", trans);
                if (trans == orig) { continue; }
                this->trans = trans;
                this->transSrc = data["src"];

                gameWrapper->Execute([this](GameWrapper* gw) {
                    gameWrapper->LogToChatbox(this->trans, std::format("[{}] trnslt", this->transSrc));
                    });
            }
        }
        catch (std::exception e) {
            LOG("{}", e.what());
        }
    });
}
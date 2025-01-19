#ifndef TRANSLITERATION_LANG
#define TRANSLITERATION_LANG

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>

namespace transliteration {
	class TransliterationLang {
	public:
		inline std::string getLanguageCode() { return this->languageCode; }
		inline std::wstring getMappingSource() { return this->mappingSource; }
		inline std::wstring getMappingTarget() { return this->mappingTarget; }
		inline std::unordered_map<std::wstring, std::wstring> getSpecificMapping() { return this->specificMapping; }
		inline std::unordered_map<std::wstring, std::wstring> getPreProcessorMapping() { return this->preProcessorMapping; }
		inline std::unordered_map<std::wstring, std::wstring> getSpecificPreProcessorMapping() { return this->specificPreProcessorMapping; }

		inline std::wstring transliterate(std::wstring text) {
			for (std::pair<std::wstring, std::wstring> preprocessorPair : this->preProcessorMapping) {
				text = std::regex_replace(text, std::wregex(preprocessorPair.first), preprocessorPair.second);
			}

			for (std::pair<std::wstring, std::wstring> mappingPair : this->mapping) {
				if (mappingPair.second == L"\'") {
					text = std::regex_replace(text, std::wregex(mappingPair.first), L"");
					continue;
				}

				text = std::regex_replace(text, std::wregex(mappingPair.first), mappingPair.second);
			}

			return text;
		}

		inline void getMapping() {
			for (int i = 0; i < this->mappingSource.size(); i++) {
				this->mapping[this->mappingSource.substr(i, 1)] = this->mappingTarget.substr(i, 1);
			}
		}

		TransliterationLang(
			std::string languageCode,
			std::wstring mappingSource,
			std::wstring mappingTarget,
			std::unordered_map<std::wstring, std::wstring> specificMapping,
			std::unordered_map<std::wstring, std::wstring> preProcessorMapping,
			std::unordered_map<std::wstring, std::wstring> specificPreProcessorMapping) {

			this->languageCode = languageCode;
			this->mappingSource = mappingSource;
			this->mappingTarget = mappingTarget;
			getMapping();
			this->specificMapping = specificMapping;
			this->preProcessorMapping = preProcessorMapping;
			this->specificPreProcessorMapping = specificPreProcessorMapping;
		}

	private:
		std::string languageCode;
		std::wstring mappingSource;
		std::wstring mappingTarget;
		std::unordered_map<std::wstring, std::wstring> mapping;
		std::unordered_map<std::wstring, std::wstring> specificMapping;
		std::unordered_map<std::wstring, std::wstring> preProcessorMapping;
		std::unordered_map<std::wstring, std::wstring> specificPreProcessorMapping;
	};
}

#endif // TRANSLITERATION_LANG
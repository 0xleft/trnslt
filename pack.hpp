// not this is basicaly a port of https://github.com/barseghyanartur/transliterate/ for c++

#include "lang.hpp"
#include "russian.hpp"

namespace transliteration {
	class TransliterationPack {
	public:

		TransliterationPack() {
			registerAllLanguages();
		}
		~TransliterationPack() {
			for (TransliterationLang* lang : languages) {
				delete lang;
			}
		}
		inline void registerLang(TransliterationLang* lang) { this->languages.push_back(lang); }
		inline std::wstring transliterate(std::wstring text, std::string languageCode) {
			for (TransliterationLang* lang : languages) {
				if (lang->getLanguageCode() == languageCode) {
					return lang->transliterate(text);
				}
			}

			return text;
		}
	private:
		std::vector<TransliterationLang*> languages;

		void registerAllLanguages() {
			registerLang(getRussian());
		}
	};
}

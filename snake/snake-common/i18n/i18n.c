#include "i18n.h"

static Language g_current = LANG_EN;

const LangMeta LANG_META[LANG_COUNT] = {
    {LANG_EN,    "en",    "English"},
    {LANG_DE,    "de",    "Deutsch"},
    {LANG_FR,    "fr",    "Fran\xc3\xa7" "ais"},
    {LANG_ES,    "es",    "Espa\xc3\xb1" "ol"},
    {LANG_IT,    "it",    "Italiano"},
    {LANG_PT_BR, "pt-BR", "Portugu\xc3\xaas (BR)"},
    {LANG_PT_PT, "pt-PT", "Portugu\xc3\xaas (PT)"},
    {LANG_ID,    "id",    "Bahasa Indonesia"},
    {LANG_TR,    "tr",    "T\xc3\xbcrk\xc3\xa7" "e"},
    {LANG_VI,    "vi",    "Ti\xe1\xba\xbfng Vi\xe1\xbb\x87t"},
    {LANG_NL,    "nl",    "Nederlands"},
    {LANG_NO,    "no",    "Norsk"},
    {LANG_DA,    "da",    "Dansk"},
    {LANG_SV,    "sv",    "Svenska"},
    {LANG_FI,    "fi",    "Suomi"},
    {LANG_PL,    "pl",    "Polski"},
    {LANG_CS,    "cs",    "\xc4\x8c" "esky"},
    {LANG_RO,    "ro",    "Rom\xc3\xa2n\xc4\x83"},
    {LANG_HU,    "hu",    "Magyar"},
    {LANG_CA,    "ca",    "Catal\xc3\xa0"},
    {LANG_AF,    "af",    "Afrikaans"},
    {LANG_SR,    "sr",    "Srpski"},
    {LANG_RU,    "ru",    "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9"},
    {LANG_UK,    "uk",    "\xd0\xa3\xd0\xba\xd1\x80\xd0\xb0\xd1\x97\xd0\xbd\xd1\x81\xd1\x8c\xd0\xba\xd0\xb0"},
    {LANG_EL,    "el",    "\xce\x95\xce\xbb\xce\xbb\xce\xb7\xce\xbd\xce\xb9\xce\xba\xce\xac"},
};

void lang_set(Language lang) {
    if (lang < LANG_COUNT) g_current = lang;
}

Language lang_get_current(void) {
    return g_current;
}

const char *lang_str(StringID id) {
    if (id >= STR_COUNT) return "";
    return STRINGS[g_current][id];
}

#include "i18n.h"

static Language current_lang = LANG_EN;

void lang_set(Language lang) {
    if (lang >= 0 && lang < LANG_COUNT)
        current_lang = lang;
}

Language lang_get_current(void) {
    return current_lang;
}

const char *lang_str(StringID id) {
    if (id >= 0 && id < STR_COUNT)
        return STRINGS[current_lang][id];
    return "???";
}

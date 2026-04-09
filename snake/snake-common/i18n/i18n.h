#pragma once

typedef enum {
    LANG_EN=0, LANG_DE,   LANG_FR,   LANG_ES,   LANG_IT,
    LANG_PT_BR,LANG_PT_PT,LANG_ID,   LANG_TR,   LANG_VI,
    LANG_NL,   LANG_NO,   LANG_DA,   LANG_SV,   LANG_FI,
    LANG_PL,   LANG_CS,   LANG_RO,   LANG_HU,   LANG_CA,
    LANG_AF,   LANG_SR,
    LANG_RU,   LANG_UK,
    LANG_EL,
    LANG_COUNT  /* 25 */
} Language;

typedef enum {
    STR_TITLE = 0,
    STR_START,
    STR_RESTART,
    STR_PAUSED,
    STR_WIN,
    STR_LOSE,
    STR_WALL_HIT,
    STR_SELF_HIT,
    STR_VANISHED,
    STR_SIZE,
    STR_AUTOPILOT,
    STR_ON,
    STR_OFF,
    STR_ANY_KEY,
    STR_COUNT
} StringID;

typedef struct {
    Language    id;
    const char *code;         /* "ru", "el"        */
    const char *native_name;  /* "Русский", "English" — displayed on the button */
} LangMeta;

extern const LangMeta        LANG_META[LANG_COUNT];
extern const char * const    STRINGS[LANG_COUNT][STR_COUNT];

void        lang_set(Language lang);
Language    lang_get_current(void);
const char *lang_str(StringID id);

/* Implemented in render_nds.c / render_3ds.c */
Language    lang_detect_system(void);

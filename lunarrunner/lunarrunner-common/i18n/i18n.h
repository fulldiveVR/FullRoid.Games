#pragma once

typedef enum {
    LANG_EN = 0,
    LANG_RU,
    LANG_COUNT
} Language;

typedef enum {
    STR_TITLE = 0,
    STR_SCORE,
    STR_BEST,
    STR_DISTANCE,
    STR_GAME_OVER,
    STR_PAUSED,
    STR_PRESS_A_START,
    STR_RETRY,
    STR_MENU,
    STR_RESUME,
    STR_QUIT,
    STR_JUMP,
    STR_DUCK,
    STR_BONUS,
    STR_PAUSE,
    STR_SHIELD,
    STR_SOLAR,
    STR_TURBO,
    STR_MAGNET,
    STR_READY,
    STR_ACTIVE,
    STR_COUNT
} StringID;

typedef struct {
    Language    id;
    const char *code;
    const char *native_name;
} LangMeta;

extern const LangMeta     LANG_META[LANG_COUNT];
extern const char * const STRINGS[LANG_COUNT][STR_COUNT];

void        lang_set(Language lang);
Language    lang_get_current(void);
const char *lang_str(StringID id);

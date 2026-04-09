#include "i18n.h"

const LangMeta LANG_META[LANG_COUNT] = {
    { LANG_EN, "en", "English" },
    { LANG_RU, "ru", "Russkij" },
};

const char * const STRINGS[LANG_COUNT][STR_COUNT] = {
    /* LANG_EN */
    {
        "LUNAR RUNNER",         /* STR_TITLE */
        "Score",                /* STR_SCORE */
        "Best",                 /* STR_BEST */
        "Dist",                 /* STR_DISTANCE */
        "GAME OVER",            /* STR_GAME_OVER */
        "PAUSED",               /* STR_PAUSED */
        "Press A to start",     /* STR_PRESS_A_START */
        "A: retry",             /* STR_RETRY */
        "B: menu",              /* STR_MENU */
        "START/A: resume",      /* STR_RESUME */
        "B: quit",              /* STR_QUIT */
        "Jump",                 /* STR_JUMP */
        "Duck",                 /* STR_DUCK */
        "Bonus",                /* STR_BONUS */
        "Pause",                /* STR_PAUSE */
        "SHIELD",               /* STR_SHIELD */
        "SOLAR x2",             /* STR_SOLAR */
        "TURBO",                /* STR_TURBO */
        "MAGNET",               /* STR_MAGNET */
        "Ready",                /* STR_READY */
        "active",               /* STR_ACTIVE */
    },
    /* LANG_RU — rendered via bitmap font (supports Cyrillic) */
    {
        "\xd0\x9b\xd0\xa3\xd0\x9d\xd0\x9d\xd0\xab\xd0\x99 \xd0\xa0\xd0\x90\xd0\x9d\xd0\x9d\xd0\x95\xd0\xa0", /* ЛУННЫЙ РАННЕР */
        "\xd0\xa1\xd1\x87\xd0\xb5\xd1\x82",                 /* Счет */
        "\xd0\xa0\xd0\xb5\xd0\xba\xd0\xbe\xd1\x80\xd0\xb4", /* Рекорд */
        "\xd0\x94\xd0\xb8\xd1\x81\xd1\x82",                 /* Дист */
        "\xd0\x98\xd0\x93\xd0\xa0\xd0\x90 \xd0\x9e\xd0\x9a\xd0\x9e\xd0\x9d\xd0\xa7\xd0\x95\xd0\x9d\xd0\x90", /* ИГРА ОКОНЧЕНА */
        "\xd0\x9f\xd0\x90\xd0\xa3\xd0\x97\xd0\x90",         /* ПАУЗА */
        "\xd0\x9d\xd0\xb0\xd0\xb6\xd0\xbc\xd0\xb8\xd1\x82\xd0\xb5 A", /* Нажмите A */
        "A: \xd0\xb7\xd0\xb0\xd0\xbd\xd0\xbe\xd0\xb2\xd0\xbe", /* A: заново */
        "B: \xd0\xbc\xd0\xb5\xd0\xbd\xd1\x8e",              /* B: меню */
        "START/A: \xd0\xb4\xd0\xb0\xd0\xbb\xd0\xb5\xd0\xb5", /* START/A: далее */
        "B: \xd0\xb2\xd1\x8b\xd1\x85\xd0\xbe\xd0\xb4",      /* B: выход */
        "\xd0\x9f\xd1\x80\xd1\x8b\xd0\xb6\xd0\xbe\xd0\xba", /* Прыжок */
        "\xd0\x9f\xd1\x80\xd0\xb8\xd1\x81\xd0\xb5\xd0\xb4", /* Присед */
        "\xd0\x91\xd0\xbe\xd0\xbd\xd1\x83\xd1\x81",         /* Бонус */
        "\xd0\x9f\xd0\xb0\xd1\x83\xd0\xb7\xd0\xb0",         /* Пауза */
        "\xd0\xa9\xd0\x98\xd0\xa2",                           /* ЩИТ */
        "\xd0\xa1\xd0\x9e\xd0\x9b\xd0\x9d\xd0\xa6\xd0\x95 x2", /* СОЛНЦЕ x2 */
        "\xd0\xa2\xd0\xa3\xd0\xa0\xd0\x91\xd0\x9e",         /* ТУРБО */
        "\xd0\x9c\xd0\x90\xd0\x93\xd0\x9d\xd0\x98\xd0\xa2", /* МАГНИТ */
        "\xd0\x93\xd0\xbe\xd1\x82\xd0\xbe\xd0\xb2",         /* Готов */
        "\xd0\xb0\xd0\xba\xd1\x82\xd0\xb8\xd0\xb2\xd0\xb5\xd0\xbd", /* активен */
    },
};

#!/usr/bin/env python3
"""Generates lang_strings.c with strings for all 25 languages."""
import os

OUT = os.path.join(os.path.dirname(__file__),
                   "..", "snake-common", "i18n", "lang_strings.c")

# (STR_TITLE, STR_START, STR_RESTART, STR_PAUSED,
#  STR_WIN, STR_LOSE, STR_WALL_HIT, STR_SELF_HIT,
#  STR_VANISHED, STR_SIZE, STR_AUTOPILOT, STR_ON, STR_OFF, STR_ANY_KEY)
LANGS = {
    "LANG_EN":    ("SNAKE",       "Start",      "Restart",    "PAUSE",
                   "Victory!",    "Game Over",  "Hit a wall", "Self collision",
                   "Snake gone",  "Size:",      "Autopilot:", "ON",  "OFF",
                   "Press any button"),
    "LANG_DE":    ("SCHLANGE",    "Start",      "Neustart",   "PAUSE",
                   "Sieg!",       "Game Over",  "Wand!",      "Selbstkollision",
                   "Weg",         "Gr\xf6\xdfe:","Autopilot:", "AN",  "AUS",
                   "Taste dr\xfccken"),
    "LANG_FR":    ("SERPENT",     "D\xe9marrer","Rejouer",    "PAUSE",
                   "Victoire!",   "Perdu",      "Mur!",       "Collision",
                   "Disparu",     "Taille:",    "Pilote auto:","OUI", "NON",
                   "Appuyer sur un bouton"),
    "LANG_ES":    ("SERPIENTE",   "Empezar",    "Reiniciar",  "PAUSA",
                   "\xa1Victoria!","Game Over", "Pared!",     "Colisi\xf3n",
                   "Desapareci\xf3","Tama\xf1o:","Autopiloto:","S\xcd",  "NO",
                   "Pulsa cualquier bot\xf3n"),
    "LANG_IT":    ("SERPENTE",    "Inizia",     "Ricomincia", "PAUSA",
                   "Vittoria!",   "Game Over",  "Muro!",      "Collisione",
                   "Sparita",     "Dimensione:","Autopilota:","S\xcc",  "NO",
                   "Premi un pulsante"),
    "LANG_PT_BR": ("COBRA",       "Jogar",      "Reiniciar",  "PAUSA",
                   "Vit\xf3ria!", "Game Over",  "Parede!",    "Colis\xe3o",
                   "Sumiu",       "Tamanho:",   "Piloto auto:","SIM", "N\xc3O",
                   "Pressione um bot\xe3o"),
    "LANG_PT_PT": ("COBRA",       "Jogar",      "Reiniciar",  "PAUSA",
                   "Vit\xf3ria!", "Game Over",  "Parede!",    "Colis\xe3o",
                   "Desapareceu","Tamanho:",    "Autopiloto:","SIM", "N\xc3O",
                   "Prima um bot\xe3o"),
    "LANG_ID":    ("ULAR",        "Mulai",      "Mulai Lagi", "JEDA",
                   "Menang!",     "Game Over",  "Dinding!",   "Tabrakan",
                   "Menghilang",  "Ukuran:",    "Autopilot:", "YA",  "TIDAK",
                   "Tekan sembarang tombol"),
    "LANG_TR":    ("YILAN",       "Ba\u015flat","Tekrar",     "DURAKLAT",
                   "Kazand\u0131n!","Oyun Bitti","Duvar!",    "\xc7arp\u0131\u015fma",
                   "Kayboldu",    "Boyut:",     "Otopilot:",  "A\xc7IK","KAPALI",
                   "Bir tu\u015fa bas"),
    "LANG_VI":    ("R\u1eafN",    "B\u1eaft \u0111\u1ea7u","Ch\u01a1i l\u1ea1i","T\u1ea0M D\u1eeeNG",
                   "Chi\u1ebfn th\u1eafng!","Thua","T\u01b0\u1eddng!","T\u1ef1 \u0111\u1ee5ng",
                   "Bi\u1ebfn m\u1ea5t","K\xedch th\u01b0\u1edbc:","T\u1ef1 l\xe1i:","B\u1eacT","T\u1eaet",
                   "Nh\u1ea5n b\u1ea5t k\u1ef3 ph\xedm"),
    "LANG_NL":    ("SLANG",       "Starten",    "Opnieuw",    "PAUZE",
                   "Gewonnen!",   "Game Over",  "Muur!",      "Botsing",
                   "Verdwenen",   "Grootte:",   "Autopiloot:","AAN", "UIT",
                   "Druk op een knop"),
    "LANG_NO":    ("SLANGE",      "Start",      "Start p\xe5 nytt","PAUSE",
                   "Vant!",       "Game Over",  "Vegg!",      "Kollisjon",
                   "Borte",       "St\xf8rrelse:","Autopilot:","P\xc5", "AV",
                   "Trykk en tast"),
    "LANG_DA":    ("SLANGE",      "Start",      "Genstart",   "PAUSE",
                   "Vundet!",     "Game Over",  "V\xe6g!",    "Kollision",
                   "V\xe6k",      "St\xf8rrelse:","Autopilot:","TIL", "FRA",
                   "Tryk p\xe5 en tast"),
    "LANG_SV":    ("ORMEN",       "Starta",     "Starta om",  "PAUS",
                   "Vann!",       "Game Over",  "V\xe4gg!",   "Kollision",
                   "F\xf6rsvann", "Storlek:",   "Autopilot:", "P\xc5",  "AV",
                   "Tryck p\xe5 en knapp"),
    "LANG_FI":    ("K\xc4\xc4RME","Aloita",    "Aloita uudelleen","TAUKO",
                   "Voitto!",     "Game Over",  "Sein\xe4!",  "T\xf6rm\xe4ys",
                   "Katosi",      "Koko:",      "Autopilotti:","P\xc4\xc4LLE","POIS",
                   "Paina nappia"),
    "LANG_PL":    ("W\u0118\u017b","\u015awiat","Jeszcze raz","PAUZA",
                   "Wygrana!",    "Koniec gry", "\u015aciana!", "Kolizja",
                   "Znik\u0142",  "Rozmiar:",   "Autopilot:", "W\u0141",  "WY\u0141",
                   "Naci\u015bnij przycisk"),
    "LANG_CS":    ("HAD",         "Start",      "Znovu",      "PAUZA",
                   "V\xedtz!",    "Konec hry",  "St\u011bna!", "Sr\xe1\u017eka",
                   "Zmizel",      "Velikost:",  "Autopilot:", "ZAP", "VYP",
                   "Stiskni kl\xe1vesu"),
    "LANG_RO":    ("BALAUR",      "Start",      "Revenire",   "PAUZ\u0102",
                   "Victorie!",   "Game Over",  "Perete!",    "Coliziune",
                   "Disp\u0103rut","M\u0103rime:","Autopilot:","DA",  "NU",
                   "Ap\u0103sa\u0163i un buton"),
    "LANG_HU":    ("K\xcd GY\xd3", "Ind\xedt\xe1s","Ism\xe9t","SZ\xdcNET",
                   "Gy\u0151zt\xe9l!","J\xe1t\xe9k v\xe9ge","Fal!","Karambol",
                   "Elt\u0171nt",  "M\xe9ret:",  "Autopil\xf3ta:","BE",  "KI",
                   "Nyomj egy gombot"),
    "LANG_CA":    ("SERP",        "Comen\xe7a", "Torna a jug.","PAUSA",
                   "Victori!",    "Game Over",  "Paret!",     "Col\xb7lisi\xf3",
                   "Desaparegut","Mida:",       "Autopilot:", "S\xcd",  "NO",
                   "Prem un bot\xf3"),
    "LANG_AF":    ("SLANG",       "Begin",      "Weer speel", "POUSE",
                   "Gewen!",      "Game verby", "Muur!",      "Botsing",
                   "Verdwyn",     "Grootte:",   "Autopiloot:","AAN", "AF",
                   "Druk enige knop"),
    "LANG_SR":    ("ZMIJA",       "Pokreni",    "Ponovo",     "PAUZA",
                   "Pobeda!",     "Kraj igre",  "Zid!",       "Sudar",
                   "Nestala",     "Veli\u010dina:","Autopilot:","UK\u013d","ISK\u013d",
                   "Pritisni dugme"),
    "LANG_RU":    ("\u0417\u041c\u0415\u042e\u041a\u0410",
                   "\u0421\u0442\u0430\u0440\u0442",
                   "\u0417\u0430\u043d\u043e\u0432\u043e",
                   "\u041f\u0410\u0423\u0417\u0410",
                   "\u041f\u043e\u0431\u0435\u0434\u0430!",
                   "\u041f\u043e\u0440\u0430\u0436\u0435\u043d\u0438\u0435",
                   "\u0421\u0442\u0435\u043d\u0430",
                   "\u0421\u0430\u043c\u043e\u0441\u0442\u043e\u043b\u043a.",
                   "\u0418\u0441\u0447\u0435\u0437\u043b\u0430",
                   "\u0420\u0430\u0437\u043c\u0435\u0440:",
                   "\u0410\u0432\u0442\u043e\u043f\u0438\u043b\u043e\u0442:",
                   "\u0412\u041a\u041b",
                   "\u0412\u042b\u041a\u041b",
                   "\u041d\u0430\u0436\u043c\u0438\u0442\u0435 \u043a\u043d\u043e\u043f\u043a\u0443"),
    "LANG_UK":    ("\u0417\u041c\u0406\u042f",
                   "\u0421\u0442\u0430\u0440\u0442",
                   "\u0417\u043d\u043e\u0432\u0443",
                   "\u041f\u0410\u0423\u0417\u0410",
                   "\u041f\u0435\u0440\u0435\u043c\u043e\u0433\u0430!",
                   "\u041f\u043e\u0440\u0430\u0437\u043a\u0430",
                   "\u0421\u0442\u0456\u043d\u0430",
                   "\u0417\u0456\u0442\u043a\u043d\u0435\u043d\u043d\u044f",
                   "\u0417\u043d\u0438\u043a\u043b\u0430",
                   "\u0420\u043e\u0437\u043c\u0456\u0440:",
                   "\u0410\u0432\u0442\u043e\u043f\u0456\u043b\u043e\u0442:",
                   "\u0423\u0412\u041c\u041a",
                   "\u0412\u0418\u041c\u041a",
                   "\u041d\u0430\u0442\u0438\u0441\u043d\u0456\u0442\u044c \u043a\u043d\u043e\u043f\u043a\u0443"),
    "LANG_EL":    ("\u03a6\u0399\u0394\u0399",
                   "\u0395\u03bd\u03b1\u03c1\u03be\u03b7",
                   "\u039e\u03b1\u03bd\u03ac",
                   "\u03a0\u0391\u03a5\u03a3\u0397",
                   "\u039d\u03af\u03ba\u03b7!",
                   "\u0397\u03c4\u03c4\u03b1",
                   "\u03a4\u03bf\u03af\u03c7\u03bf\u03c2",
                   "\u0391\u03c5\u03c4\u03bf\u03c3\u03c5\u03b3\u03ba.",
                   "\u0395\u03be\u03b1\u03c6\u03b1\u03bd.",
                   "\u039c\u03ad\u03b3\u03b5\u03b8\u03bf\u03c2:",
                   "\u0391\u03c5\u03c4\u03cc\u03bc\u03b1\u03c4\u03bf:",
                   "\u0395\u039d",
                   "\u0391\u03a0\u039f",
                   "\u03a0\u03b1\u03c4\u03ae\u03c3\u03c4\u03b5 \u03ba\u03bf\u03c5\u03bc\u03c0\u03af"),
}

ORDER = [
    "LANG_EN","LANG_DE","LANG_FR","LANG_ES","LANG_IT",
    "LANG_PT_BR","LANG_PT_PT","LANG_ID","LANG_TR","LANG_VI",
    "LANG_NL","LANG_NO","LANG_DA","LANG_SV","LANG_FI",
    "LANG_PL","LANG_CS","LANG_RO","LANG_HU","LANG_CA",
    "LANG_AF","LANG_SR",
    "LANG_RU","LANG_UK",
    "LANG_EL",
]

def escape(s):
    """Encode string to safe C hex escapes, splitting where needed to avoid
    greedy hex parsing (e.g. \x9f followed by 'e' would be read as \x9fe)."""
    import re
    raw = s.encode("utf-8")
    result = []
    last_was_hex = False
    hex_chars = set("0123456789abcdefABCDEF")
    for byte in raw:
        if byte < 0x20 or byte > 0x7E or chr(byte) == '"' or chr(byte) == '\\':
            result.append(f"\\x{byte:02x}")
            last_was_hex = True
        else:
            if last_was_hex and chr(byte) in hex_chars:
                # Close and reopen the string literal to break hex greedy parse
                result.append('" "')
            result.append(chr(byte))
            last_was_hex = False
    return "".join(result)

lines = ['#include "i18n.h"', "",
         "const char * const STRINGS[LANG_COUNT][STR_COUNT] = {"]
for key in ORDER:
    strs = LANGS[key]
    lines.append(f"    /* {key} */")
    lines.append("    {")
    for s in strs:
        lines.append(f'        "{escape(s)}",')
    lines.append("    },")
lines.append("};")

with open(OUT, "w", encoding="ascii") as f:
    f.write("\n".join(lines) + "\n")

print(f"Written {OUT}")

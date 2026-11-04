#include "data.h"

#define H CF_HANDS
#define F CF_FLY
#define M CF_MOUNT
#define R CF_RIDER
#define U CF_UNDEAD
#define T CF_TIRELESS

const CreatureInfo CREATURES[CR_COUNT] = {
    /*                 name       ap  sta con com def flags        vp */
    [CR_WIZARD] =   {"WIZARD",   14, 30, 20, 4, 5, H | R, 50},
    [CR_GOBLIN] =   {"GOBLIN",   12, 20, 8, 4, 3, H | R, 4},
    [CR_DWARF] =    {"DWARF",    10, 30, 14, 6, 6, H | R, 7},
    [CR_WOLF] =     {"WOLF",     16, 24, 10, 5, 3, 0, 5},
    [CR_SKELETON] = {"SKELETON", 10, 99, 10, 5, 4, H | U | T, 6},
    [CR_BAT] =      {"GIANT BAT", 18, 20, 5, 2, 4, F, 3},
    [CR_HARPY] =    {"HARPY",    14, 20, 9, 4, 3, H | F, 5},
    [CR_UNICORN] =  {"UNICORN",  16, 30, 16, 6, 5, M, 8},
    [CR_TROLL] =    {"TROLL",    10, 40, 24, 8, 6, H | R, 12},
    [CR_GHOST] =    {"GHOST",    12, 99, 12, 5, 6, F | U | T, 9},
    [CR_PEGASUS] =  {"PEGASUS",  16, 30, 14, 4, 4, F | M, 8},
    [CR_DRAGON] =   {"DRAGON",   14, 50, 40, 12, 10, F, 25},
    [CR_OGRE] =     {"OGRE",     10, 40, 22, 9, 5, H, 11},
    [CR_SPIDER] =   {"SPIDER",   12, 30, 12, 6, 4, 0, 6},
    [CR_ZOMBIE] =   {"ZOMBIE",    8, 99, 14, 5, 3, H | U | T, 6},
};

const ObjectInfo OBJECTS[OB_COUNT] = {
    [OB_HEAL_POTION] = {"HEALING POTION"},
    [OB_MANA_POTION] = {"MANA POTION"},
    [OB_SWORD] = {"RUNE SWORD"},
    [OB_SHIELD] = {"SHIELD"},
};

const SpellInfo SPELLS[SP_COUNT] = {
    [SP_GOBLINS] = {"GOBLINS", SK_SUMMON, 2, 8, CR_GOBLIN, "SUMMON 2 GOBLINS WITH HANDS"},
    [SP_DWARF] = {"DWARF", SK_SUMMON, 1, 6, CR_DWARF, "SUMMON A STURDY DWARF"},
    [SP_WOLVES] = {"WOLVES", SK_SUMMON, 2, 10, CR_WOLF, "SUMMON 2 FAST WOLVES"},
    [SP_SKELETONS] = {"SKELETONS", SK_SUMMON, 2, 12, CR_SKELETON, "SUMMON 2 UNDEAD SKELETONS"},
    [SP_BAT] = {"GIANT BAT", SK_SUMMON, 1, 5, CR_BAT, "SUMMON A FLYING GIANT BAT"},
    [SP_HARPY] = {"HARPY", SK_SUMMON, 1, 9, CR_HARPY, "SUMMON A FLYING HARPY"},
    [SP_UNICORN] = {"UNICORN", SK_SUMMON, 1, 10, CR_UNICORN, "SUMMON A UNICORN TO RIDE"},
    [SP_TROLL] = {"TROLL", SK_SUMMON, 1, 15, CR_TROLL, "SUMMON A STRONG TROLL"},
    [SP_GHOST] = {"GHOST", SK_SUMMON, 1, 14, CR_GHOST, "SUMMON A FLYING UNDEAD GHOST"},
    [SP_PEGASUS] = {"PEGASUS", SK_SUMMON, 1, 14, CR_PEGASUS, "SUMMON A FLYING STEED"},
    [SP_DRAGON] = {"DRAGON", SK_SUMMON, 1, 30, CR_DRAGON, "SUMMON A MIGHTY DRAGON"},
    [SP_OGRE] = {"OGRE", SK_SUMMON, 1, 18, CR_OGRE, "SUMMON A BRUTISH OGRE"},
    [SP_BOLT] = {"LIGHTNING", SK_BOLT, 2, 10, 0, "4-10 DAMAGE, HURTS UNDEAD"},
    [SP_FIREBALL] = {"FIREBALL", SK_FIREBALL, 3, 18, 0, "3-8 DAMAGE, 1-4 AROUND IT"},
    [SP_SPARKS] = {"SPARKS", SK_SPARKS, 1, 4, 0, "2-5 DAMAGE, CHEAP"},
    [SP_HEAL] = {"HEAL", SK_HEAL, 1, 8, 0, "OWN CREATURE +10 CON"},
    [SP_INVIS] = {"INVISIBILITY", SK_INVIS, 2, 12, 0, "OWN CREATURE HIDDEN 3 TURNS"},
    [SP_QUICKEN] = {"QUICKEN", SK_QUICKEN, 2, 10, 0, "OWN CREATURE REGAINS AP"},
    [SP_TELEPORT] = {"TELEPORT", SK_TELEPORT, 3, 15, 0, "JUMP TO A SQUARE IN SIGHT"},
    [SP_BANISH] = {"BANISH", SK_BANISH, 3, 12, 0, "MAY DESTROY A WEAK CREATURE"},
};

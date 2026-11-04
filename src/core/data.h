/* Static tables: creature types, objects and spells. */
#ifndef MAGES_DATA_H
#define MAGES_DATA_H
#include "defs.h"

typedef enum {
    CR_WIZARD, CR_GOBLIN, CR_DWARF, CR_WOLF, CR_SKELETON, CR_BAT, CR_HARPY,
    CR_UNICORN, CR_TROLL, CR_GHOST, CR_PEGASUS, CR_DRAGON, CR_OGRE, CR_SPIDER,
    CR_ZOMBIE, CR_COUNT
} CreatureType;

enum {
    CF_HANDS = 1,    /* can carry objects, use doors */
    CF_FLY = 2,      /* can take off */
    CF_MOUNT = 4,    /* can be ridden */
    CF_RIDER = 8,    /* can ride a mount */
    CF_UNDEAD = 16,  /* only hurt by undead, magic weapons, spells */
    CF_TIRELESS = 32 /* never loses stamina */
};

typedef struct {
    const char *name;
    uint8_t ap, stamina, con, combat, defence;
    uint8_t flags;
    uint8_t vp; /* victory points for killing it */
} CreatureInfo;

extern const CreatureInfo CREATURES[CR_COUNT];

typedef enum { OB_HEAL_POTION, OB_MANA_POTION, OB_SWORD, OB_SHIELD, OB_COUNT } ObjectType;

typedef struct {
    const char *name;
} ObjectInfo;

extern const ObjectInfo OBJECTS[OB_COUNT];

typedef enum {
    SK_SUMMON, SK_BOLT, SK_FIREBALL, SK_SPARKS, SK_HEAL, SK_INVIS, SK_QUICKEN,
    SK_TELEPORT, SK_BANISH
} SpellKind;

typedef enum {
    SP_GOBLINS, SP_DWARF, SP_WOLVES, SP_SKELETONS, SP_BAT, SP_HARPY, SP_UNICORN,
    SP_TROLL, SP_GHOST, SP_PEGASUS, SP_DRAGON, SP_OGRE, SP_BOLT, SP_FIREBALL,
    SP_SPARKS, SP_HEAL, SP_INVIS, SP_QUICKEN, SP_TELEPORT, SP_BANISH, SP_COUNT
} SpellId;

typedef struct {
    const char *name;
    uint8_t kind;
    uint8_t level;
    uint8_t mana;
    uint8_t creature; /* for summons */
    const char *desc; /* one line for the message line */
} SpellInfo;

extern const SpellInfo SPELLS[SP_COUNT];

#define SPELL_RANGE 8
#define CAST_AP 4
#define ATTACK_AP 4
#define AIR_MOVE_AP 3
#define SIGHT_RANGE 8
#define START_MANA 60
#define MANA_REGEN 2

#endif

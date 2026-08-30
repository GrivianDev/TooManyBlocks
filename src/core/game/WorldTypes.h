#ifndef TOOMANYBLOCKS_WORLDTYPES_H
#define TOOMANYBLOCKS_WORLDTYPES_H

enum Zone {
    Surface,
    TwilightHollows,
    Vault,
    GreenMaw,
    ChasmsOfOld,
    MycelicWilds,
    Swamp,
    Deep,
    DyingInferno,
    FleshThatHates,
    Void,
    Core
};

enum Biome {
    // Surface
    Plains,
    Mountains,
    Ocean,
    Desert,

    // Twilight Hollows
    LushCaves,
    CrystalCanyons,
    UndergroundSeas,
    UndergroundRivers,

    // Transition Zone: The Vault
    VaultBiome,

    // Green Maw
    Jungle,
    MossCaves,

    // Chasms of Old
    Ruins,
    SpiderCaves,
    MiningSites,

    // Mycelic Wilds
    MushroomForest,
    GlowingFungiField,
    AberrantPlains,

    // Transition Zone: The Swamp
    SkiningForest,

    // The Deep
    WaterCaves,
    UnderwaterGrooves,
    Trenches,

    // Dying Inferno
    AshenWastes,
    SkeletonRuins,
    FurnaceSite,

    // Flesh That Hates
    Organism,
    Suffering,
    Dispair,

    // The Void
    FloatingIsles,
    EchoingLandscapes,

    // The Core
    CoreBiome
};

enum StructureType {
    Decoration,
    Regional,
    SuperStructure
};

enum BlockGenPriority {
    FILLER,
    MEDIUM,
    HIGH,
    OVERRIDE
};

#endif
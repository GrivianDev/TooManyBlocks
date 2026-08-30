#include "BlockToTextureMapping.h"

#include <stdexcept>

#define ADD_MAPPING(blockType, mappingdef) map.emplace(blockType, std::array<FaceInfo, 6>{{mappingdef}})

#define ALL_FACES(val)                     {val, 0}, {val, 0}, {val, 0}, {val, 0}, {val, 0}, {val, 0}
#define AXIS_BASED(x_axis, y_axis, z_axis) {x_axis, 0}, {x_axis, 0}, {y_axis, 0}, {y_axis, 0}, {z_axis, 0}, {z_axis, 0}
#define INDIVIDUAL_FACES(xp_face, xn_face, yp_face, yn_face, zp_face, zn_face) \
    {xp_face, 0}, {xn_face, 0}, {yp_face, 0}, {yn_face, 0}, {zp_face, 0}, {zn_face, 0}
#define TOP_BOTTOM_SIDES(top, bottom, sides) {sides, 0}, {sides, 0}, {top, 0}, {bottom, 0}, {sides, 0}, {sides, 0}
#define TOP_BOTTOM_WITH_SIDES(top, bottom, xp_face, xn_face, zp_face, zn_face) \
    {xp_face, 0}, {xn_face, 0}, {top, 0}, {bottom, 0}, {zp_face, 0}, {zn_face, 0}

BlockToTextureMap::BlockToTextureMap() {
    ADD_MAPPING(STONE, ALL_FACES(0U));
    ADD_MAPPING(GRASS, TOP_BOTTOM_SIDES(5U, 3U, 4U));
    ADD_MAPPING(DIRT, ALL_FACES(3U));
    ADD_MAPPING(SAND, ALL_FACES(6U));
    ADD_MAPPING(IRON_ORE, ALL_FACES(7U));
    ADD_MAPPING(COAL_ORE, ALL_FACES(8U));
    ADD_MAPPING(COPPER_ORE, ALL_FACES(9U));
    ADD_MAPPING(WATER, ALL_FACES(10U));
    ADD_MAPPING(GROUNDWEAVE, ALL_FACES(11U));
    ADD_MAPPING(MUD, ALL_FACES(12U));
    ADD_MAPPING(TITANIUM_ORE, ALL_FACES(13U));
    ADD_MAPPING(PYRITE_ORE, ALL_FACES(14U));
    ADD_MAPPING(OPAL_ORE, ALL_FACES(15U));
    ADD_MAPPING(AMETHYST_ORE, ALL_FACES(16U));
    ADD_MAPPING(AMETHYST, ALL_FACES(17U));
    ADD_MAPPING(OAK_WOOD, TOP_BOTTOM_SIDES(19U, 19U, 18U));
    ADD_MAPPING(OAK_LEAVES, ALL_FACES(20U));
    ADD_MAPPING(OAK_PLANKS, ALL_FACES(21U));
    ADD_MAPPING(CRACKED_VAULT_BLOCK, ALL_FACES(22U));
    ADD_MAPPING(VAULT_BLOCK, ALL_FACES(23U));
    ADD_MAPPING(SEALED_GOLD, ALL_FACES(24U));
    ADD_MAPPING(OBSIDIUM, ALL_FACES(25U));
    ADD_MAPPING(ANDESITE, ALL_FACES(26U));
    ADD_MAPPING(VERDITE_ORE, ALL_FACES(27U));
    ADD_MAPPING(AMBER_RESIN, ALL_FACES(28U));
    ADD_MAPPING(CHLORITE_ORE, ALL_FACES(29U));
    ADD_MAPPING(MOSS, ALL_FACES(30U));
    ADD_MAPPING(MOSSY_ANDESITE, ALL_FACES(31U));
    ADD_MAPPING(GRANITE, ALL_FACES(32U));
    ADD_MAPPING(RUNED_SILVER_ORE, ALL_FACES(33U));
    ADD_MAPPING(AETHERSTONE_ORE, ALL_FACES(34U));
    ADD_MAPPING(PHANTOM_ORE, ALL_FACES(35U));
    ADD_MAPPING(GRANITE_BRICK, ALL_FACES(36U));
}

FaceInfo BlockToTextureMap::getInfo(uint16_t blockType, AxisDirection dir) const {
    auto it = map.find(blockType);
    if (it == map.end()) {
        throw std::out_of_range("No texture index mapping for this block type");
    }
    return it->second[static_cast<size_t>(dir)];
}

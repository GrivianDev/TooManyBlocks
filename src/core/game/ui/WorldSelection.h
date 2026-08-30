#ifndef TOOMANYBLOCKS_WORLDSELECTION_H
#define TOOMANYBLOCKS_WORLDSELECTION_H

#include <json/JsonParser.h>

#include <string>

#include "engine/ui/Ui.h"
#include "foundation/threading/Future.h"

namespace UI {
    class WorldSelection : public Widget {
    private:
        Future<Json::JsonArray> m_worldInfos;
        Json::JsonValue* m_selectedWorld;

        char m_newWorldName[64];
        uint32_t m_newWorldSeed;

        void randomSeed();

        void loadWorldInfo();

        void createNewWorld(const std::string& name);

        void renameWorld(const std::string& name);

        void deleteWorld();

    public:
        WorldSelection() : m_selectedWorld(nullptr) {}

        virtual ~WorldSelection() = default;

        void render() override;
    };
}  // namespace UI

#endif

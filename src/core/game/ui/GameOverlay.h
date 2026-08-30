#ifndef TOOMANYBLOCKS_GAMEOVERLAY_H
#define TOOMANYBLOCKS_GAMEOVERLAY_H

#include "engine/ui/Ui.h"
#include "game/ui/StatScreen.h"

namespace UI {
    class GameOverlay : public Widget {
    private:
        StatScreen m_statScreen;

        bool m_showMouse;
        bool m_showStats;

    public:
        GameOverlay() : m_showMouse(false), m_showStats(false) {}
        virtual ~GameOverlay() = default;

        void render() override;
    };
}  // namespace UI

#endif
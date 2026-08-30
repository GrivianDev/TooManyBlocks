#ifndef TOOMANYBLOCKS_PAUSEMENU_H
#define TOOMANYBLOCKS_PAUSEMENU_H

#include "engine/ui/Ui.h"

namespace UI {
    class PauseMenu : public Widget {
    private:
        bool m_escWasReleased;

    public:
        PauseMenu() : m_escWasReleased(false) {}
        virtual ~PauseMenu() = default;

        void render() override;
    };
}  // namespace UI

#endif
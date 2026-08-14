#ifndef TOOMANYBLOCKS_SETTINGSMENU_H
#define TOOMANYBLOCKS_SETTINGSMENU_H

#include "engine/ui/Ui.h"
#include "platform/audio/AudioEngine.h"

namespace UI {
    class SettingsMenu : public Widget {
    private:
        bool m_unlimitedFrameRate;
        std::vector<AudioDevice> m_outputDevices;

    public:
        SettingsMenu();
        virtual ~SettingsMenu() = default;

        void render() override;
    };
}  // namespace UI

#endif

#ifndef TOOMANYBLOCKS_ABOUTSCREEN_H
#define TOOMANYBLOCKS_ABOUTSCREEN_H

#include "engine/ui/Ui.h"
#include "foundation/threading/Future.h"

namespace UI {
    class AboutScreen : public Widget {
    private:
        Future<std::string> m_content;

        void startLoadingContent();

        void pullErrorFromContentLoad();

    public:
        AboutScreen() = default;
        virtual ~AboutScreen() = default;

        void render() override;
    };
}  // namespace UI

#endif

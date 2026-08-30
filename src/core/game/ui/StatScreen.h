#ifndef TOOMANYBLOCKS_STATSCREEN_H
#define TOOMANYBLOCKS_STATSCREEN_H

#include "engine/ui/Ui.h"
#include "engine/rendering/passes/debug/DebugReport.h"

namespace UI {
    class StatScreen : public Widget {
    private:
        float m_renderDebugReportAccumulator;
        DebugReport m_report;

    public:
        StatScreen() : m_renderDebugReportAccumulator(0) {};
        virtual ~StatScreen() = default;

        void render() override;
    };
}  // namespace UI

#endif
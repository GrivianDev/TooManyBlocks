#ifndef TOOMANYBLOCKS_MANAGER_H
#define TOOMANYBLOCKS_MANAGER_H

#include <functional>
#include <string>
#include <vector>

#include "engine/ui/Widget.h"
#include "engine/ui/fonts/FontUtil.h"

namespace UI {
    class Manager {
        std::unordered_map<std::string, std::function<Widget*()>> m_widgetFactory;

        FontPool m_fontPool;

        Widget* m_currentWidget;
        Widget* m_nextWidget;
        std::string m_currentWidgetName;
        std::string m_nextWidgetName;
        std::vector<std::string> m_navigationStack;

    public:
        Manager();
        ~Manager();

        void init();

        void shutdown();

        void renderFrame();

        void registerWidget(const std::string& widgetName, std::function<Widget*()> createFn);

        bool navigateTo(const std::string& widgetName);

        bool navigateToReplacement(const std::string& widgetName);
        
        bool navigateBackTo(const std::string& widgetName);

        bool navigateBack();

        void clearNavigationHistory();

        FontData getFont(float requestedSize);
    };
}  // namespace UI

#endif

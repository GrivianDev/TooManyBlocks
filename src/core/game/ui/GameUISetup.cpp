#include "GameUISetup.h"

#include "engine/ui/Ui.h"
#include "game/ui/AboutScreen.h"
#include "game/ui/GameOverlay.h"
#include "game/ui/MainMenu.h"
#include "game/ui/PauseMenu.h"
#include "game/ui/SettingsMenu.h"
#include "game/ui/WorldSelection.h"

void GameUI::setup() {
    UI::init();

    UI::registerWidget<UI::MainMenu>("MainMenu");
    UI::registerWidget<UI::SettingsMenu>("SettingsMenu");
    UI::registerWidget<UI::GameOverlay>("GameOverlay");
    UI::registerWidget<UI::WorldSelection>("WorldSelection");
    UI::registerWidget<UI::PauseMenu>("PauseMenu");
    UI::registerWidget<UI::AboutScreen>("AboutScreen");

    UI::navigateTo("MainMenu");
}
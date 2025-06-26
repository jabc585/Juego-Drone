#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <string>

class UIManager {
public:
    UIManager();
    void renderUI();
    void showPauseMenu();
    void showSettingsMenu();
    // Posible método para actualizar indicadores (minimapa, batería, etc.)
    void updateIndicators();
};

#endif


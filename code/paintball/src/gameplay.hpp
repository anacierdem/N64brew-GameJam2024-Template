#ifndef __GAMEPLAY_H
#define __GAMEPLAY_H

#include <libdragon.h>

#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3danim.h>

#include <memory>
#include <vector>

#include "../../../core.h"
#include "../../../minigame.h"

#include "constants.hpp"
#include "wrappers.hpp"
#include "bullet.hpp"
#include "player.hpp"

class GameplayController
{
    private:
        BulletController bulletController;

        U::Timer timer;

        U::T3DModel model;

        bool gameFinished;
        void gameOver();

        std::vector<PlayerOtherData> playerOtherData;
        std::vector<PlayerGameplayData> playerGameplayData;

        // Player calculations
        void simulatePhysics(PlayerGameplayData &gameplayData, PlayerOtherData &other, uint32_t id, float deltaTime);
        void handleActions(PlayerGameplayData &gameplayData, uint32_t id);
        void renderPlayer(PlayerGameplayData &gameplayData, PlayerOtherData &other, uint32_t id, T3DViewport &viewport, float deltaTime);
        void renderPlayer2ndPass(PlayerGameplayData &playerGameplay, PlayerOtherData &playerOther, uint32_t id);

    public:
        GameplayController();
        void render(float deltaTime, T3DViewport &viewport);
        void render2ndPass();
        void fixedUpdate(float deltaTime);
};

#endif // __GAMEPLAY_H
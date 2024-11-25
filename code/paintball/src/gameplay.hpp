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
#include "map.hpp"
#include "ui.hpp"
#include "gamestate.hpp"

// Same range as analog input, max value that can be generated by the controller
constexpr float ForceLimit = 60.f;
constexpr float PlayerInvMass = 10;
constexpr float SpeedLimit = 80.f;
constexpr float BulletOffset = 15.f;
constexpr float CooldownPerSecond = 1.f;
constexpr float TempPerBullet = 0.3f;
constexpr float OverheatPenalty = 1.0f;

class GameplayController
{
    private:
        // Resources
        BulletController bulletController;
        U::T3DModel model;
        U::T3DModel shadowModel;
        U::Sprite arrowSprite {sprite_load("rom:/paintball/arrow.ia4.sprite"), sprite_free};

        // Player data
        std::vector<Player::OtherData> playerOtherData;
        std::vector<Player::GameplayData> playerGameplayData;

        // Controllers
        std::shared_ptr<MapRenderer> map;

        // Player calculations
        void simulatePhysics(Player::GameplayData &gameplayData, Player::OtherData &other, uint32_t id, float deltaTime);
        void handleActions(Player::GameplayData &gameplayData, uint32_t id, bool enabled);

    public:
        GameplayController(std::shared_ptr<MapRenderer> map, std::shared_ptr<UIRenderer> ui);
        void newRound();
        const std::vector<Player::GameplayData> &getPlayerGameplayData() const;

        void render(float deltaTime, T3DViewport &viewport, GameState &state);
        void renderUI();
        void fixedUpdate(float deltaTime, GameState &state);
};

#endif // __GAMEPLAY_H
#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#include <libdragon.h>
#include "../../../core.h"

// Rendering
constexpr int SmallFont = 1;
constexpr int MediumFont = 2;
constexpr int BigFont = 3;

constexpr int ScreenWidth = 320;
constexpr int ScreenHeight = 240;

// Gameplay
constexpr int PlayerCount = MAXPLAYERS;
constexpr float LastOneStandingTime = 15;
constexpr int RoundCount = 5;

constexpr float PlayerRadius = 13;
constexpr float AIBulletRange = 64 * PlayerRadius * PlayerRadius;

constexpr float BulletVelocity = 300;

// AUDIO
constexpr int FireAudioChannel = 10;
constexpr int HitAudioChannel = 11;
constexpr int GeneralPurposeAudioChannel = 12;

// CORE
// Same range as analog input, max value that can be generated by the controller
constexpr float ForceLimit = 60.f;

#endif // __CONSTANTS_H
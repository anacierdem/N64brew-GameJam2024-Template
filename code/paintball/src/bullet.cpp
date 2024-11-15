#include "bullet.hpp"

Bullet::Bullet() :
    pos {0},
    prevPos {0},
    velocity {0},
    team {PLAYER_1},
    matFP({(T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP)), free_uncached}) {}

BulletController::BulletController(std::shared_ptr<MapRenderer> map) :
    newBulletCount(0),
    model({
        t3d_model_load("rom:/paintball/bullet.t3dm"),
        t3d_model_free
    }),
    block({nullptr, rspq_block_free}),
    map(map) {
        assertf(model.get(), "Bullet model is null");

        rspq_block_begin();
            t3d_model_draw(model.get());

            // Outline
            t3d_state_set_vertex_fx(T3D_VERTEX_FX_OUTLINE, (int16_t)5, (int16_t)5);
                rdpq_set_prim_color(RGBA32(0, 0, 0, 0xFF));

                // Is this necessary?
                rdpq_sync_pipe();

                rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
                t3d_state_set_drawflags((T3DDrawFlags)(T3D_FLAG_CULL_FRONT | T3D_FLAG_DEPTH));

                T3DModelIter it = t3d_model_iter_create(model.get(), T3D_CHUNK_TYPE_OBJECT);
                while(t3d_model_iter_next(&it))
                {
                    t3d_model_draw_object(it.object, nullptr);
                }
            t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0, 0);
        block = U::RSPQBlock(rspq_block_end(), rspq_block_free);
    }

void BulletController::render(float deltaTime) {
    const color_t colors[] = {
        PLAYERCOLOR_1,
        PLAYERCOLOR_2,
        PLAYERCOLOR_3,
        PLAYERCOLOR_4,
    };

    double interpolate = core_get_subtick();

    for (auto& bullet : bullets)
    {
        if (bullet.velocity.v[0] != 0 || bullet.velocity.v[1] != 0 || bullet.velocity.v[2] != 0) {
            assertf(bullet.matFP.get(), "Bullet matrix is null");
            assertf(block.get(), "Bullet dl is null");

            T3DVec3 currentPos {0};
            t3d_vec3_lerp(currentPos, bullet.prevPos, bullet.pos, interpolate);

            t3d_mat4fp_from_srt_euler(
                bullet.matFP.get(),
                T3DVec3 {0.2f, 0.2f, 0.2f},
                // TODO: add some random rotation
                T3DVec3 {0.0f, 0.0f, 0.0f},
                T3DVec3 {currentPos.v[0], currentPos.v[1], currentPos.v[2]}
            );

            t3d_matrix_push(bullet.matFP.get());
                rdpq_set_prim_color(colors[bullet.team]);
                rspq_block_run(block.get());
            t3d_matrix_pop(1);
        }
    }
}

// TODO: shift everything to left to fill in holes & gain performance
void BulletController::killBullet(Bullet &bullet) {
    assertf(map.get(), "Map renderer is null");
    bullet.velocity.v[0] = 0;
    bullet.velocity.v[1] = 0;
    bullet.velocity.v[2] = 0;
    map->splash(bullet.pos.v[0], bullet.pos.v[2], bullet.team);
}

/**
 * Returns true if the player changed team
 */
bool BulletController::applyDamage(PlayerGameplayData &gameplayData, PlyNum team) {
    auto currentVal = gameplayData.health[team];

    // Already on same team
    if (gameplayData.team == team || currentVal == MaxHealth) return false;

    gameplayData.health[team] = 0;

    auto result = std::max_element(gameplayData.health.begin(), gameplayData.health.end(), [](int a, int b)
    {
        if (a == b) return (static_cast<float>(rand()) / RAND_MAX) > 0.5f;
        return a < b;
    });

    *result -= Damage;
    if (*result < 0) *result = 0;

    gameplayData.health[team] = currentVal + Damage;
    if (gameplayData.health[team] >= MaxHealth) {
        gameplayData.health[team] = MaxHealth;
        gameplayData.team = team;
        return true;
    }
    return false;
}

void BulletController::simulatePhysics(float deltaTime, Bullet &bullet) {
    bullet.prevPos = bullet.pos;

    T3DVec3 velocityDiff = {0};
    t3d_vec3_scale(velocityDiff, T3DVec3 {0, Gravity, 0}, deltaTime);
    t3d_vec3_add(bullet.velocity, bullet.velocity, velocityDiff);

    T3DVec3 posDiff = {0};
    t3d_vec3_scale(posDiff, bullet.velocity, deltaTime);
    t3d_vec3_add(bullet.pos, bullet.pos, posDiff);

    if (bullet.pos.v[0] > 1000.f || bullet.pos.v[0] < -1000.f ||
        bullet.pos.v[2] > 1000.f || bullet.pos.v[2] < -1000.f ||
        bullet.pos.v[1] < 0.f) {
        killBullet(bullet);
    }
}

/**
 * Returns an array of players changed status this tick
 */
std::array<bool, PlayerCount> BulletController::fixedUpdate(float deltaTime, std::vector<PlayerGameplayData> &gameplayData) {
    std::array<bool, PlayerCount> playerHitStatus {0};

    for (auto& bullet : bullets)
    {
        // TODO: this will prevent firing once every slot is occupied
        // This is a free bullet slot, fill it if we have pending bullets
        if (bullet.velocity.v[0] == 0 && bullet.velocity.v[1] == 0 && bullet.velocity.v[2] == 0) {
            if (newBulletCount > 0) {
                int bulletIndex = --newBulletCount;
                bullet.pos = newBullets[bulletIndex].pos;
                bullet.prevPos = bullet.pos;
                bullet.velocity = newBullets[bulletIndex].velocity;
                bullet.team = newBullets[bulletIndex].team;
            } else {
                continue;
            }
        }

        simulatePhysics(deltaTime, bullet);

        int i = 0;
        // TODO: if we could delegate this to player.cpp, b/c collider doesn't belong here
        for (auto& player : gameplayData)
        {
            auto colliderPosition = T3DVec3{player.pos.v[0], player.pos.v[1] + BulletHeight, player.pos.v[2]};
            auto colliderPosition2 = T3DVec3{player.pos.v[0], player.pos.v[1] + BulletHeight-PlayerRadius, player.pos.v[2]};

            if (t3d_vec3_distance2(colliderPosition, bullet.pos) < PlayerRadius * PlayerRadius ||
                t3d_vec3_distance2(colliderPosition2, bullet.pos) < PlayerRadius * PlayerRadius) {
                playerHitStatus[i] = applyDamage(player, bullet.team);
                killBullet(bullet);
            }
            i++;
        }
    }

    return playerHitStatus;
}

void BulletController::fireBullet(const T3DVec3 &pos, const T3DVec3 &velocity, PlyNum team) {
    if (newBulletCount >= newBullets.size()) return;

    newBullets[newBulletCount].pos = pos;
    newBullets[newBulletCount].velocity = velocity;
    newBullets[newBulletCount].team = team;
    newBulletCount++;
}
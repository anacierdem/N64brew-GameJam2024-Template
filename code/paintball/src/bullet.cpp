#include "bullet.hpp"

Bullet::Bullet() :
    pos {0},
    prevPos {0},
    velocity {0},
    team {PLAYER_1},
    owner {PLAYER_1},
    matFP({(T3DMat4FP*)malloc_uncached(sizeof(T3DMat4FP)),free_uncached}) { }

Bullet::Bullet(T3DVec3 pos, T3DVec3 velocity, PlyNum owner, PlyNum team) :
    pos {pos},
    prevPos {pos},
    velocity {velocity},
    team {team},
    owner {owner},
    matFP({nullptr, free_uncached}) { }

Bullet& Bullet::operator=(Bullet& rhs) {
    if (this == &rhs) return *this;
    pos = rhs.pos;
    prevPos = rhs.prevPos;
    velocity = rhs.velocity;
    team = rhs.team;
    owner = rhs.owner;
    return *this;
};

BulletController::BulletController(std::shared_ptr<MapRenderer> map, std::shared_ptr<UIRenderer> ui) :
    newBulletCount(0),
    model({
        t3d_model_load("rom:/paintball/bullet.t3dm"),
        t3d_model_free
    }),
    block({nullptr, rspq_block_free}),
    map(map),
    ui(ui) {
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

    for (auto bullet = bullets.begin(); bullet != bullets.end(); ++bullet) {
        assertf(bullet->matFP.get(), "Bullet matrix is null");
        assertf(block.get(), "Bullet dl is null");

        T3DVec3 currentPos {0};
        t3d_vec3_lerp(currentPos, bullet->prevPos, bullet->pos, interpolate);

        t3d_mat4fp_from_srt_euler(
            bullet->matFP.get(),
            T3DVec3 {0.2f, 0.2f, 0.2f},
            // TODO: add some random rotation
            T3DVec3 {0.0f, 0.0f, 0.0f},
            T3DVec3 {currentPos.v[0], currentPos.v[1], currentPos.v[2]}
        );

        t3d_matrix_push(bullet->matFP.get());
            rdpq_set_prim_color(colors[bullet->team]);
            rspq_block_run(block.get());
        t3d_matrix_pop(1);
    }
}

/**
 * Returns true if the player changed team
 */
bool BulletController::processHit(Player::GameplayData &gameplayData, PlyNum team) {
    // Already on same team, heal
    if (gameplayData.team == team) {
        gameplayData.firstHit = team;
        return false;
    }

    if (gameplayData.firstHit == team) {
        gameplayData.team = team;
        gameplayData.fragCount = 0;
        return true;
    }

    gameplayData.firstHit = team;

    return false;
}

/**
 * Returns true if bullet is dead
 */
bool BulletController::simulatePhysics(float deltaTime, Bullet &bullet) {
    bullet.prevPos = bullet.pos;

    T3DVec3 velocityDiff = {0};
    t3d_vec3_scale(velocityDiff, T3DVec3 {0, Gravity, 0}, deltaTime);
    t3d_vec3_add(bullet.velocity, bullet.velocity, velocityDiff);

    T3DVec3 posDiff = {0};
    t3d_vec3_scale(posDiff, bullet.velocity, deltaTime);
    t3d_vec3_add(bullet.pos, bullet.pos, posDiff);

    if (bullet.pos.v[1] < 0.f) {
        return true;
    }
    return false;
}

void BulletController::fixedUpdate(float deltaTime, std::vector<Player::GameplayData> &gameplayData) {
    assertf(map.get(), "Map renderer is null");
    // TODO: Couldn't find a better way to convert the comparison to != here
    for (auto bullet = bullets.begin(); bullet < bullets.end(); ++bullet) {
        bool isDead = simulatePhysics(deltaTime, *bullet);
        if (isDead) {
            map->splash(bullet->pos.v[0], bullet->pos.v[2], bullet->team);
            bullets.remove(bullet);
            continue;
        }

        int i = 0;
        // TODO: if we could delegate this to player.cpp, b/c collider doesn't belong here
        for (auto& player : gameplayData)
        {
            // Don't hit the player that fired the bullet
            if (i == bullet->owner) {
                i++;
                continue;
            }

            // 2D distance
            auto dist2 =
                (player.pos.v[0] - bullet->pos.v[0]) * (player.pos.v[0] - bullet->pos.v[0]) +
                (player.pos.v[2] - bullet->pos.v[2]) * (player.pos.v[2] - bullet->pos.v[2]);

            if (dist2 < PlayerRadius * PlayerRadius) {
                // TODO: delegate to Player
                bool hit = processHit(player, bullet->team);
                gameplayData[bullet->owner].fragCount += hit ? 1 : 0;

                ui->registerHit(HitMark {bullet->pos, bullet->owner});
                map->splash(bullet->pos.v[0], bullet->pos.v[2], bullet->team);
                bullets.remove(bullet);

                // No need to check other players, we don't have the bullet anymore
                break;
            }
            i++;
        }
    }
}

void BulletController::fireBullet(const T3DVec3 &pos, const T3DVec3 &velocity, PlyNum owner, PlyNum team) {
    // TODO: this will prevent firing once every slot is occupied
    bullets.add(Bullet {pos, velocity, owner, team});
}
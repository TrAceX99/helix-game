#include "helix.h"

short showGun = false;
short showTargets = false;
short wireframe = true;
short strafe = false;
short strafeLeft = false;
short strafeRight = false;

// Default states for sprite objects
// { x, y, visible, animIndex, texture, animTimer }
const Sprite d_sprites[SPRITES_MAX] =
{
    // Barrels
    {21.5f, 1.5f, 1, 0, 0, 0},
    {15.5f, 1.5f, 1, 0, 0, 0},
    {16.0f, 1.8f, 1, 0, 0, 0},
    {16.2f, 1.2f, 1, 0, 0, 0},
    {9.5f, 15.5f, 1, 0, 0, 0},
    {10.0f, 15.1f, 1, 0, 0, 0},
    {10.5f, 15.8f, 1, 0, 0, 0},

    // Monsters
    {21.5f, 4.5f, 1, 0, 7, 0},
    {17.5f, 4.5f, 1, 0, 7, 0},
    {3.5f, 15.5f, 1, 0, 7, 0},
    {3.5f, 19.5f, 1, 0, 7, 0},
    {10.5f, 4.5f, 1, 0, 7, 0}
};

Enemy g_enemies[ENEMIES_MAX];  // Sprite object list
Sprite g_sprites[SPRITES_MAX]; // Enemy object list

Input g_input; // Player input
int g_oldKey;  // Key being held

float g_zBuffer[SCREEN_W];      // Distance to wall for each screen stripe
int g_spriteOrder[SPRITES_MAX]; // Arrays used to sort the sprites
float g_spriteDistance[SPRITES_MAX];

Camera g_mainCamera;                             // Main camera (player)
uint8_t g_screenBuffer[SCREEN_W * SCREEN_H / 2]; // Buffer used for rendering
uint8_t g_emptyBuffer[SCREEN_W * SCREEN_H / 2];  // Empty buffer containing color black

int g_time = 0;      // get_millis() at the beggining of frame
int g_timeOld = 0;   // get_millis() at the beggining of previous frame
int g_startTime = 0; // get_millis() at the game start

int g_target = -1;         // Index of enemy that the player is aiming at, -1 is none
int g_reloadAnimTimer = 0; // Timer for player gun reload

int g_kills = 0;        // Player kill count
int g_gameOverWin = 0;  // Game over win flag
int g_gameOverLoss = 0; // Game over lose flag

const int g_animms = 250;                   // One animation frame duration
const int c_reloadms = 300;                 // Player gun reload duration
const int c_reloadAnim = 200;               // Player shot animation duration
const float c_enemyViewDistance = 15.0f;    // Max distance enemies will chase the player at
const float c_enemyDamageDistance = 1.0f;   // Enemy meele attack range
const float c_enemySpeed = 1.0f / 1000;     // Enemy movement speed
const float c_playerSpeed = 1.5f / 1000;    // Player movement speed
const float c_playerRotSpeed = 1.0f / 1000; // Player rotation speed

int main()
{
    char stat[100];
    // Initialization
    init_stdio();
    video_mode(1);
    memset(g_emptyBuffer, 0, SCREEN_W * SCREEN_H / 2);
    g_startTime = get_millis();

    // Game loop
    while (1)
    {
        // Setting up a new game
        cls(0);
        init_camera();
        init_enemies();
        g_gameOverWin = 0;
        g_gameOverLoss = 0;

        // Main game loop
        while (g_gameOverWin == 0 && g_gameOverLoss == 0)
        {
            g_timeOld = g_time;
            g_time = get_millis();

            input_update();
            player_update();
            if (showTargets) enemy_update();
            renderer_update(&g_mainCamera);
            if (g_time - g_startTime < 10000)
                draw_tutorial();

            if (g_input.restart)
                break;
            if (g_input.quit)
                return 0;
            // print stats
            sprintf(stat, "FPS:%d,wrfrm:%s,trgts:%s,gun:%s", (1000/(g_time - g_timeOld)), (wireframe?"ON":"OFF"), (showTargets?"ON":"OFF"), (showGun?"ON":"OFF"));
            draw(5, 10, WHITE, stat);
        }

        // Draw game over screen
        cls(0);
        if (g_gameOverWin)
        {
            draw(110, 100, WHITE, "LEVEL CLEARED");
        }
        else if (g_gameOverLoss)
        {
            draw(130, 100, WHITE, "YOU DIED");
        }
        draw(90, 120, WHITE, "PRESS R TO RESTART");

        // Wait for keyboard input
        while (g_input.restart == 0)
        {
            input_update();
            if (g_input.quit)
                return 0;
        }
    }

    return 0;
}

void draw_tutorial()
{
    if (g_time - g_startTime < 5000)
    {
        draw(10, 220, WHITE, "USE ARROW KEYS TO MOVE, SPACE TO SHOOT");
    }
    else
    {
        draw(80, 210, WHITE, "KILL ALL THE ENEMIES");
        draw(40, 220, WHITE, "AND DESTROY THE BARRELS TO WIN");
    }
}

void init_enemies()
{
    g_kills = 0;

    // Copy sprite defaults
    for (int i = 0; i < SPRITES_MAX; i++)
    {
        g_sprites[i] = d_sprites[i];
    }

    // Init enemy defaults
    for (int i = 0; i < ENEMIES_MAX; i++)
    {
        g_enemies[i].sprite = &g_sprites[i];
        g_enemies[i].active = 1;
        g_enemies[i].hp = g_enemies[i].sprite->texture == 0 ? 1 : 4;
        g_enemies[i].isStatic = g_enemies[i].sprite->texture == 0 ? 1 : 0;
        g_enemies[i].moving = 0;
    }
}

void enemy_update()
{
    for (int i = 0; i < ENEMIES_MAX; i++)
    {
        Enemy *enemy = &g_enemies[i];

        if (enemy->sprite->visible == 0)
            continue;

        // Animations
        if (g_time >= enemy->sprite->animTimer + g_animms)
        {
            if (enemy->sprite->animIndex > 2)
            {
                // Dying animation
                enemy->sprite->animIndex++;
                enemy->sprite->animTimer = g_time;
                // 4 frames
                if (enemy->sprite->animIndex > 6)
                {
                    enemy->sprite->visible = 0;
                }
            }
            else if (enemy->sprite->animIndex == 2)
            {
                // Hit animation
                enemy->sprite->animIndex = 0;
                enemy->sprite->animTimer = g_time;
            }
            else
            {
                // Idle animation
                if (enemy->moving)
                {
                    enemy->sprite->animIndex = enemy->sprite->animIndex == 0 ? 1 : 0;
                    enemy->sprite->animTimer = g_time;
                }
            }
        }

        if (enemy->active == 0 || enemy->isStatic != 0)
            continue;

        // AI behaviour, move only if not being hit or dying
        if (enemy->sprite->animIndex < 2)
        {
            // Calculate distance to player
            // { distX, distY } is a vector pointing from enemy to player
            float distX = g_mainCamera.posX - enemy->sprite->x;
            float distY = g_mainCamera.posY - enemy->sprite->y;
            float distToPlayer = sqrtf(distX * distX + distY * distY);

            // Use DDA to find the wall we are looking at
            int hit = 0, side, stepX, stepY;
            int mapX = (int)(enemy->sprite->x);
            int mapY = (int)(enemy->sprite->y);
            float sideDistX, sideDistY, distToWall;
            float deltaDistX = ABS(1.0f / distX);
            float deltaDistY = ABS(1.0f / distY);
            if (distX < 0)
            {
                stepX = -1;
                sideDistX = (enemy->sprite->x - mapX) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (mapX + 1 - enemy->sprite->x) * deltaDistX;
            }
            if (distY < 0)
            {
                stepY = -1;
                sideDistY = (enemy->sprite->y - mapY) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (mapY + 1 - enemy->sprite->y) * deltaDistY;
            }
            while (hit == 0)
            {
                // Jump to next map square, either in x-direction, or in y-direction
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                }
                else
                {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }
                // Check if ray has hit a wall
                if (d_worldMap[mapX][mapY] > 0)
                    hit = 1;
            }

            // Compare coordinates of wall the enemy is looking at when looking at player and player coordinates
            // In case the player is in front of the wall -> we are looking at the player
            int playerMapX = (int)(g_mainCamera.posX);
            int playerMapY = (int)(g_mainCamera.posY);
            int playerToWallX = mapX - playerMapX;
            int playerToWallY = mapY - playerMapY;
            int lookingAtPlayer = 1;            // Assume we are looking at player
            if (distX < 0 && playerToWallX > 0) // Check for each direction
                lookingAtPlayer = 0;
            else if (distX > 0 && playerToWallX < 0)
                lookingAtPlayer = 0;
            if (distY < 0 && playerToWallY > 0)
                lookingAtPlayer = 0;
            else if (distY > 0 && playerToWallY < 0)
                lookingAtPlayer = 0;

            // If we are looking at the player (and not a wall), and he is in view distance
            if (lookingAtPlayer && distToPlayer < c_enemyViewDistance)
            {
                // Move towards the player
                enemy->sprite->x += distX / distToPlayer * c_enemySpeed * (g_time - g_timeOld);
                enemy->sprite->y += distY / distToPlayer * c_enemySpeed * (g_time - g_timeOld);
                enemy->moving = 1;

                char tmp[100];
                sprintf(tmp, "dist:%f", distToPlayer);
                draw(10, 20, WHITE, tmp);

                // Damage player if too close
                if (distToPlayer < c_enemyDamageDistance)
                {
                    g_gameOverLoss = 1;
                    draw(10, 20, GREEN, "END");
                }
            }
            else
            {
                enemy->moving = 0;
            }
        }
    }
}

// Divide with rounding down
uint32_t shift_div_with_round_down(uint32_t num, uint32_t shift)
{
    uint32_t d = num >> shift;
    return d;
}

// Divide with rounding up
uint32_t shift_div_with_round_up(uint32_t num, uint32_t shift)
{
    uint32_t d = num >> shift;
    uint32_t mask = (1 << shift) - 1;
    if ((num & mask) != 0)
    {
        d++;
    }
    return d;
}

void player_update()
{
    // Calculate speeds by using frame delta time, this allows for framerate-independent movement
    float moveSpeed = (g_time - g_timeOld) * c_playerSpeed;
    float rotationSpeed = (g_time - g_timeOld) * c_playerRotSpeed;
    //float rotationSpeed = /*(g_time - g_timeOld)*/ 75.0f * c_playerRotSpeed;

    float playerWidth = 0.4f;

    // Temp variables for better code readability
    float posX = g_mainCamera.posX;
    float posY = g_mainCamera.posY;
    float dirX = g_mainCamera.dirX;
    float dirY = g_mainCamera.dirY;
    float planeX = g_mainCamera.planeX;
    float planeY = g_mainCamera.planeY;

    // Move forward if no wall in front of the player
    if (g_input.up)
    {
        if (d_worldMap[(int)(posX + dirX * moveSpeed + dirX * playerWidth)][(int)(posY)] == 0)
            posX += dirX * moveSpeed;
        if (d_worldMap[(int)(posX)][(int)(posY + dirY * moveSpeed + dirY * playerWidth)] == 0)
            posY += dirY * moveSpeed;
    }
    // Move backwards if no wall behind the player
    if (g_input.down)
    {
        if (d_worldMap[(int)(posX - dirX * moveSpeed - dirX * playerWidth)][(int)(posY)] == 0)
            posX -= dirX * moveSpeed;
        if (d_worldMap[(int)(posX)][(int)(posY - dirY * moveSpeed - dirY * playerWidth)] == 0)
            posY -= dirY * moveSpeed;
    }

    if (strafeRight) 
    {
        float _dirX, _dirY;
        float rotCos = cosf(-M_PI_2);
        float rotSin = sinf(-M_PI_2);
        _dirX = dirX * rotCos - dirY * rotSin;
        _dirY = dirX * rotSin + dirY * rotCos;
        if (d_worldMap[(int)(posX + _dirX * moveSpeed + _dirX * playerWidth)][(int)(posY)] == 0)
            posX += _dirX * moveSpeed;
        if (d_worldMap[(int)(posX)][(int)(posY + _dirY * moveSpeed + _dirY * playerWidth)] == 0)
            posY += _dirY * moveSpeed;
    }
    // Rotate to the right
    else if (g_input.right)
    {
        // Both camera direction and camera plane must be rotated
        // Rotate vectors by multiplying them with the rotation matrix:
        //   [ cos(a) -sin(a) ]
        //   [ sin(a)  cos(a) ]
        float oldDirX = dirX;
        float oldPlaneX = planeX;
        float rotCos = cosf(-rotationSpeed);
        float rotSin = sinf(-rotationSpeed);

        dirX = dirX * rotCos - dirY * rotSin;
        dirY = oldDirX * rotSin + dirY * rotCos;
        planeX = planeX * rotCos - planeY * rotSin;
        planeY = oldPlaneX * rotSin + planeY * rotCos;
    }

    if (strafeLeft)
    {
        float _dirX, _dirY;
        float rotCos = cosf(M_PI_2);
        float rotSin = sinf(M_PI_2);
        _dirX = dirX * rotCos - dirY * rotSin;
        _dirY = dirX * rotSin + dirY * rotCos;
        if (d_worldMap[(int)(posX + _dirX * moveSpeed + _dirX * playerWidth)][(int)(posY)] == 0)
            posX += _dirX * moveSpeed;
        if (d_worldMap[(int)(posX)][(int)(posY + _dirY * moveSpeed + _dirY * playerWidth)] == 0)
            posY += _dirY * moveSpeed;
    }
    // Rotate to the left
    else if (g_input.left)
    {
        // Both camera direction and camera plane must be rotated
        float oldDirX = dirX;
        float oldPlaneX = planeX;
        float rotCos = cosf(rotationSpeed);
        float rotSin = sinf(rotationSpeed);

        dirX = dirX * rotCos - dirY * rotSin;
        dirY = oldDirX * rotSin + dirY * rotCos;
        planeX = planeX * rotCos - planeY * rotSin;
        planeY = oldPlaneX * rotSin + planeY * rotCos;
    }

    // Temp variables for better code readability
    g_mainCamera.posX = posX;
    g_mainCamera.posY = posY;
    g_mainCamera.dirX = dirX;
    g_mainCamera.dirY = dirY;
    g_mainCamera.planeX = planeX;
    g_mainCamera.planeY = planeY;

    // Shooting
    if (g_input.shoot && g_time >= g_reloadAnimTimer + c_reloadms)
    {
        g_reloadAnimTimer = g_time + c_reloadAnim;
        if (g_target >= 0)
        {
            Enemy *enemy = &g_enemies[g_target];
            if (enemy->active != 0)
            {
                enemy->hp--;
                enemy->sprite->animTimer = g_time;
                if (enemy->hp <= 0)
                {
                    enemy->active = 0;
                    enemy->sprite->animIndex = 3;
                    g_kills++;
                    if (g_kills >= ENEMIES_MAX)
                        g_gameOverWin = 1;
                }
                else
                {
                    enemy->sprite->animIndex = 2;
                }
            }
        }
    }
}

void renderer_update(Camera *camera)
{
    clear_buffer();

    wall_raycaster(camera);

    if (showTargets)
    {
        sort_sprites(g_spriteOrder, g_spriteDistance, camera, SPRITES_MAX);
        sprite_raycaster(camera);
    }

    if (showGun)
    {
        if (g_time >= g_reloadAnimTimer)
            draw_bitmap(g_screenBuffer, gun__p, 96, 112, 128, 5);
        else
            draw_bitmap(g_screenBuffer, gun2__p, 96, 112, 128, 5);
    }

    transfer_buffer();
}

void init_camera()
{
    g_mainCamera.posX = 22.5f, g_mainCamera.posY = 12.5f; // camera x and y start position
    g_mainCamera.dirX = -1.0f, g_mainCamera.dirY = 0.0f;  // initial camera direction vector

    // Camera plane is perpendicular to the direction, but we
    // can change the length of it. The ratio between the length
    // of the direction and the camera plane determinates the FOV.
    // FOV is 2 * atan(0.66/1.0f)=66°, which is perfect for a first person shooter game
    g_mainCamera.planeX = 0.0f, g_mainCamera.planeY = 0.66f;
}

// https://lodev.org/cgtutor/raycasting.html
// Lode Vandevenne's raycasting engine algorithm
void wall_raycaster(Camera *camera)
{
    // For every vertical line on the screen
    for (int x = 0; x < SCREEN_W; x++)
    {
        //                  \     
        //       __________camX_____________________ camera plane
        //       -1           \   |                1
        //                  ray\  |dir
        //                      \ |
        //                       \|
        //                      player
        float cameraX = 2.0f * x / SCREEN_W - 1; // x-coordinate in camera space
        float rayDirX = camera->dirX + camera->planeX * cameraX;
        float rayDirY = camera->dirY + camera->planeY * cameraX;

        // Which box of the map we're in (just rounding to int), basically the index in map matrix
        int mapX = (int)(camera->posX);
        int mapY = (int)(camera->posY);

        // Length of ray from current position to next x or y-side
        float sideDistX;
        float sideDistY;

        // The distance the ray has to travel to go from 1 x-side
        // to the next x-side, or from 1 y-side to the next y-side.
        float deltaDistX = ABS(1.0f / rayDirX);
        float deltaDistY = ABS(1.0f / rayDirY);

        // Will be used later to calculate the length of the ray
        float perpendicularWallDistance;

        int stepX;
        int stepY;

        // Used to determinate whether or not the coming loop may be ended (was there a wall hit?)
        int hit = 0;
        // Will contain if an x-side or a y-side of a wall was hit
        int side;

        // Now, before the actual DDA can start, first stepX, stepY, and the
        // initial sideDistX and sideDistY still have to be calculated.
        if (rayDirX < 0)
        {
            stepX = -1;
            sideDistX = (camera->posX - mapX) * deltaDistX;
        }
        else
        {
            stepX = 1;
            sideDistX = (mapX + 1 - camera->posX) * deltaDistX;
        }
        if (rayDirY < 0)
        {
            stepY = -1;
            sideDistY = (camera->posY - mapY) * deltaDistY;
        }
        else
        {
            stepY = 1;
            sideDistY = (mapY + 1 - camera->posY) * deltaDistY;
        }

        // perform DDA
        while (hit == 0)
        {
            // Jump to next map square, either in x-direction, or in y-direction
            if (sideDistX < sideDistY)
            {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            // Check if ray has hit a wall
            if (d_worldMap[mapX][mapY] > 0)
                hit = 1;
        }

        // Calculate distance projected on camera direction. We don't use the
        // Euclidean distance to the point representing player, but instead
        // the distance to the camera plane (or, the distance of the point
        // projected on the camera direction to the player), to avoid the fisheye effect
        if (side == 0)
            perpendicularWallDistance = (sideDistX - deltaDistX);
        else
            perpendicularWallDistance = (sideDistY - deltaDistY);

        // Calculate the height of the line that has to be drawn on screen
        int line_height = SCREEN_H / perpendicularWallDistance;

        // Calculate lowest and highest pixel to fill in current stripe.
        // The center of the wall should be at the center of the screen, and
        // if these points lie outside the screen, they're capped to 0 or height-1
        int draw_start = -(line_height) / 2 + SCREEN_H / 2;
        if (draw_start < 0)
            draw_start = 0;
        int draw_end = (line_height) / 2 + SCREEN_H / 2;
        if (draw_end >= SCREEN_H)
            draw_end = SCREEN_H - 1;

        // Choose wall color from map
        // Number 1 on map corresponds with color 0 and so on
        uint8_t color = d_worldMap[mapX][mapY] - 1;

        // Draw the vertical stripe
        if (!wireframe)
        {
            for (int y = draw_start; y < draw_end; y++)
            {
                draw_pixel(g_screenBuffer, x, y, color);
            }
        }
        else 
        {
            draw_pixel(g_screenBuffer, x, draw_start, color);
            draw_pixel(g_screenBuffer, x, draw_end, color);
        }

        // Set the Z buffer (depth) for sprite casting
        g_zBuffer[x] = perpendicularWallDistance;
    }
}

// Sprite sorting functions
void swap(int *order1, float *dist1, int *order2, float *dist2)
{
    int temp_order = *order1;
    float temp_dist = *dist1;

    *order1 = *order2;
    *dist1 = *dist2;

    *order2 = temp_order;
    *dist2 = temp_dist;
}

int partition(int *order, float *dist, int begin, int end)
{
    float pivot_dist = dist[end];

    int i = begin - 1;

    for (int j = begin; j < end; j++)
    {
        if (dist[j] > pivot_dist)
        {
            i++;
            swap(&order[i], &dist[i], &order[j], &dist[j]);
        }
    }

    swap(&order[i + 1], &dist[i + 1], &order[end], &dist[end]);

    return i + 1;
}

void quick_sort(int *order, float *dist, int begin, int end)
{
    if (begin < end)
    {
        int index = partition(order, dist, begin, end);

        quick_sort(order, dist, begin, index - 1);
        quick_sort(order, dist, index + 1, end);
    }
}

// Sorting sprites by distance to player so that closer ones are rendered last (over the others)
// From farthest to nearest
void sort_sprites(int *order, float *dist, Camera *cam, int amount)
{
    // First calculate the distance from player for each sprite
    for (int i = 0; i < amount; i++)
    {
        order[i] = i;
        // Sqrt not taken, only relative distance is enough
        dist[i] = ((cam->posX - g_sprites[i].x) * (cam->posX - g_sprites[i].x) + (cam->posY - g_sprites[i].y) * (cam->posY - g_sprites[i].y));
    }
    // Then sort them
    quick_sort(order, dist, 0, amount - 1);
}

void sprite_raycaster(Camera *camera)
{
    // Reset target indicator
    g_target = -1;
    // Draw the sprites
    for (int i = 0; i < SPRITES_MAX; i++)
    {
        // Translate sprite position to relative to camera
        Sprite *sprite = &g_sprites[g_spriteOrder[i]];
        if (!sprite->visible)
            continue;

        float spriteX = sprite->x - camera->posX;
        float spriteY = sprite->y - camera->posY;

        // Transform the sprite with the inverse camera matrix
        // [ planeX   dirX ] -1                                       [ dirY      -dirX ]
        // [               ]       =  1/(planeX*dirY-dirX*planeY) *   [                 ]
        // [ planeY   dirY ]                                          [ -planeY  planeX ]
        float invDet = 1.0f / (camera->planeX * camera->dirY - camera->dirX * camera->planeY); // Required for matrix multiplication

        float transformX = invDet * (camera->dirY * spriteX - camera->dirX * spriteY);
        float transformY = invDet * (-camera->planeY * spriteX + camera->planeX * spriteY); // This is actually the depth inside the screen, what Z is in 3D

        // Don't render sprites too close to camera
        if (transformY < SPRITE_MIN_RENDER_DISTANCE)
            continue;

        // Sprite center in screen coordinates
        int spriteScreenX = (int)((SCREEN_W / 2) * (1.0f + transformX / transformY));

        // Calculate height of the sprite on screen
        int spriteHeight = ABS((int)(SCREEN_H / transformY)); // Using 'transformY' instead of the real distance prevents fisheye

        // Calculate lowest and highest pixel to fill in current stripe
        int drawStartY = -spriteHeight / 2 + SCREEN_H / 2;
        if (drawStartY < 0)
            drawStartY = 0;
        int drawEndY = spriteHeight / 2 + SCREEN_H / 2;
        if (drawEndY >= SCREEN_H)
            drawEndY = SCREEN_H - 1;

        // Sprite width is the same as height (square bitmaps)
        int spriteWidth = spriteHeight;
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        if (drawStartX < 0)
            drawStartX = 0;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        if (drawEndX >= SCREEN_W)
            drawEndX = SCREEN_W - 1;

        // Loop through every vertical stripe of the sprite on screen
        for (int stripe = drawStartX; stripe < drawEndX; stripe++)
        {
            int texX = (int)(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * tex_width / spriteWidth) / 256;
            // Draw the stripe if:
            // 1) it's in front of camera plane so you don't see things behind you
            // 2) it's on the screen (left)
            // 3) it's on the screen (right)
            // 4) it's closer than the wall
            if (transformY > 0 && stripe >= 0 && stripe < SCREEN_W && transformY < g_zBuffer[stripe])
            {
                // If sprite is on the center stripe, it's a target candidate
                if (stripe == SCREEN_W / 2)
                {
                    g_target = g_spriteOrder[i];
                }
                for (int y = drawStartY; y < drawEndY; y++)
                {
                    // For every pixel of the current stripe
                    int d = y * 256 - SCREEN_H * 128 + spriteHeight * 128; // 256 and 128 factors to avoid floats
                    int texY = d * tex_height / spriteHeight / 256;
                    uint32_t srcIndex = texY * tex_width + texX;
                    uint32_t color = d_spriteTextures[sprite->texture + sprite->animIndex][srcIndex]; // Get current color from the bitmap
                    if (color != 0)
                        draw_pixel(g_screenBuffer, stripe, y, color);
                }
            }
        }
    }
}

void draw_bitmap(uint8_t *buffer, uint8_t *bitmap, int x, int y, int size, uint8_t transparentColor)
{
    int index = 0;
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            uint8_t color = bitmap[index];
            if (color != transparentColor)
                draw_pixel(buffer, x + j, y + i, color);
            index++;
        }
    }
}

void draw_pixel(uint8_t *buffer, int x, int y, uint8_t color)
{
    asm("mov.w r0, %0\nmov.w r1, %1\nmov.w r2, %2\nmov.w r3, %3\nmov.w r4, %4\nmov.w r5, %5\npix\n"
        : /* No output */
        : "r"(x), "r"(y), "r"(color), "r"(buffer), "i"(160), "i"(240));
}

void clear_buffer()
{
    asm("push r1\npush r2\n push r3\nmov.w r1, %0\nmov.w r2, %1\nmov.w r3, %2\nblit\npop r3\npop r2\npop r1\n"
        : /* No output */
        : "i"(g_screenBuffer), "r"(g_emptyBuffer), "i"(SCREEN_W * SCREEN_H / 2));
}

void transfer_buffer()
{
    asm("push r1\npush r2\n push r3\nmov.w r1, 1024\nmov.w r2, %0\nmov.w r3, %1\nblit\npop r3\npop r2\npop r1\n"
        : /* No output */
        : "r"(g_screenBuffer), "i"(SCREEN_W * SCREEN_H / 2));
}

void input_update()
{
    int vkp, vkr;
    vkp = is_key_pressed();
    vkr = is_key_released();

    if(vkp == VK_LEFT_ALT)
    {
        strafe = true;
    } else if (vkp == VK_RIGHT_ALT)
    {
        strafe = true;
    }
    if(vkr == VK_LEFT_ALT)
    {
        strafe = false;
        strafeLeft = strafeRight = false;
    } else if (vkr == VK_RIGHT_ALT)
    {
        strafe = false;
        strafeLeft = strafeRight = false;
    }

    if ((vkp == 0) && (vkr == 0))
    {
        vkp = g_oldKey;
    }
    if ((vkr != 0) /*&& (vkr == g_oldKey)*/)
    {
        vkp = 0;
        g_oldKey = 0;
    }
    if (vkp != 0 && vkp != VK_SPACE && vkp != VK_R && vkp != VK_ESC)
    {
        g_oldKey = vkp;
    }

    g_input.left = 0;
    g_input.right = 0;
    g_input.up = 0;
    g_input.down = 0;
    g_input.shoot = 0;
    g_input.restart = 0;
    g_input.quit = 0;
    strafeLeft = strafeRight = false;

    switch (vkp)
    {
    case VK_LEFT_ARROW:
        g_input.left = 1;
        if (strafe) strafeLeft = true; else strafeLeft = false;
        break;
    case VK_RIGHT_ARROW:
        g_input.right = 1;
        if (strafe) strafeRight = true; else strafeRight = false;
        break;
    case VK_UP_ARROW:
        g_input.up = 1;
        break;
    case VK_DOWN_ARROW:
        g_input.down = 1;
        break;
    case VK_SPACE:
        g_input.shoot = 1;
        break;
    case VK_R:
        g_input.restart = 1;
        break;
    case VK_ESC:
        g_input.quit = 1;
        break;
    case VK_W:
        wireframe ^= 1;
        break;
    case VK_T:
        showTargets ^= 1;
        break;
    case VK_G:
        showGun ^= 1;
        break;
    default:
        break;
    }
}
#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include <vector>

const int screenWidth = 1280;
const int screenHeight = 720;

const int playerSize = 50;
const float friction = .9f;
const float playerSpeed = 100;

const int gravity = 1300;
const int jumpForce = 600;

using namespace std;

class platform
{
    public:
        Vector2 position;
        Vector2 size;

        platform(float xPos, float yPos, float width, float height)
        {
            position = {xPos, yPos};
            size = {width, height};
        }

        void draw()
        {
            DrawRectangleV(position, size, DARKGRAY);
        }

        Rectangle getRect()
        {
            return Rectangle{position.x, position.y, size.x, size.y};
        }
        
};

class Player
{
    public:
        Vector2 position;
        int direction = 0;
        float xVelocity = 0;
        float yVelocity = 0;

        bool canJump = true;

        void draw()
        {
            Rectangle tempRec = Rectangle{position.x - playerSize/2, position.y - playerSize/2, playerSize, playerSize};

            DrawRectangleRounded(tempRec, .3, 6, BLACK);
        }

        void update(vector<platform>& platforms)
        {
            float deltaTime = GetFrameTime();

            xVelocity = (xVelocity + direction) * friction;
            position.x += xVelocity * deltaTime * playerSpeed;

            yVelocity += gravity * deltaTime;
            position.y += yVelocity * deltaTime;

            canJump = false;
            for (auto& plat : platforms)
                {
                    Rectangle platRect = plat.getRect();
                    Rectangle playerRect = {position.x - playerSize/2, position.y - playerSize/2, playerSize, playerSize};

                    if (CheckCollisionRecs(playerRect, platRect))
                    {
                        // Top Collision
                        if (playerRect.y + playerRect.height - yVelocity * deltaTime <= platRect.y)
                        {
                            position.y = platRect.y - playerSize / 2;
                            yVelocity = 0;
                            canJump = true;
                        }
                    }
                }

                // Stay inside window horizontally
                if (position.x < playerSize / 2) position.x = playerSize / 2;
                if (position.x > screenWidth - playerSize / 2) position.x = screenWidth - playerSize / 2;
                // Stay inside window vertically (floor)
                if (position.y > screenHeight - playerSize / 2)
                {
                    position.y = screenHeight - playerSize / 2;
                    yVelocity = 0;
                    canJump = true;
                }
            }

        void jump()
        {
            if (canJump)
            {
                yVelocity = -jumpForce;
                canJump = false;
            }
        }
};

class Game
{
    public:
        Player player = Player();
        vector<platform> platforms;

        Camera2D camera = {0};

        void gameStart()
        {
            player.position = {screenWidth / 2, screenHeight /2};

            platforms.push_back(platform(300, 500, 400, 20));
            platforms.push_back(platform(800, 400, 200, 20));
            platforms.push_back(platform(100, 300, 250, 20));
        }

        void update()
        {
            player.update(platforms);

            float lerpFactor = 0.1f; // lower = slower, higher = snappier
            camera.target.x = Lerp(camera.target.x, player.position.x, lerpFactor);
            camera.target.y = Lerp(camera.target.y, player.position.y, lerpFactor);

            camera.offset = {screenWidth/2.0f, screenHeight/2.0f};
            camera.rotation = 0;
            camera.zoom = 1.0f;
        }

        void draw()
        {
            player.draw();
            for (auto& plat : platforms)
            {
                plat.draw();
            }
        }
};

int main () {

    InitWindow(screenWidth, screenHeight, "Hookle");

    SetTargetFPS(60);

    Game game = Game();
    game.gameStart();

    while(WindowShouldClose() == false)
    {

        game.update();

        //Logic
        if (IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_W))
        {
            game.player.jump();
        }
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        {
            game.player.direction = -1;
        } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        {
            game.player.direction = 1;
        } else
        {
            game.player.direction = 0;
        }

        //Drawing
        BeginDrawing();

        ClearBackground(WHITE);

        BeginMode2D(game.camera);
        game.draw();
        EndMode2D();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
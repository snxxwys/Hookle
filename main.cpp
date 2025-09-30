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

Color lightBlue = {181, 215, 251, 255};
Color darkBlue = {139, 169, 225, 255};
Color lightPurple = {41, 42, 88, 255};
Color darkPurple = {29, 30, 97, 255};

using namespace std;

class platform
{
    public:
        Vector2 position;
        Vector2 size;
        int thickness = 10;

        platform(float xPos, float yPos, float width, float height)
        {
            position = {xPos, yPos};
            size = {width, height};
        }

        void drawLines()
        {
            Rectangle tempRec = {position.x + thickness, position.y + thickness, size.x - thickness * 2, size.y - thickness * 2};
            DrawRectangleRoundedLinesEx(tempRec, .2, 6, thickness, darkBlue);
        }

        void draw()
        {
            Rectangle tempRec = {position.x + thickness, position.y + thickness, size.x - thickness * 2, size.y - thickness * 2};
            DrawRectangleRounded(tempRec, .2, 6, lightBlue);
            //DrawRectangleV(position, size, DARKGRAY);
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
                        // Calculate how much the player overlaps in each direction
                        float overlapLeft   = (playerRect.x + playerRect.width) - platRect.x;
                        float overlapRight  = (platRect.x + platRect.width) - playerRect.x;
                        float overlapTop    = (playerRect.y + playerRect.height) - platRect.y;
                        float overlapBottom = (platRect.y + platRect.height) - playerRect.y;

                        // Find the smallest overlap
                        float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : -overlapRight;
                        float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : -overlapBottom;

                        // Resolve collision along the axis with smaller penetration
                        if (abs(minOverlapX) < abs(minOverlapY))
                        {
                            // Horizontal collision
                            position.x -= minOverlapX;
                            xVelocity = 0;
                        }
                        else
                        {
                            // Vertical collision 
                            position.y -= minOverlapY;
                            yVelocity = 0;

                            // If hitting the top of the platform, allow jumping
                            if (minOverlapY > 0)
                            {
                                canJump = true;
                            }
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

            platforms.push_back(platform(300, 500, 400, 400));
            platforms.push_back(platform(800, 400, 200, 400));
            platforms.push_back(platform(100, 300, 250, 400));
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
                plat.drawLines();
            }
            for (auto& plat : platforms)
            {
                plat.draw();
            }
        }
};

int main () {

    InitWindow(screenWidth, screenHeight, "Hookle");

    Image gradient = GenImageGradientLinear(screenWidth, screenHeight, 45, lightPurple, darkPurple);
    Texture gradientTexture = LoadTextureFromImage(gradient);

    UnloadImage(gradient);

    SetTargetFPS(60);

    Game game = Game();
    game.gameStart();

    while(WindowShouldClose() == false)
    {

        game.update();

        //Logic
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
        {
            game.player.jump();
        }
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        {
            game.player.direction = -1;
        }
        else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        {
            game.player.direction = 1;
        }
        else
        {
            game.player.direction = 0;
        }

        //Drawing
        BeginDrawing();

        ClearBackground(WHITE);
        DrawTexture(gradientTexture, 0, 0, WHITE);

        BeginMode2D(game.camera);
        game.draw();
        EndMode2D();

        EndDrawing();
    }

    UnloadTexture(gradientTexture);
    CloseWindow();
    return 0;
}
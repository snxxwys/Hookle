#include <iostream>
#include <vector>
#include <fstream>
#include <regex>
#include <sstream>
#include "raylib.h"
#include "raymath.h"

const int screenWidth = 1280;
const int screenHeight = 720;
const int playerSize = 50;
const float friction = .9f;
const float playerSpeed = 100;
const int gravity = 1300;
const int jumpForce = 600;

Color lightBlue = {181, 215, 251, 255};
Color darkBlue = {139, 169, 225, 255};
Color lightPurple = {141, 142, 188, 255};
Color darkPurple = {29, 30, 97, 255};
Color white = {215, 215, 215, 255};
Color whiter = {225, 225, 225, 255};
Color black = {25, 25, 25, 255};

Color hover = {63, 63, 63, 255};
Color selected = {209, 94, 71, 255};

using namespace std;

class platform
{
    public:
        Vector2 position;
        Vector2 size;
        int thickness = 10;

        platform() { position = {0,0}; size = {100,20}; }

        platform(float xPos, float yPos, float width, float height)
        {
            position = {xPos, yPos};
            size = {width, height};
        }

        void draw()
        {
            DrawRectangleV(position, size, black);
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
        float thickness = 10;
        bool canJump = true;

        bool swinging = false;
        Vector2 anchor;
        float ropeLength;
        float ropeAngle;
        float angularVelocity;

        bool wasSwingingLastFrame = false;

        void draw()
        {   
            if (swinging) {
                DrawLineV(anchor, position, black);
                DrawCircleV(anchor, 4, black);
            }

            Rectangle tempRec = Rectangle{position.x - playerSize/2, position.y - playerSize/2, playerSize, playerSize};
            DrawRectangle(position.x - playerSize/2, position.y - playerSize/2, playerSize, playerSize, whiter);
            DrawRectangleLinesEx(tempRec, 10, black);
        }

        void update(vector<platform>& platforms)
        {
            float deltaTime = GetFrameTime();

            if (swinging) {
                float g = 1300.0f;
                float angularAccel = -(g / ropeLength) * sinf(ropeAngle - PI/2);

                if (IsKeyDown(KEY_D)) angularAccel -= 2.0f;
                if (IsKeyDown(KEY_A)) angularAccel += 2.0f;

                angularVelocity += angularAccel * deltaTime;
                angularVelocity *= 0.995f;
                ropeAngle += angularVelocity * deltaTime;

                position.x = anchor.x + ropeLength * cosf(ropeAngle);
                position.y = anchor.y + ropeLength * sinf(ropeAngle);
            } else {
                xVelocity = (xVelocity + direction) * friction;
                position.x += xVelocity * deltaTime * playerSpeed;
                yVelocity += gravity * deltaTime;
                position.y += yVelocity * deltaTime;
            }


            canJump = false;

            for (auto& plat : platforms)
            {
                Rectangle platRect = plat.getRect();
                Rectangle playerRect = {position.x - playerSize/2, position.y - playerSize/2, playerSize, playerSize};
                if (CheckCollisionRecs(playerRect, platRect))
                {
                    float overlapLeft   = (playerRect.x + playerRect.width) - platRect.x;
                    float overlapRight  = (platRect.x + platRect.width) - playerRect.x;
                    float overlapTop    = (playerRect.y + playerRect.height) - platRect.y;
                    float overlapBottom = (platRect.y + platRect.height) - playerRect.y;
                    float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : -overlapRight;
                    float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : -overlapBottom;

                    if (!swinging)
                    {
                        if (abs(minOverlapX) < abs(minOverlapY))
                        {
                            position.x -= minOverlapX;
                            xVelocity = 0;
                        }
                        else
                        {
                            position.y -= minOverlapY;
                            yVelocity = 0;
                            if (minOverlapY > 0)
                            {
                                canJump = true;
                            }
                        }
                    }
                    else
                    {
                        angularVelocity = -angularVelocity * .95f;
                    }
                }
            }
            
            wasSwingingLastFrame = swinging;
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

enum EditAction { NONE, MOVE, RESIZE };
struct ResizeMask
{
    bool left=false;
    bool right=false;
    bool top=false;
    bool bottom=false;
    bool any() const { return left||right||top||bottom; }
};

class Game
{
    public:
        Player player = Player();
        vector<platform> platforms;
        Camera2D camera = {0};
        bool editMode = false;
        int selectedIndex = -1;
        EditAction currentAction = NONE;
        ResizeMask resizeMask;
        Vector2 dragOffset = {0,0};
        Vector2 originalPos = {0,0};
        Vector2 originalSize = {0,0};
        float edgeGrab = 8.0f;
        float minSize = 8.0f;

        void gameStart()
        {
            player.position = {screenWidth / 2, screenHeight /2};
            platforms.push_back(platform(300, 500, 400, 40));
            platforms.push_back(platform(800, 400, 200, 40));
            platforms.push_back(platform(100, 300, 250, 40));
            camera.offset = {screenWidth/2.0f, screenHeight/2.0f};
            camera.rotation = 0;
            camera.zoom = 1.0f;
            camera.target = player.position;
        }

        int pickPlatformAtPoint(Vector2 worldPoint)
        {
            for (int i = (int)platforms.size()-1; i >= 0; --i)
            {
                Rectangle r = platforms[i].getRect();
                if (CheckCollisionPointRec(worldPoint, r)) return i;
            }
            return -1;
        }

        ResizeMask calcResizeMask(platform &p, Vector2 worldPoint)
        {
            ResizeMask m;
            Rectangle r = p.getRect();
            if (fabs(worldPoint.x - r.x) <= edgeGrab) m.left = true;
            if (fabs(worldPoint.x - (r.x + r.width)) <= edgeGrab) m.right = true;
            if (fabs(worldPoint.y - r.y) <= edgeGrab) m.top = true;
            if (fabs(worldPoint.y - (r.y + r.height)) <= edgeGrab) m.bottom = true;
            return m;
        }

        void startDrag(int idx, Vector2 worldPoint)
        {
            selectedIndex = idx;
            if (selectedIndex < 0) return;
            platform &p = platforms[selectedIndex];
            ResizeMask m = calcResizeMask(p, worldPoint);
            if (m.any())
            {
                currentAction = RESIZE;
                resizeMask = m;
                originalPos = p.position;
                originalSize = p.size;
            }
            else
            {
                currentAction = MOVE;
                dragOffset = Vector2{ worldPoint.x - p.position.x, worldPoint.y - p.position.y };
            }
        }

        void applyDrag(Vector2 worldPoint)
        {
            if (selectedIndex < 0) return;
            platform &p = platforms[selectedIndex];
            if (currentAction == MOVE)
            {
                p.position = { worldPoint.x - dragOffset.x, worldPoint.y - dragOffset.y };
            }
            else if (currentAction == RESIZE)
            {
                Vector2 np = p.position;
                Vector2 ns = p.size;
                if (resizeMask.left)
                {
                    float newX = worldPoint.x;
                    float right = originalPos.x + originalSize.x;
                    float newW = right - newX;
                    if (newW < minSize) newW = minSize, newX = right - newW;
                    np.x = newX;
                    ns.x = newW;
                }
                if (resizeMask.right)
                {
                    float newW = worldPoint.x - originalPos.x;
                    if (newW < minSize) newW = minSize;
                    ns.x = newW;
                    np.x = originalPos.x;
                }
                if (resizeMask.top)
                {
                    float newY = worldPoint.y;
                    float bottom = originalPos.y + originalSize.y;
                    float newH = bottom - newY;
                    if (newH < minSize) newH = minSize, newY = bottom - newH;
                    np.y = newY;
                    ns.y = newH;
                }
                if (resizeMask.bottom)
                {
                    float newH = worldPoint.y - originalPos.y;
                    if (newH < minSize) newH = minSize;
                    ns.y = newH;
                    np.y = originalPos.y;
                }
                p.position = np;
                p.size = ns;
            }
        }

        void endDrag()
        {
            currentAction = NONE;
            resizeMask = ResizeMask();
        }

        void update()
        {
            float lerpFactor = 0.1f;
            if (!editMode)
            {
                player.update(platforms);
                camera.target.x = Lerp(camera.target.x, player.position.x, lerpFactor);
                camera.target.y = Lerp(camera.target.y, player.position.y, lerpFactor);
            }
            else
            {
                Vector2 mouseScreen = GetMousePosition();
                Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    int idx = pickPlatformAtPoint(mouseWorld);
                    startDrag(idx, mouseWorld);
                }
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && currentAction != NONE)
                {
                    applyDrag(mouseWorld);
                }
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                {
                    endDrag();
                }
                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
                {
                    Vector2 size = {150, 30};
                    Vector2 pos = { mouseWorld.x - size.x/2.0f, mouseWorld.y - size.y/2.0f };
                    platforms.emplace_back(pos.x, pos.y, size.x, size.y);
                    selectedIndex = (int)platforms.size() - 1;
                }
            }
            camera.offset = {screenWidth/2.0f, screenHeight/2.0f};
            camera.rotation = 0;
            if (camera.zoom <= 0.01f) camera.zoom = 1.0f;
            camera.zoom = camera.zoom;
        }

        void drawEditorUI()
        {
            Vector2 mouseScreen = GetMousePosition();
            Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);
            int hoverIndex = pickPlatformAtPoint(mouseWorld);
            for (int i = 0; i < (int)platforms.size(); ++i)
            {
                platform &p = platforms[i];
                Rectangle r = p.getRect();
                if (i == selectedIndex)
                {
                    DrawRectangleV(p.position, p.size, lightPurple);

                    DrawRectangleLinesEx(r, 3, darkBlue);

                    DrawRectangle(rint(p.position.x) - 5, rint(p.position.y) - 5, 10, 10, darkBlue);
                    DrawRectangle(rint(p.position.x + p.size.x) - 5, rint(p.position.y) - 5, 10, 10, darkBlue);
                    DrawRectangle(rint(p.position.x + p.size.x) - 5, rint(p.position.y + p.size.y) - 5, 10, 10, darkBlue);
                    DrawRectangle(rint(p.position.x) - 5, rint(p.position.y + p.size.y) - 5, 10, 10, darkBlue);
                }
                else if (i == hoverIndex)
                {
                    DrawRectangleLinesEx(r, 2, hover);
                    DrawRectangleV(p.position, p.size, hover);
                }
                else
                {
                    p.draw();
                }
            }
            if (hoverIndex >= 0)
            {
                platform &h = platforms[hoverIndex];
                ResizeMask m = calcResizeMask(h, mouseWorld);
                Rectangle r = h.getRect();
                if (m.left) DrawRectangleLinesEx(Rectangle{r.x-2, r.y, 4, r.height}, 0, (Color){255,0,0,255});
                if (m.right) DrawRectangleLinesEx(Rectangle{r.x+r.width-2, r.y, 4, r.height}, 0, (Color){255,0,0,255});
                if (m.top) DrawRectangleLinesEx(Rectangle{r.x, r.y-2, r.width, 4}, 0, (Color){255,0,0,255});
                if (m.bottom) DrawRectangleLinesEx(Rectangle{r.x, r.y+r.height-2, r.width, 4}, 0, (Color){255,0,0,255});
            }
        }

        void draw()
        {
            if (!editMode)
            {
                player.draw();
                for (auto& plat : platforms) plat.draw();
            }
            else
            {
                /*for (int i = 0; i < (int)platforms.size(); ++i)
                {
                    platform &p = platforms[i];
                    Rectangle r = p.getRect();
                    if (i == selectedIndex)
                    {
                        //DrawRectangleV(p.position, p.size, (Color){230,230,255,255});
                        //DrawRectangleLinesEx(r, 3, (Color){255,140,0,255});
                    }
                    else
                    {
                        //DrawRectangleV(p.position, p.size, (Color){200,200,200,255});
                        //DrawRectangleLinesEx(r, 1, black);
                    }
                }*/
                drawEditorUI();
            }
        }

        void reset(Sound resetSound)
        {
            PlaySound(resetSound);
            player.position = {screenWidth / 2, screenHeight /2};
            player.xVelocity = 0;
            player.yVelocity = 0;
            player.swinging = false;
        }

        bool saveToJson(const string &path)
        {
            ofstream out(path);
            if (!out.is_open()) return false;
            out << "[\n";
            for (size_t i = 0; i < platforms.size(); ++i)
            {
                platform &p = platforms[i];
                out << "  {\"x\":" << p.position.x << ",\"y\":" << p.position.y << ",\"w\":" << p.size.x << ",\"h\":" << p.size.y << "}";
                if (i + 1 < platforms.size()) out << ",\n"; else out << "\n";
            }
            out << "]\n";
            out.close();
            return true;
        }

        bool loadFromJson(const string &path)
        {
            ifstream in(path);
            if (!in.is_open()) return false;
            string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
            in.close();
            regex numberRegex("(-?\\d+\\.?\\d*)");
            sregex_iterator it(content.begin(), content.end(), numberRegex);
            sregex_iterator end;
            vector<float> nums;
            for (; it != end; ++it)
            {
                nums.push_back(stof((*it)[1].str()));
            }
            vector<platform> newPlats;
            for (size_t i = 0; i + 3 < nums.size(); i += 4)
            {
                newPlats.emplace_back(nums[i+0], nums[i+1], nums[i+2], nums[i+3]);
            }
            platforms = std::move(newPlats);
            selectedIndex = -1;
            return true;
        }
};

int main () {

    InitWindow(screenWidth, screenHeight, "Hookle Editor");
    SetTargetFPS(60);

    Game game = Game();
    game.gameStart();

    InitAudioDevice();
    Sound resetSound = LoadSound("sounds/reset.mp3");

    game.loadFromJson("main.json");

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_E))
        {
            game.editMode = !game.editMode;
            game.currentAction = NONE;
            game.selectedIndex = -1;
        }

        if (IsKeyPressed(KEY_O) && game.editMode)
        {
            game.saveToJson("main.json");
        }
        if (IsKeyPressed(KEY_L) && game.editMode)
        {
            game.loadFromJson("main.json");
        }
        if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))
        {
            if (game.editMode && game.selectedIndex >= 0 && game.selectedIndex < (int)game.platforms.size())
            {
                game.platforms.erase(game.platforms.begin() + game.selectedIndex);
                game.selectedIndex = -1;
            }
        }

        if (!game.editMode)
        {
            if (game.player.position.y > 1000)
            {
                game.reset(resetSound);
            }

            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) game.player.jump();
            if (IsKeyDown(KEY_R)) game.reset(resetSound);
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) game.player.direction = -1;
            else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) game.player.direction = 1;
            else game.player.direction = 0;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), game.camera);
                game.player.anchor = mouseWorld;

                Vector2 diff = Vector2Subtract(game.player.position, game.player.anchor);
                game.player.ropeLength = Vector2Length(diff);
                game.player.ropeAngle = atan2f(diff.y, diff.x);

                if (game.player.ropeLength != 0)
                {
                    Vector2 tangent = { -diff.y / game.player.ropeLength, diff.x / game.player.ropeLength };
                    game.player.angularVelocity = (game.player.xVelocity * tangent.x + game.player.yVelocity * tangent.y) / game.player.ropeLength;
                }

                game.player.swinging = true;
            }

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            {
                if (game.player.swinging) 
                {
                    Vector2 toPlayer = Vector2Subtract(game.player.position, game.player.anchor);
                    float len = Vector2Length(toPlayer);

                    if (len != 0)
                    {
                        Vector2 tangent = { -toPlayer.y / len, toPlayer.x / len };

                        float speed = game.player.angularVelocity * game.player.ropeLength;
                        game.player.xVelocity = tangent.x * speed / 35;
                        game.player.yVelocity = tangent.y * speed;
                    }
                }

                game.player.swinging = false;
            }
        }
        else
        {
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
            {
                game.camera.target.x -= 400 * GetFrameTime();
            }
            else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
            {
                game.camera.target.x += 400 * GetFrameTime();
            }
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
            {
                game.camera.target.y -= 400 * GetFrameTime();
            }
            else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
            {
                game.camera.target.y += 400 * GetFrameTime();
            }
        }

        game.update();

        BeginDrawing();

        ClearBackground(white);

        BeginMode2D(game.camera);

        game.draw();

        EndMode2D();

        DrawText("EDITOR MODE", 10, 10, 18, black);
        DrawText("E - Toggle | Right-click - New Box | Delete - Remove | O - Save | L - Load", 10, 30, 18, black);
        

        EndDrawing();
    }

    UnloadSound(resetSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
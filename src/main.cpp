#include <iostream>
#include <sstream>
#include <raylib.h>
#include <raymath.h>
#include "virtual.h"

#define ROCKET_SPEED 6.0f
#define ROCKET_ROTATION_SPEED 3.0f
#define ROCKET_ACCELRATION 0.02f

#define METEOR_SPEED 0.5f
#define METEOR_ANGULAR_SPEED 0.5f
#define METEOR_DEACCEL 0.0003f
#define MAX_METEORS 15
#define METEOR_COLLISION_DAMPING 0.0001f
#define METEOR_ROCKET_COLLISION_DAMPING 0.1f;

#define COMMAND_PORT 10
#define STATUS_PORT  11

#define DEFAULT_FONT_SIZE 20

typedef struct Rocket {
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float acceleration;
} Rocket;

typedef struct Meteor {
    Vector2 position;
    Vector2 velocity;
    float   radius;   // Scale factor of texture (1.0 default)
} Meteor;

enum Status {
    NOT_READY = 0,
    READY = 69,             // Initial ready set by the emulator   
    COMMAND_NOT_READY = 99, // Not ready to execute the next command (check from asm before setting command)
    COMMAND_READY = 100,    // Ready to execute the next command
};

// Commands to control rocket from virtual port
enum Command {
    NOP,
    ACCEL_POS,    // Acceleration increases by ROCKET_ACCELERATION
    ACCEL_NEG,
    ROTATE_RIGHT, // Rotation factor is ROCKET_ROTATION_SPEED
    ROTATE_LEFT,
    EXIT          // Sent when emulator wants to shutdown the device
};


char *command_str_map[6] = {
    "Command::NOP",
    "Command::ACCEL_POS",
    "Command::ACCEL_NEG",
    "Command::ROTATE_RIGHT",
    "Command::ROTATE_LEFT",
    "Command::EXIT",
};

bool portAccessAvailable = false;

Meteor meteors[MAX_METEORS] = {};

Meteor *currentSelectedMeteorDebug = nullptr;

int main() {
    const int width = 1020;
    const int height = 600;
    // Wait until response from port
    if (init_virtual_device() == 0) {
        portAccessAvailable = true;
        std::cout << "Waiting for STATUS::READY from port!" << std::endl;
        unsigned char statusByte;
        while ((statusByte = read_port_byte(STATUS_PORT)) != Status::READY)
            ; 
        std::cout << "Port Value: " << (int) statusByte << std::endl;
        std::cout << "Currnet Path: " << GetWorkingDirectory() << std::endl;
    } else {
        portAccessAvailable = false;
    }

    InitWindow(width, height, "Rocket Simulation - 8086");
    SetTargetFPS(60);


    // Load rocket texture
    Texture2D sprite = LoadTexture("resources/Main Ship - Base - Full health.png");
    float frameWidth = sprite.width;
    float frameHeight = sprite.height;
    float frameScaleFactor = 1.5;
    Rocket rocket = Rocket{0.0};
    rocket.position = Vector2{width/2, height/2};

    // Load font
    Font ttfFont = LoadFont("resources/font.ttf");

    Image noiseImage = GenImagePerlinNoise(width, height, 50, 50, 2.5f);
    Texture2D noiseTexture = LoadTextureFromImage(noiseImage);

    // Intialize shader (distortion cloud)
    Shader shader = LoadShader(0, TextFormat("resources/space_noise.fs", 330));
    int useconds = GetShaderLocation(shader, "seconds");
    int ufreqX =   GetShaderLocation(shader, "freqX");
    int ufreqY =   GetShaderLocation(shader, "freqY");
    int uampX =    GetShaderLocation(shader, "ampX");
    int uampY =    GetShaderLocation(shader, "ampY");
    int uspeedX =  GetShaderLocation(shader, "speedX");
    int uspeedY =  GetShaderLocation(shader, "speedY");

    // Shader uniform values that can be updated at any time
    const float freqX = 10.0f;
    const float freqY = 10.0f;
    const float ampX = 2.0f;
    const float ampY = 2.0f;
    const float speedX = 3.0f;
    const float speedY = 3.0f;

    float screenSize[2] = { (float)GetScreenWidth(), (float)GetScreenHeight() };
    SetShaderValue(shader, GetShaderLocation(shader, "size"), &screenSize, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader, ufreqX, &freqX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, ufreqY, &freqY, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, uampX, &ampX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, uampY, &ampY, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, uspeedX, &speedX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, uspeedY, &speedY, SHADER_UNIFORM_FLOAT);


    // Intialize shader (stars)
    Image colorImage = GenImageColor(width, height, BLACK);
    Texture2D colorTexture = LoadTextureFromImage(colorImage);
    Shader starsShader = LoadShader(0, TextFormat("resources/stars.fs", 330));
    int utime = GetShaderLocation(starsShader, "time");
    int ubgcolor = GetShaderLocation(starsShader, "bg_color");
    Vector4 bg_color = {0.0, 0.0, 0.0, 1.0};
    SetShaderValue(starsShader, ubgcolor, &bg_color, SHADER_UNIFORM_VEC4);

    float seconds = 0.0f;

    // Source rectangle (part of the texture to use for drawing)
    Rectangle sourceRec = { 0.0f, 0.0f, (float)frameWidth, (float)frameHeight };
    // Destination rectangle (screen rectangle where drawing part of texture)
    Rectangle destRec = { rocket.position.x, rocket.position.y, frameWidth * frameScaleFactor, frameHeight * frameScaleFactor };
    // Origin of the texture (rotation/scale point), it's relative to destination rectangle size
    Vector2 origin = { (float)(frameWidth * frameScaleFactor) / 2, (float)(frameHeight * frameScaleFactor) / 2 };

    // Initialize meteors
    Texture2D meteorSprite = LoadTexture("resources/Asteroid 01 - Base.png");

    for (int i = 0; i < MAX_METEORS; i++) {
        Vector2 spawnPosition = Vector2{
            (float)GetRandomValue(0, width),
            (float)GetRandomValue(-1, 1) * height
        };
        Vector2 velocity = Vector2Subtract(rocket.position, spawnPosition);
        velocity.x += GetRandomValue(-5, 5);
        velocity.y += GetRandomValue(-5, 5);
        velocity = Vector2Normalize(velocity);

        meteors[i] = Meteor{
            spawnPosition,
            velocity, 
            (float)GetRandomValue(0.5, 2.0)
        };
    }

    int command = 0;
    // Disregard NOP commands, Used only for displaying the command.
    int lastCommandExecuted = 0;
    while (!WindowShouldClose() && command != Command::EXIT)
    {
        // Process commands from port
        if (portAccessAvailable) {
            write_port_byte(STATUS_PORT, Status::COMMAND_READY);
            command = read_port_byte(COMMAND_PORT);
            lastCommandExecuted = command > 0 ? command : lastCommandExecuted;
            switch (command) {
            case Command::ACCEL_POS:
                rocket.acceleration += ROCKET_ACCELRATION;
                break;
            case Command::ACCEL_NEG:
                rocket.acceleration -= ROCKET_ACCELRATION;
                break;
            case Command::ROTATE_RIGHT:
                rocket.rotation += ROCKET_ROTATION_SPEED;
                break;
            case Command::ROTATE_LEFT:
                rocket.rotation -= ROCKET_ROTATION_SPEED;
                break;
            default:
                break;
            }
            // Reset the command port so the command does not get repeated over many cycles
            write_port_byte(COMMAND_PORT, Command::NOP);
            write_port_byte(STATUS_PORT, Status::COMMAND_NOT_READY);
        }


        // Select asteroid based on mouse click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            int idx = -1;
            float lowestDistance = 999999;
            for (int i = 0 ; i < MAX_METEORS; i++) {
                float distance = Vector2Distance(meteors[i].position, mousePos);
                if (distance < 20 && distance < lowestDistance) {
                    lowestDistance = distance;
                    idx = i;
                }
            }
            if (idx != -1)
                currentSelectedMeteorDebug = &meteors[idx];
            else
                currentSelectedMeteorDebug = nullptr;
        }


        // Manual key control
        if (IsKeyDown(KEY_LEFT))  rocket.rotation -= ROCKET_ROTATION_SPEED;
        if (IsKeyDown(KEY_RIGHT)) rocket.rotation += ROCKET_ROTATION_SPEED;
        if (IsKeyDown(KEY_UP))   rocket.acceleration += ROCKET_ACCELRATION;
        if (IsKeyDown(KEY_DOWN)) rocket.acceleration -= ROCKET_ACCELRATION;

        // Calculate velocity of rocket relative to the rotation
        rocket.velocity.x = sin(rocket.rotation * DEG2RAD) * ROCKET_SPEED;
        rocket.velocity.y = cos(rocket.rotation * DEG2RAD) * ROCKET_SPEED;

        // Update position with velocity and acceleration
        rocket.position.x += (rocket.velocity.x * rocket.acceleration);
        rocket.position.y -= (rocket.velocity.y * rocket.acceleration);
        destRec.x = rocket.position.x;
        destRec.y = rocket.position.y;

        seconds = GetTime();
        SetShaderValue(shader, useconds, &seconds, SHADER_UNIFORM_FLOAT);
        SetShaderValue(starsShader, utime, &seconds, SHADER_UNIFORM_FLOAT);

        // Keep the rocket within screen bounds
        int rocketHeight = destRec.height;
        if (destRec.x > width + rocketHeight) 
            rocket.position.x = -rocketHeight;
        else if (destRec.x < -rocketHeight)
            rocket.position.x = width + rocketHeight;
        if (destRec.y > height + rocketHeight)
            rocket.position.y = -rocketHeight;
        else if (destRec.y < -rocketHeight)
            rocket.position.y = height + rocketHeight;
        
        Rectangle shipRec;
        // Check meteor collisions
        for (int i = 0; i < MAX_METEORS; i++) {
            // With rocket
            Meteor *meteor = &meteors[i];
            bool isColliding = CheckCollisionCircles(meteor->position, meteor->radius * 13.0, rocket.position, destRec.width / 2 - 10.0f);
            if (isColliding) {
                Vector2 norm = Vector2Normalize(rocket.velocity);
                if (abs(rocket.acceleration) < 0.20) {
                    meteor->velocity.x =  -0.5 * meteor->velocity.x;
                    meteor->velocity.y =  -0.5 * meteor->velocity.y;
                } else {
                    meteor->velocity.x +=  norm.x * rocket.acceleration * METEOR_ROCKET_COLLISION_DAMPING;
                    meteor->velocity.y +=  -1 * norm.y * rocket.acceleration * METEOR_ROCKET_COLLISION_DAMPING;
                }
            }

            // With other meteors
            for (int j = 0; j < MAX_METEORS; j++) {
                Meteor *meteor1 = &meteors[j];
                // No need to check collision for itself
                if (i == j)
                    continue;
                bool isColliding = CheckCollisionCircles(meteor->position, meteor->radius * 13.0, meteor1->position, meteor1->radius * 13.0);
                if (isColliding) {
                    // Change direction of meteor after collision relative to size
                    Vector2 tempMeteor1Vel = meteor1->velocity;
                    meteor1->velocity.x += meteor->velocity.x * meteor->radius * METEOR_COLLISION_DAMPING ;
                    meteor1->velocity.y += meteor->velocity.y * meteor->radius * METEOR_COLLISION_DAMPING ;
                    meteor->velocity.x +=  tempMeteor1Vel.x * meteor1->radius * METEOR_COLLISION_DAMPING;
                    meteor->velocity.y +=  tempMeteor1Vel.y * meteor1->radius * METEOR_COLLISION_DAMPING;
                }
            }


        }

        // Update meteors
        for (int i = 0; i < MAX_METEORS; i++) {
            Meteor *meteor = &meteors[i];
            meteor->position.x += meteor->velocity.x;
            meteor->position.y += meteor->velocity.y;
            // Dampen asteroids over time
            if (abs(meteor->velocity.x) > 0.0f) 
                meteor->velocity.x = Lerp(meteor->velocity.x, 0.0, METEOR_DEACCEL);
            if (abs(meteor->velocity.y) > 0.0f)
                meteor->velocity.y = Lerp(meteor->velocity.y, 0.0, METEOR_DEACCEL);

            int meteorHeight = meteorSprite.height;
            // Meteor wall behaviour
            if (meteor->position.x > width + meteorHeight)
                meteor->position.x = -meteorHeight;
            else if (meteor->position.x < -meteorHeight)
                meteor->position.x = width + meteorHeight;
            if (meteor->position.y > height + meteorHeight)
                meteor->position.y = -meteorHeight;
            else if (meteor->position.y < -meteorHeight)
                meteor->position.y = height + meteorHeight;

        }


        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(starsShader);
                DrawTexture(colorTexture, 0, 0, Color{0, 0, 0, 30});
            EndShaderMode();
            BeginShaderMode(shader);
                DrawTexture(noiseTexture, 0, 0, Color{255, 255, 255, 30});
                DrawTexture(noiseTexture, noiseTexture.width, 0, Color{255, 255, 255, 30});
            EndShaderMode();

            // Draw meteors
            for (int i = 0; i < MAX_METEORS; i++) {
                Meteor *meteor = &meteors[i];
                Rectangle meteorSpriteSourceRec = {0.0f, 0.0f, (float)meteorSprite.width, (float)meteorSprite.height};
                Rectangle meteorSpriteDestRec   = {meteor->position.x, meteor->position.y, (float)meteorSprite.width * meteor->radius, (float)meteorSprite.height * meteor->radius};
                Vector2 meteorTextureOrigin     = { (float)(meteorSprite.width * meteor->radius) / 2, (float)(meteorSprite.height * meteor->radius) / 2 };
                DrawTexturePro(meteorSprite, meteorSpriteSourceRec, meteorSpriteDestRec, meteorTextureOrigin, 0.0, WHITE);
                // DrawCircle(meteor->position.x, meteor->position.y, meteor->radius * 13.0, ORANGE);
            }

            // Draw rocket texture
            DrawTexturePro(sprite, sourceRec, destRec, origin, (float)rocket.rotation, WHITE);
            // DrawCircle(rocket.position.x, rocket.position.y, destRec.width /2 - 10.0f, RED);

            // Rocket Debug Information
            DrawTextEx(ttfFont, TextFormat("Position: %03.0f, %03.0f", rocket.position.x, rocket.position.y), Vector2{20, 20},DEFAULT_FONT_SIZE, 1.0, WHITE );
            DrawTextEx(ttfFont, TextFormat("Velocity: %0.2f, %0.2f", rocket.velocity.x, rocket.velocity.y), Vector2{20, 45},DEFAULT_FONT_SIZE, 1.0, WHITE );
            DrawTextEx(ttfFont, TextFormat("Acceleration: %0.2f", rocket.acceleration), Vector2{20, 70},DEFAULT_FONT_SIZE, 1.0, WHITE );
            DrawTextEx(ttfFont, TextFormat("Rotation: %03.f", rocket.rotation), Vector2{20, 95},DEFAULT_FONT_SIZE, 1.0, WHITE );
            DrawTextEx(ttfFont, TextFormat("Connected to port: %s", portAccessAvailable ? "Yes" : "No"), Vector2{20, 120},DEFAULT_FONT_SIZE, 1.0, WHITE );
            if (portAccessAvailable)
                DrawTextEx(ttfFont, TextFormat("Last run command: %s", command_str_map[lastCommandExecuted]), Vector2{20, 145},DEFAULT_FONT_SIZE, 1.0, WHITE );

            if (currentSelectedMeteorDebug != nullptr) {
                DrawCircleLines(currentSelectedMeteorDebug->position.x, currentSelectedMeteorDebug->position.y, currentSelectedMeteorDebug->radius * 20.0, RED);
                DrawTextEx(ttfFont, TextFormat("%d", currentSelectedMeteorDebug - meteors), currentSelectedMeteorDebug->position, 1.5 * DEFAULT_FONT_SIZE, 1.0, RED);
                DrawTextEx(ttfFont, TextFormat("Position: %03.0f, %03.0f", currentSelectedMeteorDebug->position.x, currentSelectedMeteorDebug->position.y), Vector2{width - 250.0f, 20},DEFAULT_FONT_SIZE, 1.0, WHITE );
                DrawTextEx(ttfFont, TextFormat("Velocity: %0.2f, %0.2f", currentSelectedMeteorDebug->velocity.x, currentSelectedMeteorDebug->velocity.y), Vector2{width - 250.0, 45},DEFAULT_FONT_SIZE, 1.0, WHITE );
            } else {
                DrawTextEx(ttfFont, "[Debug]: Click asteroid to see debug info", Vector2{width - 500.f, 20}, DEFAULT_FONT_SIZE, 1.0, WHITE);
            }

        EndDrawing();
    }
    
    // De-initialization
    UnloadShader(shader); 
    UnloadTexture(sprite);
    CloseWindow();
    

    // Port De-initialization
    write_port_byte(STATUS_PORT, Status::NOT_READY);
    write_port_byte(COMMAND_PORT, Command::NOP);
    free_virtual_device();

    return 0;
}
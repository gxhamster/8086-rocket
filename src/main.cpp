#include <iostream>
#include <sstream>
#include <raylib.h>
#include "virtual.h"

#define ROCKET_SPEED 6.0f
#define ROCKET_ROTATION_SPEED 3.0f
#define ROCKET_ACCELRATION 0.02f

#define COMMAND_PORT 10
#define STATUS_PORT  11

#define DEFAULT_FONT_SIZE 20

typedef struct Rocket {
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float acceleration;
} Rocket;

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
    ROTATE_LEFT
};


char *command_str_map[5] = {
    "Command::NOP",
    "Command::ACCEL_POS",
    "Command::ACCEL_NEG",
    "Command::ROTATE_RIGHT",
    "Command::ROTATE_LEFT",
};

bool portAccessAvailable = false;

int main() {
    const int width = 920;
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

    Texture2D sprite = LoadTexture("resources/rocket.png");
    float frameWidth = sprite.width;
    float frameHeight = sprite.height;
    float frameScaleFactor = 0.1;
    Rocket rocket = Rocket{0.0};
    rocket.position = Vector2{width/2, height/2};


    // Load font
    Font ttfFont = LoadFont("resources/font.ttf");

    Image noiseImage = GenImagePerlinNoise(width, height, 50, 50, 2.5f);
    Texture2D noiseTexture = LoadTextureFromImage(noiseImage);

    // Intialize shader
    Shader shader = LoadShader(0, TextFormat("resources/space_noise.fs", 330));
    int secondsLoc = GetShaderLocation(shader, "seconds");
    int freqXLoc =   GetShaderLocation(shader, "freqX");
    int freqYLoc =   GetShaderLocation(shader, "freqY");
    int ampXLoc =    GetShaderLocation(shader, "ampX");
    int ampYLoc =    GetShaderLocation(shader, "ampY");
    int speedXLoc =  GetShaderLocation(shader, "speedX");
    int speedYLoc =  GetShaderLocation(shader, "speedY");

    // Shader uniform values that can be updated at any time
    float freqX = 10.0f;
    float freqY = 10.0f;
    float ampX = 2.0f;
    float ampY = 2.0f;
    float speedX = 3.0f;
    float speedY = 3.0f;

    float screenSize[2] = { (float)GetScreenWidth(), (float)GetScreenHeight() };
    SetShaderValue(shader, GetShaderLocation(shader, "size"), &screenSize, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader, freqXLoc, &freqX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, freqYLoc, &freqY, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, ampXLoc, &ampX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, ampYLoc, &ampY, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, speedXLoc, &speedX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, speedYLoc, &speedY, SHADER_UNIFORM_FLOAT);

    float seconds = 0.0f;

    // Source rectangle (part of the texture to use for drawing)
    Rectangle sourceRec = { 0.0f, 0.0f, (float)frameWidth, (float)frameHeight };
    // Destination rectangle (screen rectangle where drawing part of texture)
    Rectangle destRec = { rocket.position.x, rocket.position.y, frameWidth * frameScaleFactor, frameHeight * frameScaleFactor };
    // Origin of the texture (rotation/scale point), it's relative to destination rectangle size
    Vector2 origin = { (float)(frameWidth * frameScaleFactor) / 2, (float)(frameHeight * frameScaleFactor) / 2 };

    int command = 0;
    // Disregard NOP commands, Used only for displaying the command.
    int lastCommandExecuted = 0;
    while (!WindowShouldClose())
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
        SetShaderValue(shader, secondsLoc, &seconds, SHADER_UNIFORM_FLOAT);

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


        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(shader);
                DrawTexture(noiseTexture, 0, 0, Color{255, 255, 255, 30});
                DrawTexture(noiseTexture, noiseTexture.width, 0, Color{255, 255, 255, 30});
            EndShaderMode();
            DrawTexturePro(sprite, sourceRec, destRec, origin, (float)rocket.rotation, WHITE);

            // Debug Information
            DrawTextEx(ttfFont, TextFormat("Position: %03.0f, %03.0f", rocket.position.x, rocket.position.y), Vector2{20, 20},DEFAULT_FONT_SIZE, 1.0, WHITE );
            DrawTextEx(ttfFont, TextFormat("Velocity: %0.2f, %0.2f", rocket.velocity.x, rocket.velocity.y), Vector2{20, 45},DEFAULT_FONT_SIZE, 1.0, WHITE );
            DrawTextEx(ttfFont, TextFormat("Acceleration: %0.2f", rocket.acceleration), Vector2{20, 70},DEFAULT_FONT_SIZE, 1.0, WHITE );
            DrawTextEx(ttfFont, TextFormat("Rotation: %03.f", rocket.rotation), Vector2{20, 95},DEFAULT_FONT_SIZE, 1.0, WHITE );
            DrawTextEx(ttfFont, TextFormat("Connected to port: %s", portAccessAvailable ? "Yes" : "No"), Vector2{20, 120},DEFAULT_FONT_SIZE, 1.0, WHITE );
            if (portAccessAvailable)
                DrawTextEx(ttfFont, TextFormat("Last run command: %s", command_str_map[lastCommandExecuted]), Vector2{20, 145},DEFAULT_FONT_SIZE, 1.0, WHITE );
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
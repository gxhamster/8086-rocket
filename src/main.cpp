#include <iostream>
#include <sstream>
#include <raylib.h>
#include "virtual.h"

#define ROCKET_SPEED 6.0f
#define ROCKET_ROTATION_SPEED 3.0f
#define ROCKET_ACCELRATION 0.02f

typedef struct Rocket {
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float acceleration;
} Rocket;


int main() {
    const int width = 920;
    const int height = 600;

    // WRITE_IO_BYTE(110, 5);
    // unsigned char byte =  READ_IO_BYTE(10);
    // std::cout << "Port Value: " << (int) byte << std::endl;
    // getchar();

    InitWindow(width, height, "Rocket Simulation - 8086");
    SetTargetFPS(60);

    Texture2D sprite = LoadTexture("resources/rocket.png");
    float frameWidth = sprite.width;
    float frameHeight = sprite.height;
    float frameScaleFactor = 0.1;
    Rocket rocket = Rocket{0.0};
    rocket.position = Vector2{width/2, height/2};


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
    
    while (!WindowShouldClose())
    {
        if (IsKeyDown(KEY_LEFT))  rocket.rotation -= ROCKET_ROTATION_SPEED;
        if (IsKeyDown(KEY_RIGHT)) rocket.rotation += ROCKET_ROTATION_SPEED;

        rocket.velocity.x = sin(rocket.rotation * DEG2RAD) * ROCKET_SPEED;
        rocket.velocity.y = cos(rocket.rotation * DEG2RAD) * ROCKET_SPEED;

        if (IsKeyDown(KEY_UP))   rocket.acceleration += ROCKET_ACCELRATION;
        if (IsKeyDown(KEY_DOWN)) rocket.acceleration -= ROCKET_ACCELRATION;

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
                // DrawRectangle(0, 0, width, height, RAYWHITE);
                DrawTexture(noiseTexture, 0, 0, Color{255, 255, 255, 30});
                DrawTexture(noiseTexture, noiseTexture.width, 0, Color{255, 255, 255, 30});
            EndShaderMode();
            DrawTexturePro(sprite, sourceRec, destRec, origin, (float)rocket.rotation, WHITE);

            // Debug Information
            DrawText(TextFormat("Position: %03.0f, %03.0f", rocket.position.x, rocket.position.y), 20, 20, 20, WHITE);
            DrawText(TextFormat("Velocity: %0.2f, %0.2f", rocket.velocity.x, rocket.velocity.y), 20, 50, 20, WHITE);
            DrawText(TextFormat("Acceleration: %0.2f", rocket.acceleration), 20, 80, 20, WHITE);
            DrawText(TextFormat("Rotation: %03.f", rocket.rotation), 20, 110, 20, WHITE);
        EndDrawing();
    }
    
    // De-initialization
    UnloadShader(shader); 
    UnloadTexture(sprite);

    CloseWindow();
    return 0;
}
#pragma once

class Time
{
public:
    // Called once per frame (in main loop)
    static void Update();

    // Seconds since last frame
    static float DeltaTime();

    // Total time since engine start
    static float TimeSinceStart();

private:
    static double s_LastTime;
    static float  s_DeltaTime;
};

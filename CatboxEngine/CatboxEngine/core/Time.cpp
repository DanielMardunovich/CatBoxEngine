#include "Time.h"
#include <glfw3.h>

double Time::s_LastTime = 0.0;
float  Time::s_DeltaTime = 0.0f;

void Time::Update()
{
    double currentTime = glfwGetTime();

    if (s_LastTime == 0.0)
    {
        s_LastTime = currentTime;
        s_DeltaTime = 0.0f;
        return;
    }

    double frameTime = currentTime - s_LastTime;
    s_LastTime = currentTime;

    // Clamp delta time (important for stability)
    const double maxDelta = 0.1; // 100 ms
    if (frameTime > maxDelta)
        frameTime = maxDelta;

    s_DeltaTime = static_cast<float>(frameTime);
}

float Time::DeltaTime()
{
    return s_DeltaTime;
}

float Time::TimeSinceStart()
{
    return static_cast<float>(glfwGetTime());
}

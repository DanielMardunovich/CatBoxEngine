#include "RecordTimeSystem.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <direct.h>   // _mkdir

RecordTimeSystem::RecordTimeSystem()
{
    LoadRecords();
}

void RecordTimeSystem::Start()
{
    m_elapsed = 0.0f;
    m_running = true;
}

void RecordTimeSystem::Stop()
{
    m_running = false;
}

void RecordTimeSystem::Reset()
{
    m_elapsed = 0.0f;
    m_running = false;
}

void RecordTimeSystem::Update(float deltaTime)
{
    if (m_running)
        m_elapsed += deltaTime;
}

bool RecordTimeSystem::SubmitTime(const std::string& levelPath, float time)
{
    auto it = m_records.find(levelPath);
    bool isNewBest = (it == m_records.end() || time < it->second);
    if (isNewBest)
    {
        m_records[levelPath] = time;
        SaveRecords();
    }
    return isNewBest;
}

float RecordTimeSystem::GetBestTime(const std::string& levelPath) const
{
    auto it = m_records.find(levelPath);
    return (it != m_records.end()) ? it->second : -1.0f;
}

bool RecordTimeSystem::HasRecord(const std::string& levelPath) const
{
    return m_records.find(levelPath) != m_records.end();
}

void RecordTimeSystem::LoadRecords()
{
    std::ifstream in(k_recordsPath);
    if (!in.is_open())
        return;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        try { m_records[key] = std::stof(val); } catch (...) {}
    }

    std::cout << "Records loaded: " << m_records.size() << " entries" << std::endl;
}

void RecordTimeSystem::SaveRecords() const
{
    std::ofstream out(k_recordsPath);
    if (!out.is_open())
    {
        std::cerr << "Failed to save records to: " << k_recordsPath << std::endl;
        return;
    }

    out << "# CatboxEngine Level Records\n";
    for (const auto& entry : m_records)
        out << entry.first << "=" << entry.second << "\n";
}

std::string RecordTimeSystem::FormatTime(float seconds)
{
    if (seconds < 0.0f)
        return "--:--.--";

    int   mins = static_cast<int>(seconds / 60.0f);
    float secs = seconds - static_cast<float>(mins) * 60.0f;

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d:%05.2f", mins, secs);
    return buf;
}

#pragma once
#include <string>
#include <unordered_map>

// Tracks elapsed play-time and persists per-level best times to disk.
class RecordTimeSystem
{
public:
    RecordTimeSystem();

    // Timer control
    void Start();           // Reset and begin counting
    void Stop();            // Pause the timer (keeps current elapsed value)
    void Reset();           // Stop and zero the timer
    void Update(float deltaTime);

    float GetCurrentTime() const { return m_elapsed; }
    bool  IsRunning()      const { return m_running; }

    // Records — returns true when the submitted time beats the stored best
    bool  SubmitTime(const std::string& levelPath, float time);
    float GetBestTime(const std::string& levelPath) const;
    bool  HasRecord(const std::string& levelPath)   const;

    const std::unordered_map<std::string, float>& GetAllRecords() const { return m_records; }

    // Persistence
    void LoadRecords();
    void SaveRecords() const;

    // "m:ss.cc" format; negative input returns "--:--.--"
    static std::string FormatTime(float seconds);

private:
    float m_elapsed = 0.0f;
    bool  m_running = false;

    std::unordered_map<std::string, float> m_records;

    static constexpr const char* k_recordsPath = "records.dat";
};

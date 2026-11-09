#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <atomic>

class PuzzleHunterGUI {
public:
    PuzzleHunterGUI();
    ~PuzzleHunterGUI();

    bool Initialize();
    void Run();
    void Shutdown();

    std::string symbols;
    int numThreads;  // ДОБАВЛЕНО: количество потоков (должно быть public для ImGui)
    bool isRunning;
    std::atomic<uint64_t> processedCount;  // ДОЛЖЕН БЫТЬ PRIVATE

private:
    void Render();
    void AddTarget();
    void RemoveTarget(int index);
    void SolverThread();
    void LoadTargetsFromFile();

    std::vector<std::string> targetHexList;
    std::thread* solverThread;
    std::atomic<bool> solverRunning;
};
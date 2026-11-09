#include "gui.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <iostream>
#include <cstring>
#include <comdef.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <ctime>
#include <mutex>
#include <algorithm>
#include <commdlg.h>
#include "multiset_permutation.h"
#include "bitcoin.h"
#include "targets.h"
#include "cuda_batch.h"
#include "gpu_solver_v2.h"

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static HWND g_hwnd = nullptr;

// Глобальный указатель на флаг остановки для перехвата закрытия окна
static std::atomic<bool>* g_solverRunningPtr = nullptr;

static std::mutex g_logMutex;

static bool HexToHash160(const std::string& hex, uint8_t out[20]) {
    if (hex.size() != 40) return false;
    for (size_t i = 0; i < 20; ++i) {
        unsigned int byte = 0;
        std::istringstream iss(hex.substr(i * 2, 2));
        iss >> std::hex >> byte;
        if (iss.fail()) return false;
        out[i] = (uint8_t)byte;
    }
    return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    
    // Обработка закрытия окна (исправлено: теперь этот код выполняется)
    if (msg == WM_CLOSE) {
        if (g_solverRunningPtr) {
            *g_solverRunningPtr = false;
        }
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

PuzzleHunterGUI::PuzzleHunterGUI() 
    : isRunning(false), solverThread(nullptr), solverRunning(false), processedCount(0), numThreads(6)
{
    symbols = "17B88F967189FD9695";
    targetHexList.push_back("b153537ff1d7e16c24dd33583cb644f5bb4a9412");
}

PuzzleHunterGUI::~PuzzleHunterGUI() {
    Shutdown();
}

void PuzzleHunterGUI::SolverThread() {
    std::cout << "Solver V2 thread started with symbols: " << symbols << std::endl;
    
    GPUSolverV2 solver;
    
    for (const auto& targetHex : targetHexList) {
        uint8_t hash160[20];
        if (HexToHash160(targetHex, hash160)) {
            solver.AddTarget(hash160);
        } else {
            std::cerr << "WARNING: Invalid target hex: " << targetHex << std::endl;
        }
    }
    
    uint64_t total = CountUniquePermutations(symbols);
    std::cout << "Total permutations: " << total << std::endl;
    
    uint64_t N = total;
    
    solver.ScanPermutations(symbols, N, numThreads, solverRunning);
    
    solverRunning = false;
    isRunning = false;
    
    std::cout << "Solver thread finished. You can close this window." << std::endl;
}

bool PuzzleHunterGUI::Initialize() {
    std::cout << "Initializing COM..." << std::endl;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "CoInitializeEx failed! Error: " << hr << std::endl;
        return false;
    }

    std::cout << "Initializing window..." << std::endl;
    
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, _T("PuzzleHunter"), nullptr };
    if (!::RegisterClassEx(&wc)) {
        std::cerr << "RegisterClassEx failed! Error: " << GetLastError() << std::endl;
        return false;
    }

    // ИСПРАВЛЕНИЕ: Делаем окно ФИКСИРОВАННОГО размера. 
    // Это полностью устраняет артефакты при перемещении и предотвращает краши при ресайзе.
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT windowRect = {0, 0, 520, 580}; // Оптимальный размер под все кнопки
    AdjustWindowRect(&windowRect, windowStyle, FALSE);
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;

    g_hwnd = ::CreateWindow(wc.lpszClassName, _T("PuzzleHunter GPU Solver"), windowStyle, 100, 100, width, height, nullptr, nullptr, wc.hInstance, nullptr);
    if (g_hwnd == nullptr) {
        std::cerr << "CreateWindow failed! Error: " << GetLastError() << std::endl;
        return false;
    }
    
    g_solverRunningPtr = &solverRunning;
    std::cout << "Window created." << std::endl;

    std::cout << "Initializing DirectX 11..." << std::endl;
    
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (FAILED(hr) || g_pd3dDevice == nullptr) {
        std::cerr << "D3D11CreateDevice failed! Error: " << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }
    std::cout << "D3D11 device created. Feature level: " << featureLevel << std::endl;

    std::cout << "Creating DXGI factory..." << std::endl;
    IDXGIFactory1* dxgiFactory = nullptr;
    hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&dxgiFactory);
    if (FAILED(hr) || dxgiFactory == nullptr) {
        std::cerr << "CreateDXGIFactory1 failed! Error: " << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }

    IDXGIAdapter* adapter = nullptr;
    hr = dxgiFactory->EnumAdapters(0, &adapter);
    if (FAILED(hr) || adapter == nullptr) {
        std::cerr << "EnumAdapters failed! Error: " << _com_error(hr).ErrorMessage() << std::endl;
        dxgiFactory->Release();
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = width;  // ИСПРАВЛЕНО: явное указание ширины
    sd.BufferDesc.Height = height; // ИСПРАВЛЕНО: явное указание высоты
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    std::cout << "Creating swap chain..." << std::endl;
    hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
    dxgiFactory->Release();
    adapter->Release();
    if (FAILED(hr) || g_pSwapChain == nullptr) {
        std::cerr << "CreateSwapChain failed! Error: " << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }

    std::cout << "Creating render target view..." << std::endl;
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr) || pBackBuffer == nullptr) {
        std::cerr << "GetBuffer failed! Error: " << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }
    
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr) || g_mainRenderTargetView == nullptr) {
        std::cerr << "CreateRenderTargetView failed! Error: " << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }

    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

    std::cout << "Initializing ImGui..." << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hwnd);
    std::cout << "GUI Initialized successfully!" << std::endl;

    return true;
}

void PuzzleHunterGUI::Render() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Обновляем размер дисплея
    RECT clientRect;
    GetClientRect(g_hwnd, &clientRect);
    io.DisplaySize = ImVec2((float)(clientRect.right - clientRect.left), 
                            (float)(clientRect.bottom - clientRect.top));
    
    // ИСПРАВЛЕНИЕ: Очистка фона перед рендерингом устраняет визуальные артефакты
    const float clear_color[] = {0.15f, 0.15f, 0.15f, 1.0f};
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // AlwaysAutoResize подстраивает окно под содержимое
    ImGui::Begin("PuzzleHunter Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Symbols:");
    char symbolsBuf[256];
    strncpy(symbolsBuf, symbols.c_str(), sizeof(symbolsBuf));
    symbolsBuf[sizeof(symbolsBuf) - 1] = '\0';
    if (ImGui::InputText("##symbols", symbolsBuf, sizeof(symbolsBuf))) {
        symbols = symbolsBuf;
    }
    
    int maxThreads = std::thread::hardware_concurrency();
    if (maxThreads == 0) maxThreads = 8;
    ImGui::SliderInt("CPU Threads", &numThreads, 1, maxThreads);
    ImGui::Text("Max available: %d", maxThreads);
    
    ImGui::Separator();
    ImGui::Text("Targets (HASH160 hex):");
    
    for (int i = 0; i < (int)targetHexList.size(); i++) {
        ImGui::PushID(i);
        char buf[256];
        strncpy(buf, targetHexList[i].c_str(), sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        
        if (ImGui::InputText("##target", buf, sizeof(buf))) {
            targetHexList[i] = buf;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Remove")) {
            RemoveTarget(i);
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }
    
    ImGui::Separator();
    
    if (ImGui::Button("Add Target")) {
        AddTarget();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load from File...")) {
        LoadTargetsFromFile();
    }
    
    ImGui::Separator();
    
    if (isRunning) {
        if (ImGui::Button("Stop")) {
            solverRunning = false;
        }
        ImGui::SameLine();
        std::string status = "Running... Check console for progress.";
        ImGui::Text("%s", status.c_str());
    } else {
        if (ImGui::Button("Start")) {
            isRunning = true;
            solverRunning = true;
            processedCount = 0;
            solverThread = new std::thread(&PuzzleHunterGUI::SolverThread, this);
        }
    }
    
    ImGui::End();

    ImGui::Render();
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(1, 0);
}

void PuzzleHunterGUI::AddTarget() {
    targetHexList.push_back("0000000000000000000000000000000000000000");
}

void PuzzleHunterGUI::RemoveTarget(int index) {
    if (index >= 0 && index < (int)targetHexList.size()) {
        targetHexList.erase(targetHexList.begin() + index);
    }
}

void PuzzleHunterGUI::LoadTargetsFromFile() {
    OPENFILENAME ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        std::ifstream file(szFile);
        if (file.is_open()) {
            std::string line;
            int addedCount = 0;
            while (std::getline(file, line)) {
                line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
                if (line.size() == 40) {
                    bool exists = false;
                    for (const auto& t : targetHexList) {
                        if (t == line) { exists = true; break; }
                    }
                    if (!exists) {
                        targetHexList.push_back(line);
                        addedCount++;
                    }
                }
            }
            std::cout << "Loaded " << addedCount << " targets from " << szFile << std::endl;
            file.close();
        } else {
            std::cerr << "Failed to open file: " << szFile << std::endl;
        }
    }
}

void PuzzleHunterGUI::Run() {
    std::cout << "Starting message loop..." << std::endl;
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }
        Render();
    }
    std::cout << "Message loop ended." << std::endl;
}

void PuzzleHunterGUI::Shutdown() {
    std::cout << "\n[Shutdown] Initiating graceful shutdown..." << std::endl;
    
    solverRunning = false;
    
    if (solverThread && solverThread->joinable()) {
        std::cout << "[Shutdown] Waiting for solver threads to finish..." << std::endl;
        solverThread->join();
        std::cout << "[Shutdown] Solver threads joined successfully." << std::endl;
    }
    
    delete solverThread;
    solverThread = nullptr;
    
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_mainRenderTargetView) g_mainRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
    
    CoUninitialize();
    std::cout << "[Shutdown] Complete." << std::endl;
}
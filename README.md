# PuzzleHunter

Солвер Bitcoin Puzzle: перебор уникальных перестановок мультимножества hex-символов.
Каждая перестановка интерпретируется как приватный ключ (дополняется ведущими нулями до 64 hex),
для него вычисляется публичный ключ (secp256k1) и HASH160 (SHA256+RIPEMD160), результат сверяется
со списком целей. Совпадения пишутся в `found_keys.log`.

## Возможности
- GUI на ImGui (DirectX11): строка символов, ползунок CPU Threads, редактируемый список целей,
  загрузка целей из файла, Start/Stop.
- Гибридная архитектура: CPU генерирует кандидаты и публичные ключи (EC-умножение),
  GPU (CUDA) пакетно считает HASH160.
- Возобновление прогресса: `progress.dat` (строка символов + индекс). Смена строки = старт с нуля.
- Корректное завершение: кнопка Stop, закрытие окна, Ctrl+C / закрытие консоли — прогресс сохраняется.
- Байт-ориентированный генератор перестановок без строк в горячем цикле, предвычисленные факториалы.

## Структура
PuzzleHunter/
├─ CMakeLists.txt
├─ src/
│ ├─ main.cpp входная точка (Run + Shutdown)
│ ├─ gui.h / gui.cpp ImGui GUI (Win32 + DX11), фикс. окно, WM_CLOSE
│ ├─ gpu_solver_v2.h/.cpp солвер: перебор индексов, EC на CPU, батчи на GPU
│ ├─ cuda_batch.h/.cu CUDA-процессор пакетного HASH160
│ ├─ multiset_permutation.* генератор k-й перестановки мультимножества (байты, факториалы)
│ └─ bitcoin.* / hash.* / targets.* вспомогательные модули
└─ external/
├─ vanity/ VanitySearch (Jean-Luc Pons, GPL-3.0): Int, Point, Secp256K1, GPU-хеши
├─ secp256k1/ libsecp256k1 (prebuilt x64 lib)
└─ imgui/ Dear ImGui (MIT)


## Требования (Windows)
- Visual Studio **2022** Build Tools: workload «Desktop development with C++»
  (MSVC v143, Windows 10 SDK, «C++ CMake tools for Windows»).
  CUDA 12.x не поддерживает MSVC из VS 2025/2026 официально — флаг
  `-allow-unsupported-compiler` уже добавлен в CMakeLists, но предпочтителен VS 2022.
- CUDA Toolkit 12.x (Development + Runtime).
- Ninja (`python -m pip install ninja` или из компонента VS).

## Сборка
Из меню Пуск: **x64 Native Tools Command Prompt for VS 2022**:
```cmd
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .

Архитектура GPU по умолчанию: sm_86 (CMAKE_CUDA_ARCHITECTURES).
Для других поколений расширь список (например, 61 75 86 89 + PTX compute_86).
Запуск
PuzzleHunter.exe. Файлы progress.dat и found_keys.log создаются в рабочей директории.
Ползунок CPU Threads: для полного перебора EC-математика живёт на CPU — ставь по числу потоков CPU.

Производительность (измерено)
Машина	Скорость	Примечание
2 ядра CPU + RTX 3080	~201k keys/s	упор в CPU EC
Ryzen 5950X + RTX 3060 Ti	~1.7M keys/s	упор в CPU EC, GPU считает только HASH160

Архитектурное ограничение: EC-умножение = 90–99% стоимости и выполняется на CPU;
GPU занят 1–10% (HASH160). Перенос EC на GPU — в roadmap.
Troubleshooting
unsupported Microsoft Visual Studio version → использовать VS 2022 (или флаг уже в CMakeLists).
No CUDA toolset found при генераторе VS → собирать Ninja из x64 Native Tools prompt.
Нет cudart64_12.dll на целевой машине → set(CMAKE_CUDA_RUNTIME_LIBRARY Static) до project() + чистая пересборка.
Коллизия SECP256K1.h / secp256k1.h на Windows (case-insensitive): в .cu подключать заголовки vanity явным путём ../external/vanity/....
Нет GPU → CPU fallback (в версии cuda_batch с флагом cpu_fallback).
Лицензии и третьи стороны
VanitySearch (external/vanity): GPL-3.0, Copyright (c) 2020 Jean Luc PONS.
Dear ImGui: MIT. libsecp256k1: MIT.
Собственный код проекта распространяется на условиях GPL-3.0 (совместимость с VanitySearch).
Проект образовательный; использование на свой риск.


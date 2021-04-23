# Optimization methods lab

## Requirements
* `qt5`
* `qt5charts` (arch `qt5-charts`, ubuntu `libqt5charts5` and `libqt5chats5-dev`)
* `cmake` >= 3.8
* `c++` compiler with C++20 support (tested with `g++ v10.2.0`)

## Build on *nix-like
* build with `./build.sh <configuration> [args for cmake...]`, e. g. `./build.sh Release`
* rebuild with `./build.sh`

## Build on Windows
* build with `build.bat <configuration> [args for cmake...]`, e. g. `build.bat Release`
* rebuild with `build.bat`

## Packaging on *nix-like
* build in `Release`
* `cd` to build directory
* run `cpack -C Release -G "STGZ"`


## Project structure
```
.
├── cmake                     - вспомогательные cmake листы: генераторы проектов, информация для запаковки
├── doc                       - отчеты
│   ├── 1
│   └── 2
├── include
│   └── opt-methods
│       ├── approximators     - одномерные методы оптимизации
│       ├── coroutines        - поддержка корутин
│       ├── math              - миниатюрная библиотека математики
│       ├── multidim          - многомерные методы оптимизации
│       ├── solvers           - общий интерфейс аппроксиматоров
│       │   └── function      - ``стертая'' функция с поддержкой градиента
│       └── util              - вспомогательный модуль для визуализации
├── misc
└── src                       - реализации пользовательских приложений, сборщиков статистики
    ├── app1
    ├── app2
    ├── helper1-eps2complexity
    ├── helper1-ranges
    ├── helper2-multidim
    └── lib                   - реализация нешаблонных методов
```

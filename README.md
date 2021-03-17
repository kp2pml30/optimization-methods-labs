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
┣ [doc](/doc/1/) — Отчет и данные, полученные в результате работы  
┣ [opt-methods](/include/opt-methods/) — Реализация алгоритмов минимизации  
┊&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;┣ [approximators](/include/opt-methods/approximators/) — Методы дихотомии, золотого сечения, Фиббоначи, парабол, Брента  
┊&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;┣ [solvers](/include/opt-methods/solvers/) — Модуль стягивания отрезка поиска  
┊&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;┗ [util](/include/opt-methods/util/) — Вспомогательный модуль для отрисовки графиков  
┣ [src](/src/) — Группа приложений лабораторной работы  
┊&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;┣ [app1](/src/app1/) — Основное приложение с графическим интерфейсом  
┊&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;┣ [helper1-eps2complexity](/src/helper1-eps2complexity/) — Вспомогательное приложение для получения графиков  
┊&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;┗ [helper1-ranges](/src/helper1-ranges/) — Вспомогательное приложение для получения таблиц  
┗ [build.sh](/build.sh) — Скрипт для сборки приложения под *\*nix-like* ОС.

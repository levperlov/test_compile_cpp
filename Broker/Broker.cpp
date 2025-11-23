/**
 * @mainpage Broker
 *
 * @section description Описание проекта
 * **Broker** — это управляющий сервис (оркестратор) для автоматизированной сборки
 * и компиляции сетевых проектов (NoC — Network on Chip).
 *
 * Программа координирует взаимодействие между несколькими независимыми компонентами:
 * - **Project_manager** — создание, открытие, переименование или удаление проекта;
 * - **Graph_verilog_generator** — генерация описания сети (графа) и Verilog-файлов;
 * - **Quartus_compiler** — автоматическая компиляция проекта Quartus;
 * - **Database_writer** — запись итоговых данных в базу данных.
 *
 * Broker позволяет запускать любую комбинацию этапов и обеспечивает их корректную последовательность.
 *
 * Поддерживаются ОС **Windows** и **Linux**.
 *
 * @section usage Использование
 * Пример запуска:
 * @code
 * Broker --project -n MyProject -l ./projects --create
 * Broker --graph -l ./projects -n MyProject --params "Nx=4 Ny=4"
 * Broker --quartus -l ./projects -n MyProject
 * Broker --database -l ./projects -n MyProject --write
 * @endcode
 *
 * @section metadata Метаданные проекта
 * Каждый проект содержит JSON-файл `<имя>_metadata.json`, в котором хранится состояние этапов:
 * - **graphVerilogMetadata** — информация о сериализации и генерации графа;
 * - **quartusMetadata** — состояние компиляции Quartus;
 * - **databaseMetadata** — информация о записи данных в БД.
 *
 * При каждом запуске Broker обновляет эти флаги, чтобы исключить повторные или некорректные действия.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <cstdlib>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#define PLATFORM_WINDOWS
#else
#include <unistd.h>
#include <sys/wait.h>
#define PLATFORM_LINUX
#endif

#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

/**
 * @brief Структура метаданных проекта
 *
 * Описывает текущее состояние основных стадий разработки проекта NoC:
 * - сериализация и генерация Verilog;
 * - компиляция Quartus;
 * - запись в базу данных.
 */
struct ProjectSettings {
    struct {
        bool graphSerialized = false;  ///< Граф проекта сериализован
        bool verilogGenerated = false; ///< Сгенерированы Verilog-файлы
    } graphVerilogMetadata;

    struct {
        bool quartusCompiled = false;  ///< Проект успешно скомпилирован в Quartus
    } quartusMetadata;

    struct {
        bool writtenToDB = false;      ///< Данные проекта записаны в БД
    } databaseMetadata;
};

/**
 * @brief Сбрасывает флаги метаданных проекта до указанного этапа.
 *
 * Используется для "отката" прогресса при повторном запуске этапов.
 *
 * @param jsonPath Путь к JSON-файлу с метаданными проекта.
 * @param stage Этап (0–3):
 * - `0` — сброс всех флагов;
 * - `1` — сброс до стадии генерации Verilog;
 * - `2` — сброс до стадии компиляции Quartus;
 * - `3` — сброс до стадии записи в базу данных.
 */
void uncheckMetadata(const std::string& jsonPath, int stage) {
    if (!fs::exists(jsonPath)) {
        std::cerr << "Metadata file not found: " << jsonPath << std::endl;
        return;
    }

    std::ifstream in(jsonPath);
    json j;
    in >> j;

    ProjectSettings ps;

    ps.graphVerilogMetadata.graphSerialized = j["graphVerilogMetadata"]["graphSerialized"];
    ps.graphVerilogMetadata.verilogGenerated = j["graphVerilogMetadata"]["verilogGenerated"];
    ps.quartusMetadata.quartusCompiled = j["quartusMetadata"]["quartusCompiled"];
    ps.databaseMetadata.writtenToDB = j["databaseMetadata"]["writtenToDB"];

    if (stage <= 3) ps.databaseMetadata.writtenToDB = false;
    if (stage <= 2) ps.quartusMetadata.quartusCompiled = false;
    if (stage <= 1) ps.graphVerilogMetadata.verilogGenerated = false;
    if (stage == 0) ps.graphVerilogMetadata.graphSerialized = false;

    j["graphVerilogMetadata"]["graphSerialized"] = ps.graphVerilogMetadata.graphSerialized;
    j["graphVerilogMetadata"]["verilogGenerated"] = ps.graphVerilogMetadata.verilogGenerated;
    j["quartusMetadata"]["quartusCompiled"] = ps.quartusMetadata.quartusCompiled;
    j["databaseMetadata"]["writtenToDB"] = ps.databaseMetadata.writtenToDB;

    std::ofstream out(jsonPath);
    out << std::setw(4) << j;
}

/**
 * @brief Запускает внешний процесс и отображает его вывод.
 *
 * На Windows использует API `CreateProcess`, на Linux — стандартный вызов `system()`.
 *
 * @param command Полная строка с командой для выполнения.
 * @return Код возврата внешнего процесса (0 — успех).
 */
int runProcess(const std::string& command) {
    std::cout << "Executing: " << command << std::endl;
#ifdef PLATFORM_WINDOWS
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    std::string cmd = "cmd /C " + command;
    if (!CreateProcessA(NULL, cmd.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to start process: " << command << std::endl;
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(exitCode);
#else
    int ret = system(command.c_str());
    return WEXITSTATUS(ret);
#endif
}

/**
 * @brief Главная функция программы.
 *
 * Анализирует аргументы командной строки, определяет какие сервисы запускать,
 * и последовательно вызывает их с корректными параметрами.
 *
 * Поддерживаемые режимы:
 * - `--project` — управление проектами;
 * - `--graph` — генерация графа и Verilog-файлов;
 * - `--quartus` — компиляция проекта Quartus;
 * - `--database` — запись итогов в базу данных;
 * - `--help` — отображение справки.
 *
 * @param argc Количество аргументов командной строки.
 * @param argv Массив аргументов.
 * @return Код возврата (0 — успех, 1 — ошибка).
 */
int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        std::cout << "Use --help to see usage information.\n";
        return 0;
    }

    bool launch_manager = false, launch_graph = false, launch_quartus = false, launch_db = false;
    std::string project_name, project_location, project_new_name, project_action = "o";
    std::string graph_args, quartus_args, db_args;
    std::string key_arg;

    try {
        for (size_t i = 0; i < args.size(); ++i) {
            std::string arg = args[i];

            if (arg == "--project") {
                launch_manager = true;
                bool project_breaker = false;
                while (!project_breaker && i + 1 < args.size()) {
                    std::string next = args[++i];
                    if (next == "-n" || next == "--name") project_name = args[++i];
                    else if (next == "-l" || next == "--location") project_location = args[++i];
                    else if (next == "-o" || next == "--open") project_action = "o";
                    else if (next == "-c" || next == "--create") project_action = "c";
                    else if (next == "-e" || next == "--erase") project_action = "e";
                    else if (next == "-r" || next == "--rename") {
                        project_action = "r";
                        project_new_name = args[++i];
                    }
                    else {
                        i--;
                        project_breaker = true;
                    }
                }
            }
            else if (arg == "--graph") {
                launch_graph = true;
                key_arg = arg;
            }
            else if (arg == "--quartus") {
                launch_quartus = true;
                key_arg = arg;
            }
            else if (arg == "--database") {
                launch_db = true;
                key_arg = arg;
            }
            else if (arg == "-h" || arg == "--help") {
                std::ifstream helpFile("help.txt");
                if (helpFile.is_open()) {
                    std::cout << helpFile.rdbuf();
                }
                else {
                    std::cerr << "help.txt not found.\n";
                }
                return 0;
            }
            else {
                if (key_arg.empty()) {
                    std::cerr << "Invalid argument: " << arg << std::endl;
                    return 1;
                }
                if (key_arg == "--graph") graph_args += " " + arg;
                else if (key_arg == "--quartus") quartus_args += " " + arg;
                else if (key_arg == "--database") db_args += " " + arg;
            }
        }
    }
    catch (...) {
        std::cerr << "Argument parsing error.\n";
        return 1;
    }

#ifdef PLATFORM_WINDOWS
    const std::string manager_exec = "../../../../../Project_manager/Project_manager/bin/Debug/net8.0/Project_manager.exe";
    const std::string veriloger_exec = "../../../../../Graph_verilog_generator/Graph_verilog_generator/bin/Debug/net8.0/Graph_verilog_generator.exe";
    const std::string quartus_exec = "../../../../../Quartus_compiler/Quartus_compiler/bin/Debug/net8.0/Quartus_compiler.exe";
    const std::string db_exec = "../../../../../Database_writer/Database_writer/bin/Debug/net8.0/Database_writer.exe";
#else
    const std::string manager_exec = "../../../../../Project_manager/Project_manager";
    const std::string veriloger_exec = "../../../../../Graph_verilog_generator/Graph_verilog_generator";
    const std::string quartus_exec = "../../../../../Quartus_compiler/Quartus_compiler";
    const std::string db_exec = "../../../../../Database_writer/Database_writer";
#endif

    if (launch_manager) {
        std::ostringstream ss;
        ss << manager_exec << " -l " << project_location << " -n " << project_name
            << " -" << project_action;
        if (project_action == "r") ss << " " << project_new_name;
        int res = runProcess(ss.str());
        if (res != 0) {
            std::cerr << "Project_manager failure.\n";
            return 1;
        }
        std::cout << "Project_manager success.\n";
        if (project_action == "r") project_name = project_new_name;
    }

    if (launch_graph) {
        uncheckMetadata(project_location + "/" + project_name + "_metadata.json", 0);
        std::ostringstream ss;
        ss << veriloger_exec << " -l " << project_location << " -n " << project_name << graph_args;
        int res = runProcess(ss.str());
        if (res != 0) {
            std::cerr << "Graph_verilog_generator failure.\n";
            return 1;
        }
        std::cout << "Graph_verilog_generator success.\n";
    }

    if (launch_quartus) {
        uncheckMetadata(project_location + "/" + project_name + "_metadata.json", 2);
        std::ostringstream ss;
        ss << quartus_exec << " -l " << project_location << " -n " << project_name;
        int res = runProcess(ss.str());
        if (res != 0) {
            std::cerr << "Quartus_compiler failure.\n";
            return 1;
        }
        std::cout << "Quartus_compiler success.\n";
    }

    if (launch_db) {
        uncheckMetadata(project_location + "/" + project_name + "_metadata.json", 3);
        std::ostringstream ss;
        ss << db_exec << " -l " << project_location << " -n " << project_name << db_args;
        int res = runProcess(ss.str());
        if (res != 0) {
            std::cerr << "Database_writer failure.\n";
            return 1;
        }
        std::cout << "Database_writer success.\n";
    }

    return 0;
}



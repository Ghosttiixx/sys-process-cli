#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>

struct ProcessInfo {
    DWORD id;
    std::string name;
    SIZE_T memoryUsage;
};

double bytesToMB(SIZE_T bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

std::vector<ProcessInfo> getAllProcesses() {
    std::vector<ProcessInfo> processes;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            ProcessInfo proc;
            proc.id = pe32.th32ProcessID;

            #ifdef UNICODE
                std::wstring wName(pe32.szExeFile);
                proc.name = std::string(wName.begin(), wName.end());
            #else
                proc.name = std::string(pe32.szExeFile);
            #endif

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, proc.id);
            if (hProcess) {
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    proc.memoryUsage = pmc.WorkingSetSize;
                } else {
                    proc.memoryUsage = 0;
                }
                CloseHandle(hProcess);
            } else {
                proc.memoryUsage = 0;
            }

            processes.push_back(proc);
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processes;
}

void printTopMemoryProcesses(int limit = 10) {
    auto processes = getAllProcesses();

    std::sort(processes.begin(), processes.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        return a.memoryUsage > b.memoryUsage;
    });

    std::cout << "\n=======================================================\n";
    std::cout << "          ТОП " << limit << " ПРОЦЕССОВ ПО ИСПОЛЬЗОВАНИЮ RAM          \n";
    std::cout << "=======================================================\n";
    std::cout << std::left << std::setw(10) << "PID" 
              << std::setw(30) << "Имя процесса" 
              << std::setw(15) << "Память (MB)" << "\n";
    std::cout << "-------------------------------------------------------\n";

    int count = 0;
    for (const auto& proc : processes) {
        if (count >= limit) break;
        std::cout << std::left << std::setw(10) << proc.id 
                  << std::setw(30) << proc.name 
                  << std::fixed << std::setprecision(2) 
                  << std::setw(15) << bytesToMB(proc.memoryUsage) << "\n";
        count++;
    }
    std::cout << "=======================================================\n\n";
}

void killProcessById(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        std::cout << "[!] Не удалось открыть процесс PID: " << pid << "\n";
        return;
    }

    if (TerminateProcess(hProcess, 0)) {
        std::cout << "[+] Процесс PID " << pid << " успешно завершен.\n";
    } else {
        std::cout << "[!] Ошибка при попытке завершить процесс PID " << pid << ".\n";
    }

    CloseHandle(hProcess);
}

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    int choice = 0;
    while (true) {
        std::cout << "=== C++ System Process Inspector ===\n";
        std::cout << "1. Показать ТОП-10 процессов по RAM\n";
        std::cout << "2. Показать ТОП-25 процессов по RAM\n";
        std::cout << "3. Завершить процесс по PID\n";
        std::cout << "4. Выход\n";
        std::cout << "Выберите действие: ";
        std::cin >> choice;

        if (choice == 1) {
            printTopMemoryProcesses(10);
        } else if (choice == 2) {
            printTopMemoryProcesses(25);
        } else if (choice == 3) {
            DWORD pid;
            std::cout << "Введите PID процесса для завершения: ";
            std::cin >> pid;
            killProcessById(pid);
        } else if (choice == 4) {
            break;
        } else {
            std::cout << "Неверный ввод, попробуйте снова.\n\n";
        }
    }

    return 0;
}
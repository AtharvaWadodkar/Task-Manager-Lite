#include <stdio.h>
#include <windows.h>
#include <psapi.h>
#include <pdh.h>

PDH_HQUERY query;
PDH_HCOUNTER totalCpuCounter;
DWORD lastTotalValue = 0;
int numProcessors = 0;

void initializeCpuUsage() {
    PDH_STATUS status;

    status = PdhOpenQuery(NULL, 0, &query);
    if (status != ERROR_SUCCESS) {
        printf("PdhOpenQuery failed with status 0x%x\n", status);
        return;
    }

    // Add a counter to monitor total CPU usage
    status = PdhAddCounter(query, L"\\Processor(_Total)\\% Processor Time", 0, &totalCpuCounter);
    if (status != ERROR_SUCCESS) {
        printf("PdhAddCounter failed with status 0x%x\n", status);
        PdhCloseQuery(query);
        return;
    }

    status = PdhCollectQueryData(query);
    if (status != ERROR_SUCCESS) {
        printf("PdhCollectQueryData failed with status 0x%x\n", status);
        PdhCloseQuery(query);
        return;
    }

    // Determine the number of processors in the system
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    // Sleep for a short duration to collect initial data
    Sleep(1000);
}

float calculateCpuUsage(DWORD processId) {
    PDH_FMT_COUNTERVALUE value;
    PDH_STATUS status;

    status = PdhCollectQueryData(query);
    if (status == ERROR_SUCCESS) {
        // Retrieve the total CPU usage value=
        status = PdhGetFormattedCounterValue(totalCpuCounter, PDH_FMT_DOUBLE, NULL, &value);
        if (status == ERROR_SUCCESS) {
            // Calculate the CPU usage for a specific process
            float totalCpuUsage = (float)value.doubleValue / numProcessors;

            // Calculate CPU usage for the process with the given processId
            // You can use a similar approach as in your displayCpuUsage() function
            // For example, add a counter for the specific process and calculate its CPU usage

            return totalCpuUsage;
        }
        else {
            printf("PdhGetFormattedCounterValue failed with status 0x%x\n", status);
        }
    }
    else {
        printf("PdhCollectQueryData failed with status 0x%x\n", status);
    }

    return 0.0;
}

void listProcessesWithCpuUsage() {
    DWORD processes[1024];
    DWORD needed;

    if (EnumProcesses(processes, sizeof(processes), &needed)) {
        int processCount = needed / sizeof(DWORD);
        printf("%-12s | %-35s | %-12s | %-18s\n", "Process ID", "Process Name", "CPU Usage (%)", "Memory Usage (MB)");

        for (int i = 0; i < processCount; i++) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
            if (hProcess) {
                char processName[MAX_PATH];

                if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) {
                    PROCESS_MEMORY_COUNTERS pmc;

                    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                        float cpuUsage = calculateCpuUsage(processes[i]);
                        printf("%-12u | %-35s | %-12.2f%% | %-18u MB\n", processes[i], processName, cpuUsage, (DWORD)(pmc.WorkingSetSize / (1024 * 1024)));
                    }
                    else {
                        printf("%-12u | %-35s | %-12s | %-18s\n", processes[i], processName, "N/A", "N/A");
                    }
                }

                CloseHandle(hProcess);
            }
        }
    }
}


int main() {
    initializeCpuUsage(); // Initialize CPU usage data
    listProcessesWithCpuUsage(); // List process information with CPU usage
    PdhCloseQuery(query); // Close the query
    return 0;
}


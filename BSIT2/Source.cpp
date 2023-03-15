#include <Windows.h>
#include <iostream>
#include <fstream>
#include "params.h"
#include "archive_manager.h"

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
char* servicePath;

template <typename T>
void log_message(T msg)
{
    std::ofstream log_file(LOG_PATH, std::ios_base::app);
    log_file << msg << std::endl;
    log_file.close();
}

int InstallService() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) {
        log_message("Error: Can't open Service Control Manager.");
        return -1;
    }

    SC_HANDLE hService = CreateService(
        hSCManager,
        SERVICE_NAME,
        SERVICE_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        servicePath,
        NULL, NULL, NULL, NULL, NULL
    );

    if (!hService) {
        int err = GetLastError();
        log_message(err);
        CloseServiceHandle(hSCManager);
        return -1;
    }
    CloseServiceHandle(hService);

    CloseServiceHandle(hSCManager);
    log_message("Success install service!");
    return 0;
}

int RemoveService() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) {
        log_message("Error: Can't open Service Control Manager");
        return -1;
    }
    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (!hService) {
        log_message("Error: Can't remove service");
        CloseServiceHandle(hSCManager);
        return -1;
    }
    
    DeleteService(hService);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    log_message("Success remove service!");
    return 0;
}

void StopService()
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) {
        log_message("Error: Can't open Service Control Manager");
        return ;
    }
    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (!hService) {
        log_message("Error: Can't remove service");
        CloseServiceHandle(hSCManager);
        return ;
    }

    ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);
}

int StartMyService() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_START);
    if (!StartService(hService, 0, NULL)) {
        CloseServiceHandle(hSCManager);
        log_message("Error: Can't start service");
        return -1;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return 0;
}


VOID WINAPI ControlHandler(DWORD request) {
	switch (request)
	{
	case SERVICE_CONTROL_STOP:
		log_message("Stopped.");

		ServiceStatus.dwWin32ExitCode = 0;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;

	case SERVICE_CONTROL_SHUTDOWN:
		log_message("Shutdown.");

		ServiceStatus.dwWin32ExitCode = 0;
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;
    case SERVICE_CONTROL_TRIGGEREVENT:
        log_message("TRIGGER");
        break;
	default:
        log_message("DEFAULT");
		break;
	}

	SetServiceStatus(hStatus, &ServiceStatus);

	return;
}

VOID WINAPI service_main(DWORD argc, LPSTR* argv)
{
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_TRIGGEREVENT;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;

	hStatus = RegisterServiceCtrlHandler(SERVICE_NAME, (LPHANDLER_FUNCTION)ControlHandler);
	if (hStatus == (SERVICE_STATUS_HANDLE)0) {
		return;
	}

	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hStatus, &ServiceStatus);


    log_message("Started");
    
    InitArchiveManager();
    while (true)
    {
        BackupFiles(GetFilesToBackup());
        Sleep(5000);
    }

	return;
}

int main(int argc, char *argv[])
{
    
    servicePath = LPTSTR(argv[0]);

    if (argc - 1 == 0) {
        SERVICE_TABLE_ENTRY ServiceTable[] = { {SERVICE_NAME, service_main}, {NULL, NULL} };

        if (!StartServiceCtrlDispatcher(ServiceTable)) {
            log_message("Error: StartServiceCtrlDispatcher");
        }
    }
    else if (strcmp(argv[argc - 1], "install") == 0) {
        InstallService();
    }
    else if (strcmp(argv[argc - 1], "remove") == 0) {
        RemoveService();
    }
    else if (strcmp(argv[argc - 1], "start") == 0) {
        StartMyService();
    }
    else if (strcmp(argv[argc - 1], "stop") == 0) {
        StopService();
    }

	return 0;
}

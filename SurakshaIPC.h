#pragma once

#include <windows.h>
#include <string>
#include <functional>
#include <atomic>
#include <thread>

#define SURAKSHA_PIPE_NAME L"\\\\.\\pipe\\SurakshaSecurePipe"

enum class IPCCommand {
    Ping = 1,
    LockAll,
    AddApp,
    RemoveApp,
    GetStatus,
    VerifyLogs
};

struct IPCMessage {
    IPCCommand cmd;
    wchar_t payload[512];
};

struct IPCResponse {
    bool success;
    wchar_t message[512];
};

class SurakshaIPCServer {
public:
    static SurakshaIPCServer& GetInstance() {
        static SurakshaIPCServer instance;
        return instance;
    }

    bool StartServer(std::function<IPCResponse(const IPCMessage&)> handler);
    void StopServer();

private:
    SurakshaIPCServer();
    ~SurakshaIPCServer();

    static DWORD WINAPI ServerThreadProc(LPVOID lpParam);
    void ServerLoop();

    std::atomic<bool> m_running;
    HANDLE m_hThread;
    std::function<IPCResponse(const IPCMessage&)> m_handler;
};


class SurakshaIPCClient {
public:
    static IPCResponse SendIPCCommand(IPCCommand cmd, const std::wstring& payload);
};

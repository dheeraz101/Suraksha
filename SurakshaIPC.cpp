#include "SurakshaIPC.h"
#include <exception>

SurakshaIPCServer::SurakshaIPCServer() : m_running(false), m_hThread(NULL) {}

SurakshaIPCServer::~SurakshaIPCServer() {
    StopServer();
}

DWORD WINAPI SurakshaIPCServer::ServerThreadProc(LPVOID lpParam) {
    SurakshaIPCServer* pThis = (SurakshaIPCServer*)lpParam;
    if (pThis) {
        pThis->ServerLoop();
    }
    return 0;
}

bool SurakshaIPCServer::StartServer(std::function<IPCResponse(const IPCMessage&)> handler) {
    if (m_running.load()) return true;

    m_handler = handler;
    m_running.store(true);

    if (m_hThread != NULL) {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    m_hThread = CreateThread(NULL, 0, ServerThreadProc, this, 0, NULL);
    return (m_hThread != NULL);
}

void SurakshaIPCServer::StopServer() {
    bool wasRunning = m_running.exchange(false);
    if (wasRunning) {
        HANDLE hPipe = CreateFileW(SURAKSHA_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
        }
    }
    if (m_hThread != NULL) {
        WaitForSingleObject(m_hThread, 500);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}


void SurakshaIPCServer::ServerLoop() {
    while (m_running.load()) {
        try {
            HANDLE hPipe = CreateNamedPipeW(
                SURAKSHA_PIPE_NAME,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                sizeof(IPCResponse),
                sizeof(IPCMessage),
                0,
                NULL
            );

            if (hPipe == INVALID_HANDLE_VALUE) {
                Sleep(100);
                continue;
            }

            BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

            if (connected && m_running.load()) {
                IPCMessage msg = { static_cast<IPCCommand>(0), {0} };
                DWORD cbRead = 0;
                if (ReadFile(hPipe, &msg, sizeof(msg), &cbRead, NULL) && cbRead > 0) {
                    IPCResponse resp = { false, L"No handler registered." };
                    if (m_handler) {
                        try {
                            resp = m_handler(msg);
                        } catch (...) {
                            resp.success = false;
                            wcsncpy_s(resp.message, L"IPC Handler Exception", 511);
                        }
                    }
                    DWORD cbWritten = 0;
                    WriteFile(hPipe, &resp, sizeof(resp), &cbWritten, NULL);
                }
            }

            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
        } catch (...) {
            Sleep(100);
        }
    }
}

IPCResponse SurakshaIPCClient::SendIPCCommand(IPCCommand cmd, const std::wstring& payload) {
    IPCResponse resp = { false, L"IPC Connection Failed" };

    try {
        HANDLE hPipe = CreateFileW(
            SURAKSHA_PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (hPipe != INVALID_HANDLE_VALUE) {
            DWORD dwMode = PIPE_READMODE_MESSAGE;
            SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);

            IPCMessage msg = { cmd, {0} };
            wcsncpy_s(msg.payload, payload.c_str(), 511);

            DWORD cbWritten = 0;
            if (WriteFile(hPipe, &msg, sizeof(msg), &cbWritten, NULL)) {
                DWORD cbRead = 0;
                ReadFile(hPipe, &resp, sizeof(resp), &cbRead, NULL);
            }
            CloseHandle(hPipe);
        }
    } catch (...) {
    }
    return resp;
}


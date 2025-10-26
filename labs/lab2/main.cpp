#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>  
#include <iostream>
#include <vector>
#include <cstring> 
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

#pragma pack(push, 1)
struct PciEntry {
    BYTE busNum;
    BYTE devNum;
    BYTE funcNum;
    WORD venID;
    WORD devID;
};
#pragma pack(pop)

typedef BOOL (__stdcall *FnInit)();
typedef void (__stdcall *FnShutdown)();
typedef BOOL (__stdcall *FnOutPort)(WORD, DWORD, BYTE);
typedef BOOL (__stdcall *FnInPort)(WORD, PDWORD, BYTE);

DWORD readPciConfig(FnOutPort outPort, FnInPort inPort,
                    BYTE bus, BYTE device, BYTE function, BYTE offset)
{
    DWORD cfgAddr = (1UL << 31) | ((DWORD)bus << 16)
                  | ((DWORD)device << 11) | ((DWORD)function << 8)
                  | (offset & 0xFC);
    if (!outPort(0xCF8, cfgAddr, 4))
        return 0xFFFFFFFF;

    DWORD cfgData = 0xFFFFFFFF;
    if (!inPort(0xCFC, &cfgData, 4))
        return 0xFFFFFFFF;

    return cfgData;
}

std::string createJsonFromVector(const std::vector<PciEntry>& data)
{
    std::string out;
    out.reserve(2048);
    out.push_back('[');

    for (size_t i = 0; i < data.size(); ++i) {
        if (i) out.push_back(',');

        char buf[128];
        sprintf_s(buf, sizeof(buf),
            "{\"bus\":%u,\"device\":%u,\"function\":%u,"
            "\"vendorID\":\"%04X\",\"deviceID\":\"%04X\"}",
            (unsigned int)data[i].busNum,
            (unsigned int)data[i].devNum,
            (unsigned int)data[i].funcNum,
            (unsigned int)data[i].venID,
            (unsigned int)data[i].devID);

        out += buf;
    }

    out.push_back(']');
    return out;
}

bool sendToServer(const char* ipAddr, int port, const std::vector<PciEntry>& data)
{
    std::cout << "Connecting to " << ipAddr << ":" << port << "...\n";
    
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return false;
    }

    int timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in srv = {};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(static_cast<u_short>(port));
    srv.sin_addr.s_addr = inet_addr(ipAddr);

    if (srv.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid IP address\n";
        closesocket(sock);
        WSACleanup();
        return false;
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&srv), sizeof(srv)) != 0) {
        std::cerr << "Connection failed, WSAGetLastError=" << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return false;
    }
    
    std::cout << "Connected successfully!\n";

    std::string json = createJsonFromVector(data);
    if (json.empty()) {
        std::cerr << "Nothing to send (empty JSON)\n";
        closesocket(sock);
        WSACleanup();
        return false;
    }

    const char* ptr = json.c_str();
    int bytesToSend = static_cast<int>(json.size());
    std::cout << "Sending JSON (" << bytesToSend << " bytes)...\n";

    int totalSent = 0;
    while (totalSent < bytesToSend) {
        int n = send(sock, ptr + totalSent, bytesToSend - totalSent, 0);
        if (n == SOCKET_ERROR) {
            int err = WSAGetLastError();
            std::cerr << "Send failed: " << err << "\n";
            closesocket(sock);
            WSACleanup();
            return false;
        }
        if (n == 0) {
            std::cerr << "Send returned 0 (peer closed?)\n";
            break;
        }
        totalSent += n;
    }

    std::cout << "Sent " << totalSent << " bytes\n";

    closesocket(sock);
    WSACleanup();
    return (totalSent == bytesToSend);
}

int main()
{
    std::cout << "PCI Scanner starting...\n";
    
    const char* dllPath = "C:\\WinIo\\Binaries\\WinIo32.dll";
    
    if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "ERROR: " << dllPath << " not found!\n";
        std::cerr << "Please create folder C:\\WinIo\\Binaries\\ and copy WinIo32.dll there\n";
        system("pause");
        return 1;
    }
    
    const char* sysPath = "C:\\WinIo\\Binaries\\WinIo32.sys";
    if (GetFileAttributesA(sysPath) == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "ERROR: " << sysPath << " not found!\n";
        std::cerr << "Please copy WinIo32.sys to C:\\WinIo\\Binaries\\\n";
        system("pause");
        return 1;
    }

    HMODULE hDll = LoadLibraryA(dllPath);
    if (!hDll) {
        DWORD err = GetLastError();
        std::cerr << "Failed to load WinIo32.dll. Error: " << err << "\n";
        system("pause");
        return 1;
    }
    
    std::cout << "WinIo32.dll loaded\n";

    FnInit initLib = (FnInit)GetProcAddress(hDll, "InitializeWinIo");
    FnShutdown shutdownLib = (FnShutdown)GetProcAddress(hDll, "ShutdownWinIo");
    FnInPort getPortVal = (FnInPort)GetProcAddress(hDll, "GetPortVal");
    FnOutPort setPortVal = (FnOutPort)GetProcAddress(hDll, "SetPortVal");

    if (!initLib || !shutdownLib || !getPortVal || !setPortVal) {
        std::cerr << "Failed to get WinIo functions\n";
        FreeLibrary(hDll);
        system("pause");
        return 1;
    }

    std::cout << "Initializing WinIo...\n";
    if (!initLib()) {
        std::cerr << "WinIo initialization failed!\n";
        std::cerr << "Make sure:\n";
        std::cerr << "1. You run as Administrator\n";
        std::cerr << "2. WinIo32.sys is in C:\\WinIo\\Binaries\\\n";
        std::cerr << "3. VMware IO settings are correct\n";
        FreeLibrary(hDll);
        system("pause");
        return 1;
    }
    
    std::cout << "WinIo initialized successfully\n";
    std::cout << "Scanning PCI devices...\n";

    std::vector<PciEntry> pciDevices;

	int counter = 0;
	int jj = 0;
	int kk = 0;
    for (int bus = 0; bus < 256; ++bus) {
		kk++;
		
        for (int dev = 0; dev < 32; ++dev) {
            DWORD baseData = readPciConfig(setPortVal, getPortVal, bus, dev, 0, 0);
			jj++;
            if (baseData == 0xFFFFFFFF)
                continue;

            for (int func = 0; func < 8; ++func) {
				counter++;
                DWORD cfgVal = readPciConfig(setPortVal, getPortVal, bus, dev, func, 0);
                WORD ven = (WORD)(cfgVal & 0xFFFF);
                WORD devID = (WORD)((cfgVal >> 16) & 0xFFFF);

                if (ven != 0xFFFF && ven != 0x0000) {
                    PciEntry entry;
                    entry.busNum = (BYTE)bus;
                    entry.devNum = (BYTE)dev;
                    entry.funcNum = (BYTE)func;
                    entry.venID = ven;
                    entry.devID = devID;
                    pciDevices.push_back(entry);
                    
                    std::cout << "Found device: Bus " << (int)bus 
                              << ", Dev " << (int)dev 
                              << ", Func " << (int)func
                              << ", VenID 0x" << std::hex << ven
                              << ", DevID 0x" << devID << std::dec << "\n";
                }
            }
        }
        
        if (bus % 16 == 0) {
            std::cout << "Scanned bus " << bus << " of 256...\n";
        }
    }

	std::cout << "\n!!!!!!!!!!!\n" << counter << std::endl << jj << std::endl << kk << std::endl;

    std::cout << "Found " << pciDevices.size() << " PCI devices\n";

    shutdownLib();
    FreeLibrary(hDll);

    if (sendToServer("10.217.9.232", 12345, pciDevices)) {
        std::cout << "Data sent successfully!\n";
    } else {
        std::cout << "Failed to send data\n";
    }

    system("pause");
    return 0;
}
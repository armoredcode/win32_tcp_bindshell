// https://docs.microsoft.com/en-us/windows/desktop/winsock/getting-started-with-winsock
// i686-w64-mingw32-gcc win32_basic_single_thread_tcp_server_stripped.c -l ws2_32 

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

  // Receive until the peer shuts down the connection
  do {

    iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0) {
      printf("Bytes received: %d\n", iResult);

      // Echo the buffer back to the sender
      iSendResult = send(ClientSocket, recvbuf, iResult, 0);
      if (iSendResult == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
      }
      printf("Bytes sent: %d\n", iSendResult);
    } else if (iResult == 0)
      printf("Connection closing...\n");
    else {
      printf("recv failed: %d\n", WSAGetLastError());
      closesocket(ClientSocket);
      WSACleanup();
      return 1;
    }

  } while (iResult > 0);
  // shutdown the send half of the connection since no more data will be sent
  shutdown(ClientSocket, SD_SEND);
  // cleanup
  closesocket(ClientSocket);
  WSACleanup();
  return 0;
}


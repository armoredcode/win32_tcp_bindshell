# Creating a BIND SHELL shellcode for Windows 32 bit

## Introduction
For the first SLAE certification assignment, [I was
asked](https://codiceinsicuro.it/slae/assignment-1-create-a-bind-shellcode/) to
create a TCP bind shell shellcode in assembly for Linux operating system.

Since all the payload I delivered, can't run on Windows operating system and
since I'm preparing to [Cracking the
Perimeter](https://www.offensive-security.com/information-security-training/cracking-the-perimeter/)
course, I started coding a win32 alternative.

The idea is to integrate such code in
[shellerate](https://github.com/thesp0nge/shellerate) project.

## My coding setup

I will use my Ubuntu Budgie virtual machine as coding platform. In order to
cross compile my C code for Windows platform, I installed the following
packages:

```
# apt install gcc-mingw-w64-i686 wine64
```

Please note that I choose the gcc extension for windows 32 bit. I don't have
any experience in assembly coding for 64bit processors (yet!), so I will skip
this now.

## A simple echo server

Let's start creating a simple TCP echo server using, Microsoft documentation
that it is very detailed in [explaining how to start developing a WinSock
application](https://docs.microsoft.com/en-us/windows/desktop/winsock/getting-started-with-winsock).

So I started with a very basic single thread TCP echo server accepting incoming
connections on port 4444.

The server structure is quite close to its Linux counterpart but I feel Windows
code is prolix. 

``` c
// https://docs.microsoft.com/en-us/windows/desktop/winsock/getting-started-with-winsock
// i686-w64-mingw32-gcc win32_basic_single_thread_tcp_server.c -l ws2_32 

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


  ZeroMemory(&hints, sizeof (hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed: %d\n", iResult);
    return 1;
  }

  // Resolve the local address and port to be used by the server
  iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
  if (iResult != 0) {
    printf("getaddrinfo failed: %d\n", iResult);
    WSACleanup();
    return 1;
  }
  ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

  if (ListenSocket == INVALID_SOCKET) {
    printf("Error at socket(): %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }

  // Setup the TCP listening socket
  iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    printf("bind failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  freeaddrinfo(result);

  if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
    printf( "Listen failed with error: %ld\n", WSAGetLastError() );
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  // Accept a client socket
  ClientSocket = accept(ListenSocket, NULL, NULL);
  if (ClientSocket == INVALID_SOCKET) {
    printf("accept failed: %d\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }
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
  iResult = shutdown(ClientSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed: %d\n", WSAGetLastError());
    closesocket(ClientSocket);
    WSACleanup();
    return 1;
  }
  // cleanup
  closesocket(ClientSocket);
  WSACleanup();
  return 0;
}
```

Now I can compile it and see the process waiting for connections on port 4444.

```
$ i686-w64-mingw32-gcc win32_basic_single_thread_tcp_server.c -l ws2_32 
```

As you can see [here](https://asciinema.org/a/220409), when executed the a.exe
program opens TCP port 4444 and stands for incoming connections.

A note for this pre-processor definition:

``` c
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
```

The purpose is to reduce the size of the Win32 header files by excluding some
of the less frequently used APIs. Looking at Windows.h header file we can see
that defining WIN32\_LEAN\_AND\_MEAN have a strong impact in not including a
lot of libraries our code won't use. More information available on [Windows dev
center](https://docs.microsoft.com/it-it/windows/desktop/WinProg/using-the-windows-headers).

## A stripped version of our echo server

Most of the code is for error checking so it can be easily removed to have a
more compact code.

``` c
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

```

## Let's move into a bind shell


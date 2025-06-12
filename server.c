// server.c

// Always include stdio.h first
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSESOCKET closesocket
#define THREAD_TYPE HANDLE
#define THREAD_RETURN DWORD WINAPI
#define THREAD_CREATE(thr, func, arg) thr = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL)
#define THREAD_DETACH(thr) CloseHandle(thr)
#define THREAD_EXIT() ExitThread(0)
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#define CLOSESOCKET close
#define THREAD_TYPE pthread_t
#define THREAD_RETURN void *
#define THREAD_CREATE(thr, func, arg) pthread_create(&thr, NULL, func, arg)
#define THREAD_DETACH(thr) pthread_detach(thr)
#define THREAD_EXIT() pthread_exit(NULL)
#endif

#define MAX_CLIENTS 100

#define BUFFER_SIZE 1024

#define NAME_LEN 32

struct client_info
{

    int sockfd;

    char name[NAME_LEN];
};

struct client_info clients[MAX_CLIENTS];

int client_count = 0;

#ifdef _WIN32
CRITICAL_SECTION clients_mutex;
#else
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void broadcast(char *message, int sender_sock)
{

#ifdef _WIN32
    EnterCriticalSection(&clients_mutex);
#else
    pthread_mutex_lock(&clients_mutex);
#endif

    for (int i = 0; i < client_count; ++i)
    {

        if (clients[i].sockfd != sender_sock)
        {

            send(clients[i].sockfd, message, strlen(message), 0);
        }
    }

#ifdef _WIN32
    LeaveCriticalSection(&clients_mutex);
#else
    pthread_mutex_unlock(&clients_mutex);
#endif
}

int find_client_by_name(const char *name)
{

    for (int i = 0; i < client_count; i++)
    {

        if (strcmp(clients[i].name, name) == 0)
        {

            return clients[i].sockfd;
        }
    }

    return -1;
}

void *handle_client(void *arg)
{

    int client_sock = *(int *)arg;

    char buffer[BUFFER_SIZE];

    char name[NAME_LEN];

    char full_msg[BUFFER_SIZE];

    int bytes;

    // Receive name

    if ((bytes = recv(client_sock, name, NAME_LEN, 0)) <= 0)
    {

        CLOSESOCKET(client_sock);

        THREAD_EXIT();
    }

    name[bytes - 1] = '\0'; // Remove newline if present

#ifdef _WIN32
    EnterCriticalSection(&clients_mutex);
#else
    pthread_mutex_lock(&clients_mutex);
#endif

    strcpy(clients[client_count].name, name);

    clients[client_count].sockfd = client_sock;

    client_count++;

#ifdef _WIN32
    LeaveCriticalSection(&clients_mutex);
#else
    pthread_mutex_unlock(&clients_mutex);
#endif

    // Send welcome message to this client only

    snprintf(buffer, sizeof(buffer), "Welcome to the chatroom, %s!\n", name);

    send(client_sock, buffer, strlen(buffer), 0);

    // Log to server

    printf("%s has joined the chatroom.\n", name);

    while ((bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {

        buffer[bytes] = '\0';

        if (strncmp(buffer, "/msg ", 5) == 0)
        {

            char target[NAME_LEN];

            char *msg_start;

            sscanf(buffer + 5, "%s", target);

            msg_start = strchr(buffer + 5, ' ');

            if (msg_start != NULL)
            {

                msg_start = strchr(msg_start + 1, ' ');

                if (msg_start != NULL)
                {

                    msg_start++;

                    int target_fd = find_client_by_name(target);

                    if (target_fd != -1)
                    {

                        snprintf(full_msg, sizeof(full_msg), "[Private from %s]: ", name);

                        size_t offset = strlen(full_msg);

                        strncat(full_msg, msg_start, sizeof(full_msg) - offset - 2);

                        strcat(full_msg, "\n");

                        send(target_fd, full_msg, strlen(full_msg), 0);
                    }
                    else
                    {

                        send(client_sock, "User not found.\n", 16, 0);
                    }
                }
            }
        }
        else
        {

            // Safe message composition to prevent truncation

            snprintf(full_msg, sizeof(full_msg), "%s: ", name);

            size_t offset = strlen(full_msg);

            strncat(full_msg, buffer, sizeof(full_msg) - offset - 2); // reserve space

            strcat(full_msg, "\n");

            broadcast(full_msg, client_sock);
        }
    }

    // Remove client on disconnect

#ifdef _WIN32
    EnterCriticalSection(&clients_mutex);
#else
    pthread_mutex_lock(&clients_mutex);
#endif

    for (int i = 0; i < client_count; i++)
    {

        if (clients[i].sockfd == client_sock)
        {

            for (int j = i; j < client_count - 1; j++)
            {

                clients[j] = clients[j + 1];
            }

            break;
        }
    }

    client_count--;

#ifdef _WIN32
    LeaveCriticalSection(&clients_mutex);
#else
    pthread_mutex_unlock(&clients_mutex);
#endif

    CLOSESOCKET(client_sock);

    printf("%s has left the chatroom.\n", name);

    THREAD_EXIT();
}

int main()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed.\n");
        return 1;
    }
    InitializeCriticalSection(&clients_mutex);
#endif

    int server_sock, client_sock;

    struct sockaddr_in server_addr, client_addr;

    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr = INADDR_ANY;

    server_addr.sin_port = htons(8080);

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(server_sock, MAX_CLIENTS);

    printf("Server started on port 8080...\n");

    while (1)
    {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        THREAD_TYPE tid;
        THREAD_CREATE(tid, handle_client, (void *)&client_sock);
        THREAD_DETACH(tid);
    }
#ifdef _WIN32
    DeleteCriticalSection(&clients_mutex);
    WSACleanup();
#endif
    return 0;
}
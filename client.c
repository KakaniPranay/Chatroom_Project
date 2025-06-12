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
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#define CLOSESOCKET close
#define THREAD_TYPE pthread_t
#define THREAD_RETURN void *
#endif

#define LENGTH 2048

// Cross-platform signal handling
typedef volatile int atomic_flag_t;
atomic_flag_t flag = 0;
int sockfd = 0;
char name[32];

void str_overwrite_stdout()
{
    printf("%s", "> ");
    fflush(stdout);
}

void str_trim_lf(char *arr, int length)
{
    for (int i = 0; i < length; i++)
    { // trim \n
        if (arr[i] == '\n')
        {
            arr[i] = '\0';
            break;
        }
    }
}

#ifdef _WIN32
BOOL WINAPI console_handler(DWORD signal)
{
    if (signal == CTRL_C_EVENT)
    {
        flag = 1;
    }
    return TRUE;
}
#else
void catch_ctrl_c_and_exit(int sig)
{
    flag = 1;
}
#endif

void send_msg_handler()
{
    char message[LENGTH] = {};
    while (1)
    {
        str_overwrite_stdout();
        fgets(message, LENGTH, stdin);
        str_trim_lf(message, LENGTH);

        if (strcmp(message, "/exit") == 0)
        {
            break;
        }
        else
        {
            send(sockfd, message, strlen(message), 0);
        }
        memset(message, 0, LENGTH);
    }
    // Removed call to catch_ctrl_c_and_exit(2); as flag is set by signal/console handler
}

void recv_msg_handler()
{
    char message[LENGTH] = {};
    while (1)
    {
        int receive = recv(sockfd, message, LENGTH, 0);
        if (receive > 0)
        {
            printf("%s\n", message);
            str_overwrite_stdout();
        }
        else if (receive == 0)
        {
            break;
        }
        else
        {
            // -1
        }
        memset(message, 0, sizeof(message));
    }
}

void connect_to_server(char *ip, int port)
{
    struct sockaddr_in server_addr;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed.\n");
        exit(EXIT_FAILURE);
    }
#endif
    /* Socket settings */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Connect to Server
    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1)
    {
        printf("ERROR: connect\n");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Send name
    send(sockfd, name, 32, 0);

    printf("=== WELCOME TO THE CHATROOM ===\n");

#ifdef _WIN32
    THREAD_TYPE send_msg_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)send_msg_handler, NULL, 0, NULL);
    THREAD_TYPE recv_msg_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)recv_msg_handler, NULL, 0, NULL);
#else
    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0)
    {
        printf("ERROR: pthread\n");
        exit(EXIT_FAILURE);
    }
    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0)
    {
        printf("ERROR: pthread\n");
        exit(EXIT_FAILURE);
    }
#endif

    while (1)
    {
        if (flag)
        {
            printf("\nBye\n");
            break;
        }
    }

    CLOSESOCKET(sockfd);
#ifdef _WIN32
    WSACleanup();
#endif
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

#ifdef _WIN32
    SetConsoleCtrlHandler(console_handler, TRUE);
#else
    signal(SIGINT, catch_ctrl_c_and_exit);
#endif

    while (1)
    {
        printf("Please enter your name: ");
        fgets(name, 32, stdin);
        str_trim_lf(name, strlen(name));

        if (strlen(name) > 32 || strlen(name) < 2)
        {
            printf("Name must be less than 30 and more than 2 characters.\n");
            continue;
        }

        flag = 0;
        connect_to_server(ip, port);

        // If flag is set, it means the user wants to exit and re-enter with a new nickname
        if (flag)
        {
            continue; // Go back to the beginning of the loop to re-enter the name
        }
        else
        {
            break; // Exit the loop if the user wants to leave the chat
        }
    }

    return EXIT_SUCCESS;
}

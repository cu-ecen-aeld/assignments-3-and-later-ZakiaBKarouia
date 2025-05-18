#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>


#define PORT 9000
#define BUFFER_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"

int server_fd = -1;
int client_fd = -1;
FILE *file = NULL;

void daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Parent exits
        exit(EXIT_SUCCESS);
    }

    // Child continues
    if (setsid() < 0) {
        syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Redirect standard file descriptors to /dev/null
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}


void cleanup_and_exit(int signo) {
    syslog(LOG_INFO, "Caught signal, exiting");

    if (client_fd != -1) {
        syslog(LOG_INFO, "close clientfd");
        close(client_fd);
        syslog(LOG_INFO, "close clientfd");
    }

    if (server_fd != -1) {
        syslog(LOG_INFO, "close serverfd");

        close(server_fd);
        syslog(LOG_INFO, "closed serverfd");

    }
    remove(FILE_PATH);

    closelog();
    exit(EXIT_SUCCESS);
}

void setup_signal_handlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cleanup_and_exit;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char *argv[]) {
    int is_daemon = 0;

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    char client_ip[INET_ADDRSTRLEN];

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        is_daemon = 1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);
    setup_signal_handlers();

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
        return -1;
    }

    // Allow socket reuse to avoid "address already in use"
    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    // Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    if (is_daemon) {
        daemonize();
    }

    if (listen(server_fd, 10) < 0) {
        syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    // Accept connections loop
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        file = fopen(FILE_PATH, "a+");
        if (!file) {
            syslog(LOG_ERR, "Failed to open file: %s", strerror(errno));
            close(client_fd);
            continue;
        }

        // Receive data until newline
        while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
            char *newline = memchr(buffer, '\n', bytes_received);
            fwrite(buffer, 1, bytes_received, file);
            fflush(file);

            if (newline != NULL) break;
        }

        fclose(file);
        file = fopen(FILE_PATH, "r");
        if (file) {
            while ((bytes_received = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                send(client_fd, buffer, bytes_received, 0);
            }
            fclose(file);
            file = NULL;
        }

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client_fd);
        client_fd = -1;
    }

    cleanup_and_exit(0);
    return 0;
}

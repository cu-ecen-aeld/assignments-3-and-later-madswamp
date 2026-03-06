#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define PORT "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define RECV_BUF_SIZE 1024
#define SEND_BUF_SIZE 1024

static volatile sig_atomic_t exit_requested = 0;

void signalHandler(int signo)
{
    (void)signo;
    exit_requested = 1;
    syslog(LOG_INFO, "Caught signal, exiting");
}

int setupSignalHandlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        syslog(LOG_ERR, "sigaction SIGINT failed");
        return -1;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "sigaction SIGTERM failed");
        return -1;
    }

    return 0;
}

int createDaemon(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "fork failed");
        return -1;
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() == -1) {
        syslog(LOG_ERR, "setsid failed");
        return -1;
    }

    if (chdir("/") == -1) {
        syslog(LOG_ERR, "chdir failed");
        return -1;
    }

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        syslog(LOG_ERR, "open /dev/null failed");
        return -1;
    }

    if (dup2(fd, STDIN_FILENO) == -1 ||
        dup2(fd, STDOUT_FILENO) == -1 ||
        dup2(fd, STDERR_FILENO) == -1) {
        syslog(LOG_ERR, "dup2 failed");
        close(fd);
        return -1;
    }

    if (fd > STDERR_FILENO) {
        close(fd);
    }

    return 0;
}

int bindStreamSocket(void)
{
    struct addrinfo hints;
    struct addrinfo *res, *p;
    int sockfd = -1;
    int status;
    int yes = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, PORT, &hints, &res);
    if (status != 0) 
    {
        syslog(LOG_ERR, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) 
        {
            close(sockfd);
            sockfd = -1;
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);

    if (p == NULL) 
    {
        syslog(LOG_ERR, "Failed to bind\n");
        return -1;
    }

    return sockfd;
}



int acceptConnection(int sockfd, char *client_ip, size_t ip_len)
{
    
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
    if (client == -1) {
        if (errno == EINTR) {
            return -1;
        }
        syslog(LOG_ERR, "accept error");
        return -1;
    }

    if (getnameinfo((struct sockaddr *)&client_addr, addr_len,
                    client_ip, ip_len,
                    NULL, 0, NI_NUMERICHOST) != 0) {
        strncpy(client_ip, "unknown", ip_len);
    }
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    return client;
}

int sendFileToClient(int client_fd)
{
    FILE *fp = fopen(DATA_FILE, "r");
    if (fp == NULL) {
        syslog(LOG_ERR, "fopen failed for %s", DATA_FILE);
        return -1;
    }

    char buffer[SEND_BUF_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        size_t total_sent = 0;

        while (total_sent < bytes_read) {
            ssize_t bytes_sent = send(client_fd,
                                      buffer + total_sent,
                                      bytes_read - total_sent,
                                      0);
            if (bytes_sent < 0) {
                syslog(LOG_ERR, "send failed");
                fclose(fp);
                return -1;
            }

            total_sent += bytes_sent;
        }
    }

    if (ferror(fp)) {
        syslog(LOG_ERR, "fread failed");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int receiveAndAppendPackets(int client_fd)
{
    char recv_buf[RECV_BUF_SIZE];
    char *packet_buf = NULL;
    size_t packet_size = 0;

    while (1) {
        ssize_t bytes_received = recv(client_fd, recv_buf, sizeof(recv_buf), 0);
        if (bytes_received < 0) {
            syslog(LOG_ERR, "recv failed");
            free(packet_buf);
            return -1;
        }

        if (bytes_received == 0) {
            break;
        }

        char *new_buf = realloc(packet_buf, packet_size + bytes_received + 1);
        if (new_buf == NULL) {
            syslog(LOG_ERR, "realloc failed");
            free(packet_buf);
            return -1;
        }

        packet_buf = new_buf;
        memcpy(packet_buf + packet_size, recv_buf, bytes_received);
        packet_size += (size_t)bytes_received;
        packet_buf[packet_size] = '\0';

        char *newline_pos;
        while ((newline_pos = strchr(packet_buf, '\n')) != NULL) {
            size_t packet_len = (size_t)(newline_pos - packet_buf) + 1;

            FILE *fp = fopen(DATA_FILE, "a");
            if (fp == NULL) {
                syslog(LOG_ERR, "fopen failed");
                free(packet_buf);
                return -1;
            }

            if (fwrite(packet_buf, 1, packet_len, fp) != packet_len) {
                syslog(LOG_ERR, "fwrite failed");
                fclose(fp);
                free(packet_buf);
                return -1;
            }

            fclose(fp);

            if (sendFileToClient(client_fd) != 0) {
                free(packet_buf);
                return -1;
            }

            size_t remaining = packet_size - packet_len;
            memmove(packet_buf, packet_buf + packet_len, remaining);
            packet_size = remaining;
            packet_buf[packet_size] = '\0';

            if (packet_size == 0) {
                free(packet_buf);
                packet_buf = NULL;
                break;
            }

            char *shrunk = realloc(packet_buf, packet_size + 1);
            if (shrunk != NULL) {
                packet_buf = shrunk;
            }
        }
    }

    free(packet_buf);
    return 0;
}



int main(int argc, char *argv[])
{
    bool daemon_mode = false;

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = true;
    } else if (argc > 1) {
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        return -1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);
    
    if (setupSignalHandlers() < 0) {
        return -1;
    }

    int sockfd = bindStreamSocket();
    if (sockfd < 0) 
    {
        return -1;
    }

    if (daemon_mode) {
        if (createDaemon() < 0) {
            return -1;
        }
    }

    if (listen(sockfd, 10) == -1) 
    {
        syslog(LOG_ERR, "listen error\n");
        return -1;
    }
     while (!exit_requested) {
        char client_ip[NI_MAXHOST];

        int client = acceptConnection(sockfd, client_ip, sizeof(client_ip));
        if (client < 0) {
            if (exit_requested) {
                break;
            }
            continue;
        }

        receiveAndAppendPackets(client);

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client);
    }  

    if (sockfd != -1) {
        close(sockfd);
    }

    unlink(DATA_FILE);
    closelog();
    return 0;
}
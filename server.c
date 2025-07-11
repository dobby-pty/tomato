// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8888
#define MAX_CLIENTS 100
#define RANK_FILE "rank.txt"

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_rank(const char *username) {
    pthread_mutex_lock(&file_mutex);
    FILE *file = fopen(RANK_FILE, "r+");
    char name[100];
    int count, found = 0;
    char lines[100][128];
    int n = 0;

    if (!file) {
        file = fopen(RANK_FILE, "w");
    }

    while (fscanf(file, "%s %d", name, &count) != EOF) {
        if (strcmp(name, username) == 0) {
            count++;
            found = 1;
        }
        sprintf(lines[n++], "%s %d\n", name, count);
    }

    if (!found) {
        sprintf(lines[n++], "%s 1\n", username);
    }

    freopen(RANK_FILE, "w", file);
    for (int i = 0; i < n; i++) {
        fputs(lines[i], file);
    }

    fclose(file);
    pthread_mutex_unlock(&file_mutex);
}

void send_rank(int sock) {
    pthread_mutex_lock(&file_mutex);
    FILE *file = fopen(RANK_FILE, "r");
    char buffer[1024] = "排行榜：\n";
    char line[128];

    if (file) {
        while (fgets(line, sizeof(line), file)) {
            strcat(buffer, line);
        }
        fclose(file);
    } else {
        strcat(buffer, "暂无记录。\n");
    }

    send(sock, buffer, strlen(buffer), 0);
    pthread_mutex_unlock(&file_mutex);
}

void *client_handler(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char username[100], buffer[1024];
    free(socket_desc);

    recv(sock, username, sizeof(username), 0);
    printf("用户已连接：%s\n", username);

    snprintf(buffer, sizeof(buffer), "欢迎你，%s！", username);
    send(sock, buffer, strlen(buffer), 0);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;

        if (strcmp(buffer, "DONE") == 0) {
            update_rank(username);
        } else if (strcmp(buffer, "RANK") == 0) {
            send_rank(sock);
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main() {
    int server_fd, client_sock, *new_sock;
    struct sockaddr_in server, client;
    socklen_t c = sizeof(client);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server, sizeof(server));
    listen(server_fd, MAX_CLIENTS);

    printf("服务器启动，等待客户端连接...\n");

    while ((client_sock = accept(server_fd, (struct sockaddr *)&client, &c))) {
        pthread_t tid;
        new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        pthread_create(&tid, NULL, client_handler, (void *)new_sock);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}

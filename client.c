// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

#define PORT 8888
#define SERVER_IP "127.0.0.1"

void show_menu() {
    printf("\n==== 番茄钟菜单 ====\n");
    printf("1. 倒计时模式\n");
    printf("2. 计时模式（按 Ctrl+C 停止）\n");
    printf("3. 多轮番茄计时\n");
    printf("4. 查看排行榜\n");
    printf("0. 退出程序\n");
    printf("请选择模式（0-4）：");
}

void countdown(int seconds, int sock) {
    while (seconds > 0) {
        printf("\r剩余时间：%02d 秒", seconds);
        fflush(stdout);
        sleep(1);
        seconds--;
    }
    printf("\n番茄计时结束！休息一下吧！\n");

    // 向服务器发送 DONE
    send(sock, "DONE", strlen("DONE"), 0);
}

void set_input_mode(int enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

void stopwatch() {
    int seconds = 0;
    printf("计时开始（按任意键结束）...\n");
    set_input_mode(1); // 启用非规范模式

    while (1) {
        printf("\r计时时间：%02d 秒", seconds++);
        fflush(stdout);

        fd_set fds;
        struct timeval tv = {1, 0}; // 1 秒超时
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
        if (ret > 0) {
            getchar(); // 清除缓冲区的键
            break;
        }
    }

    set_input_mode(0); // 恢复终端设置
    printf("\n计时结束！\n");
}

void multi_pomodoro(int rounds, int work_sec, int break_sec, int sock) {
    for (int i = 1; i <= rounds; ++i) {
        printf("\n第 %d 轮番茄计时开始！\n", i);
        countdown(work_sec, sock);
        if (i < rounds) {
            printf("开始休息！\n");
            countdown(break_sec, sock); // 休息也可以发送 DONE，或者跳过
        }
    }
    printf("多轮番茄计时完成！\n");
}

int main() {
    int sock;
    struct sockaddr_in server;
    char username[100], buffer[1024];

    // 创建 socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    // 输入用户名
    printf("请输入用户名：");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    send(sock, username, strlen(username), 0);

    // 接收欢迎信息
    memset(buffer, 0, sizeof(buffer));
    recv(sock, buffer, sizeof(buffer), 0);
    printf("收到服务器消息：%s\n", buffer);

    int choice;

    while (1) {
        show_menu();
        scanf("%d", &choice);

        if (choice == 1) {
            int sec;
            printf("请输入倒计时时长（秒）：");
            scanf("%d", &sec);
            printf("番茄计时开始！请专注 %d 秒。\n", sec);
            countdown(sec, sock);
        } else if (choice == 2) {
            stopwatch(); // Ctrl+C 停止
        } else if (choice == 3) {
            int rounds, work, rest;
            printf("请输入番茄计时轮数：");
            scanf("%d", &rounds);
            printf("每轮工作时长（秒）：");
            scanf("%d", &work);
            printf("每轮休息时长（秒）：");
            scanf("%d", &rest);
            multi_pomodoro(rounds, work, rest, sock);
        } else if (choice == 4) {
            send(sock, "RANK", strlen("RANK"), 0);
            memset(buffer, 0, sizeof(buffer));
            recv(sock, buffer, sizeof(buffer), 0);
            printf("\n==== 排行榜 ====\n%s\n", buffer);
        } else if (choice == 0) {
            printf("退出程序，感谢使用！\n");
            break; // 跳出循环
        } else {
            printf("无效选择。\n");
        }

        printf("\n按回车键返回菜单...\n");
        getchar(); // 吸收上一个回车
        getchar(); // 等待用户输入回车
    }

    close(sock);
    return 0;
}

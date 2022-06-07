/*
    使い方:
        サーバー側: ./discorb.out <port>
        クライアント: ./discorb.out <ip> <port>
*/

#include <arpa/inet.h>
#include <fft.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BUFSIZE 1024

int serverConnect(int port);
int clientConnect(char *ip, int port);
int call(int s);

int main(int argc, char **argv) {
    // connect
    int s = -1;
    if (argc == 2) {
        // server side
        // ./a.out <port>
        int port = atoi(argv[1]);
        s = serverConnect(port);
    } else if (argc == 3) {
        // client side
        // ./a.out <IP> <port>
        char *ip = argv[1];
        int port = atoi(argv[2]);
        s = clientConnect(ip, port);
    } else {
        fprintf(stderr, "usage: %s <ip> <port> or %s <port>\n", argv[0],
                argv[0]);
        exit(1);
    }
    call(s);
    close(s);
}

int serverConnect(int port) {
    // Prepare
    int ss = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(ss, (struct sockaddr *)&addr, sizeof(addr));

    // listen and accept incoming connections
    fprintf(stderr, "waiting for connections...\n");
    listen(ss, 10);
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int s = accept(ss, (struct sockaddr *)&client_addr, &len);
    close(ss);
    return s;
}

int clientConnect(char *ip, int port) {
    // connect & set up
    int s = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;  // IPv4のIPアドレスを使うことを明示
    inet_aton(ip, &addr.sin_addr);  // IPアドレスの指定
    if (addr.sin_addr.s_addr == 0) {
        // 不正なIPアドレス
        perror("Invalid IP");
        exit(EXIT_FAILURE);
    }
    addr.sin_port = htons(port);  // ポートの指定
    fprintf(stderr, "Successfully selected the IP address.\n");

    fprintf(stderr, "start connecting to IP address...\n");
    int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));  // connect
    if (ret == -1) {
        perror("connection");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Successfully connected to the IP adress.\n");
    return s;
}

int call(int s) {
    // parameters
    const double ratio = 0.5;

    // start recording
    FILE *fp = popen("rec -t raw -b 16 -c 1 -e s -r 44100 -", "r");

    // after the client has connected
    fprintf(stderr, "A client has connected to your port.\n");
    fprintf(stderr, "Input what you want to send.\n");
    short sendBuf[BUFSIZE];
    short recvBuf[BUFSIZE];
    complex double recvBeforeX[BUFSIZE];
    complex double recvBeforeY[BUFSIZE];
    complex double sendX[BUFSIZE];
    complex double sendY[BUFSIZE];
    for (;;) {
        // receive sound
        int recvNum = recv(s, recvBuf, sizeof(short) * BUFSIZE, 0);
        write(1, recvBuf, recvNum);

        // save pre-received data for noise cancellation
        sample_to_complex(recvBuf, recvBeforeX, BUFSIZE);
        fft(recvBeforeX, recvBeforeY, BUFSIZE);

        // send sound
        int sendNum = fread(sendBuf, sizeof(short), BUFSIZE, fp);
        if (sendNum == 0) break;
        if (sendNum < 0) {
            perror("send");
            return 1;
        }

        // noise calcelling
        sample_to_complex(sendBuf, sendX, BUFSIZE);
        fft(sendX, sendY, BUFSIZE);
        for (int i = 0; i < BUFSIZE; i++) sendY[i] -= ratio * recvBeforeY[i];
        ifft(sendY, sendX, BUFSIZE);
        complex_to_sample(sendX, sendBuf, BUFSIZE);

        // send data
        send(s, sendBuf, sendNum * sizeof(short), 0);
    }
    // fprintf(stderr, "Ended connection.\n");
    pclose(fp);
    return 0;
}
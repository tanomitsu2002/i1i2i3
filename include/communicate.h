#pragma once
#define BUFSIZE 1024
#define COMMAND_LEN 1024

#include <pthread.h>
#include <stdio.h>

#include "visualize.h"

typedef struct threads Threads;
typedef struct state State;
typedef struct callState CallState;

struct threads {
    pthread_t *callThread;
    pthread_t *sendChatThread;
    pthread_t *recvChatThread;
    pthread_t *stateThread;
};

/*
複数スレッドで共有される、現在の状態を保存するための構造体
*/
struct state {
    char myName[NAME_LEN];
    char cmd[COMMAND_LEN];
    ChatQueue *q;
    unsigned char isMeMuted; // ミュートするかどうか
    int scrolledUp;  // how many scrolled
    Threads threads;    // 終了処理するためにスレッドへのポインターを持っておく
    int curPos;  // chat cur's position
};

/*
相互通信して状態を共有する用の構造体
*/
struct callState {
    char stopProgram;
};

// sへのポインタを引数にとり、通話を行う
int call(void *arg);

int sendChat(void *arg);

int recvChat(void *arg);

int sendRcvState(void *arg);

typedef struct callProps {
    int s;
    char *stopProgram;
    pthread_mutex_t *mutex;
    State *state;
} CallProps;

typedef struct sendChatProps {
    int s;
    char *stopProgram;
    pthread_mutex_t *mutex;
    State *state;
} SendChatProps;

typedef struct recvChatProps {
    int s;
    char *stopProgram;
    pthread_mutex_t *mutex;
    State *state;
} RecvChatProps;

typedef struct sendRcvStateProps {
    int s;
    char *stopProgram;
    pthread_mutex_t *mutex;
    State *state;
} SendRcvStateProps;

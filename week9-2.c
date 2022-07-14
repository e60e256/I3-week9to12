#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <complex.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "fft2.h"
#include <time.h>
#define N 16384


// CMDIMPUTTE を追加する
// mode を追加する
// Git pull

struct CMDINPUTTE {
    int* s;
    int* dw;
    int* bytetotal;
    int* mode;
    unsigned char* data2;
    
};

struct UKEIREDATA {
    int data0;
    int data1;
    int data2;
    int port;
    int* sBp;
};

struct RECTOSENDDATA {
    int* s;
    int* sB;
    int* ds;
    int* datatotal;
    double* recamp;
    double* playamp;
    long* maxhz;
    long* minhz;
    unsigned char* data;
};

struct RECVTOPLAYDATA {
    int* s;
    int* sB;
    int* dw;
    int* bytetotal;
    int* mode;
    double* recamp;
    double* playamp;
    long* maxhz;
    long* minhz;
    int* isholding;
    unsigned char* data2;
    clock_t* start;
};

/*
void *ukeire(void *arg);

void *ukeire(void *arg){
    struct UKEIREDATA *ud = (struct UKEIREDATA*)arg;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int s = accept(ud->data0, (struct sockaddr*)&client_addr, &len);
    if (s == -1) {
        perror("Access denied\n");
        abort();
    }
    ud->data1 = s;
    pclose(ud->data2); 
    return NULL;
}
*/
void *hassinOn(void *arg){
    struct UKEIREDATA *has = (struct UKEIREDATA*)arg;
    
    pid_t pidd = fork();

    if (pidd < 0) {
        perror("hassinOn");
        exit(1);
    } else if (pidd == 0) {

        srand((unsigned int)time(NULL));
        int hasshinMusic = rand()%6;
        int exe=0;
        switch(hasshinMusic){
            case 0:
            exe = execlp("play", "play", "music1.wav", NULL); //ほん怖
            break;
            case 1:
            exe = execlp("play", "play", "music2.wav", NULL); //ハズレ
            break;
            case 2:
            exe = execlp("play", "play", "music3.wav", NULL); //オープニング
            break;
            case 3:
            exe = execlp("play", "play", "music4.wav", NULL); //感動曲
            break;
            case 4:
            exe = execlp("play", "play", "music5.wav", NULL); //ハズレ
            break;
            case 5:
            exe = execlp("play", "play", "music6.wav", NULL); //ハズレ
            break;
        }
        if (exe == -1) {
            perror("exec error");
            exit(1);
        }
    } else {
    fprintf(stderr,"%d\n",pidd);
    while(1){
    
     if(has->data2==1) {
            // fprintf(stderr,"こわいよ　%d\n",has->data2);
            int rc = kill(pidd, SIGTERM);
            if(rc < 0){
            fprintf(stderr, "Error: kill()(pid=%d)\n", pidd);
            perror("kill");
            exit(1);
            } else {
            fprintf(stderr, "kill()(pid=%d)\n", pidd);
            }
           
            // fprintf(stderr,"owari %d\n", has->data2);
            break;
            }
        }
    }
     return NULL;
}


void* recToSend(void* arg) {
        struct RECTOSENDDATA* has = (struct RECTOSENDDATA*)arg;
        FILE* rec = popen("rec -q -V1 -t raw -b 16 -c 1 -e s -r 44100 -", "r");
        sample_t * buf = calloc(sizeof(sample_t), N/2);
        
        int s2 = *(has->s);
        int ds;
        int datatotal = 0;
        unsigned char data[N];
        while (1) {
            // rec -> send parts
            ds = fread(data, 1, N, rec);
            datatotal += ds;
        if (ds == -1) {
            perror("Read error\n");
            exit(1);
        } else if (ds == 0) {
            break;
        }
        
        //fprintf(stderr, "%d\n", datatotal);
        mail(buf, data, ds/2, (long)(*(has->minhz)), (long)(*(has->maxhz)), (double)(*(has->recamp)));
        //fprintf(stderr, "%d, %d\n", ds, N);
        write(s2, data, ds);
        }
    pclose(rec);
    return NULL;
}

void* recvToPlay(void* arg) {
        struct RECVTOPLAYDATA *has = (struct RECVTOPLAYDATA*)arg;
        FILE* play = popen("play -q -V1 -t raw -b 16 -c 1 -e s -r 44100 -", "w");
        int s2 = *(has->s);
        unsigned char data2[N];
        int dw;
        while (1) {
        // recv -> play part
        // fprintf(stderr, "%d Bytes sent, %d in total\n", ds, datatotal);
        int byte = read(s2, data2, N/2);
        dw = fwrite(data2, 1, byte, play);
        // fprintf(stderr, "%d Bytes recv, %d in total\n", byte, bytetotal);
        if (dw <= 0) {
            break;                
        }
        *(has->dw) = dw;
        }

        pclose(play);
        return NULL;
}

// read commands and send all of them to the other party
void* commandInput(void* arg) {
    struct RECVTOPLAYDATA* has = (struct RECVTOPLAYDATA*)arg;
    // FILE* hold; // for holding
    char buf[30];
    while(1) {
        
        int n = read(0, buf, 30);      
        if (n != sizeof(buf)) {
            buf[n] = '\0';
        }
        if (strncmp(buf, "/mute", maxi(strlen(buf)-1, strlen("/mute")-1)) == 0) {
            fprintf(stderr, "Muted\n");
            *(has->recamp) = 0;
         } else if (strncmp(buf, "/quit", maxi(strlen(buf)-1, strlen("/quit")-1)) == 0) {
            fprintf(stderr, "Quitting\n");
            shutdown(*(has->s), SHUT_WR);
            FILE *fp_out = popen("play -q -V1 drun.wav", "w");
            pclose(fp_out);
            exit(1);
        } else if (strncmp(buf, "/time", maxi(strlen(buf)-1, strlen("/time")-1)) == 0) {
            clock_t ed = clock();
           // fprintf(stderr,"aiueo\n");
            fprintf(stderr,"%lf end  %lf cps\n",(double)ed, (double)CLOCKS_PER_SEC);
            int keika = (int)((double)(ed-*(has->start))/166666);
            fprintf(stderr, "Now, you are talking %d sec.\n",keika);
        }
        
         else if (strncmp(buf, "/unmute", maxi(strlen(buf)-1, strlen("/unmute")-1)) == 0) {
            *(has->recamp) = 1;
            fprintf(stderr, "Unmuted\n");
        } else if (strncmp(buf, "/recamp", strlen("/recamp")-1) == 0) {
            char cpytext[8];
            strncpy(cpytext, &buf[strlen("/recamp ")], 8);
            fprintf(stderr, "%s", cpytext);
            *(has->recamp) = atof(cpytext);
            fprintf(stderr, "Amp set to %lf\n", *(has->recamp));
        } else if (strncmp(buf, "/cut below", strlen("/cut below")-1) == 0) {
            char cpytext[8];
            strncpy(cpytext, &buf[strlen("/cut below ")], 8);
            fprintf(stderr, "%s", cpytext);
            *(has->minhz) = atol(cpytext);
            fprintf(stderr, "Cut below %ldHz\n", *(has->minhz));
        } else if (strncmp(buf, "/cut above", strlen("/cut above")-1) == 0) {
            char cpytext[8];
            strncpy(cpytext, &buf[strlen("/cut below ")], 8);
            fprintf(stderr, "%s", cpytext);
            *(has->maxhz) = atol(cpytext);
            fprintf(stderr, "Cut above %ldHz\n", *(has->maxhz));
        }else if (strncmp(buf, "/reset", maxi(strlen(buf)-1, strlen("/reset")-1)) == 0) {
            fprintf(stderr, "resetted\n");
            *(has->recamp) = 1;
            *(has->maxhz)=100000;
            *(has->minhz)=1;
             fprintf(stderr, "Resetted\n");
         } else if (strncmp(buf, "/holding", maxi(strlen(buf)-1, strlen("/holding")-1)) == 0) {
            fprintf(stderr, "On hold\n");
            // hold =  popen("play -q -V1 hold.wav", "w");
            *(has->recamp) = 0;
         } else if (strncmp(buf, "/unhold", maxi(strlen(buf)-1, strlen("/unhold")-1)) == 0) {
            *(has->recamp) = 1;
            fprintf(stderr, "Unholding\n");
            // pclose(hold);
         } else {
            fprintf(stderr, "%s\n", buf);
            continue;
        }
        // send to the other party
        write(*(has->sB), buf, n);
    }
    return NULL;
}

void* commandRecv(void* arg) {

    struct RECVTOPLAYDATA* has = (struct RECVTOPLAYDATA*)arg;
    char buf[30];
    static pid_t pidd;
    
    while(1) {
        int n = read(*(has->sB), buf, 30);      
        if (n != sizeof(buf)) {
            buf[n] = '\0';
        }
    fprintf(stderr, "%s\n", buf);
        if (strncmp(buf, "/holding", maxi(strlen(buf)-1, strlen("/holding")-1)) == 0 && *(has->isholding) == 0) {
            *(has->isholding) = 1;
            pidd = fork();
            if (pidd == 0) {
                int exe = execlp("play", "play", "music1.wav", "-q", "-V1", NULL); //ほん怖
                if (exe == -1) {
                    perror("execlp");
                    exit(1);
                }
            }
            fprintf(stderr, "On hold\n");
         } else if (strncmp(buf, "/unhold", maxi(strlen(buf)-1, strlen("/unhold")-1)) == 0 && *(has->isholding) == 1) {
            *(has->isholding) = 0;
            kill(pidd, SIGTERM);
            fprintf(stderr, "Unholding\n");
         }
    }
}

// Listening on 2nd port
void* sBaccept(void* arg) {
    struct UKEIREDATA* has = (struct UKEIREDATA*)arg;
    int ss = socket(PF_INET, SOCK_STREAM, 0);
    if (ss == -1) {
        perror("Socket creation failed\n");
        abort();
    }
    int port = has->port;
    struct sockaddr_in addr2; /* 最終的に bind に渡すアドレス情報 */
    addr2.sin_family = AF_INET; /* このアドレスはIPv4アドレスです */
    addr2.sin_port = htons(port+1); /* ポート...で待ち受けしたいです*/
    addr2.sin_addr.s_addr = INADDR_ANY; /* どのIPアドレスで待ち受けしたいです*/
    int erf = bind(ss, (struct sockaddr*)&addr2, sizeof(addr2));
    if (erf == -1) {
        perror("Binding error\n");
        abort();
    }

    erf = listen(ss, 10);
    if (erf == -1) {
        perror("Listening error\n");
        abort();
    }

   //FILE* po = popen("play music.wav", "r");

    struct sockaddr_in client_addr2;
    socklen_t len = sizeof(struct sockaddr_in);
    int sB = accept(ss, (struct sockaddr*)&client_addr2, &len);
    if (sB == -1) {
        perror("Access denied\n");
        abort();
    }
    *(has->sBp) = sB;
}



int main(int argc, char** argv) {
    pthread_t t[10];
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: ./serv_send2 <port-number>\n");
        exit(1);
    }
    if (argc == 2) {
    int port = atoi(argv[1]);

    int ss = socket(PF_INET, SOCK_STREAM, 0);
    if (ss == -1) {
        perror("Socket creation failed\n");
        abort();
    }

    struct sockaddr_in addr; /* 最終的に bind に渡すアドレス情報 */
    addr.sin_family = AF_INET; /* このアドレスはIPv4アドレスです */
    addr.sin_port = htons(port); /* ポート...で待ち受けしたいです*/
    addr.sin_addr.s_addr = INADDR_ANY; /* どのIPアドレスで待ち受けしたいです*/
    int erf = bind(ss, (struct sockaddr*)&addr, sizeof(addr));
    if (erf == -1) {
        perror("Binding error\n");
        abort();
    }

    erf = listen(ss, 10);
    if (erf == -1) {
        perror("Listening error\n");
        abort();
    }




    struct UKEIREDATA uk[10];
    uk[0].data2 = 0; //FILE*

    // for hassinOn
   pthread_create(&t[1],NULL,hassinOn,&uk[0]);
    // for 2nd thread
    int sB;
    uk[5].port = port;
    uk[5].sBp = &sB;
   pthread_create(&t[5], NULL, sBaccept, &uk[5]);
   //FILE* po = popen("play music.wav", "r");
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int s = accept(ss, (struct sockaddr*)&client_addr, &len);
    if (s == -1) {
        perror("Access denied\n");
        abort();
    }

    // Wait until 2nd servers' connection is established
    pthread_join(t[5], NULL);
    

    // client_addr の要素(sin_addr.s_addr, sin_port: 相手のアドレス、相手のポート)
    // 返り値: 新しいソケット（接続してきたクライアントとの送受信をするにはこのソケットを使う
    close(ss); // 古い奴は使わない

    // 第２接続

    uk[0].data2 =1;
    fprintf(stderr, "Connected!3\n");
    // client_addr の要素(sin_addr.s_addr, sin_port: 相手のアドレス、相手のポート)
    // 返り値: 新しいソケット（接続してきたクライアントとの送受信をするにはこのソケットを使う
    close(ss); // 古い奴は使わない


    //
    clock_t st;
    st = clock();
    
    unsigned char data[N];
    unsigned char data2[N];
    int ds = 0;
    int dw = 0;
    int datatotal = 0;
    int bytetotal = 0;
    double recamp = 1;
    double playamp = 1;
    long Max = 100000;
    long Min = 1;
    int isholding = 0;

    struct RECTOSENDDATA a;
    a.datatotal = &datatotal;
    a.ds = &ds;
    a.s = &s;
    a.recamp = &recamp;
    a.playamp = &playamp;
    a.data = data;
    a.maxhz = &Max;
    a.minhz = &Min;
    pthread_create(&t[2], NULL, recToSend, &a);

    struct RECVTOPLAYDATA b;
    b.dw = &dw;
    b.s = &s;
    b.data2 = data2;
    b.bytetotal = &bytetotal;
    pthread_create(&t[3], NULL, recvToPlay, &b);
    struct RECVTOPLAYDATA c;
    c.dw = &dw;
    c.s = &s;
    c.sB = &sB;
    c.recamp = &recamp;
    c.playamp = &playamp;
    c.maxhz = &Max;
    c.minhz = &Min;
    c.start =&st;
    pthread_create(&t[4], NULL, commandInput, &c);

    struct RECVTOPLAYDATA d;
    d.dw = &dw;
    d.s = &s;
    d.sB = &sB;
    d.recamp = &recamp;
    d.playamp = &playamp;
    d.maxhz = &Max;
    d.minhz = &Min;
    d.start =&st;
    d.isholding =&isholding;
    pthread_create(&t[6], NULL, commandRecv, &d);

    pthread_join(t[3], NULL);
    pthread_join(t[2], NULL);
    pthread_kill(t[4], SIGTERM);
    pthread_kill(t[6], SIGTERM);
    close(s);
    close(sB); // kill 2nd
    } else if (argc == 3) {
        int port = atoi(argv[2]);
        char* ipadd = argv[1];
        int s = socket(PF_INET, SOCK_STREAM, 0);
        if (s == -1) {
        perror("Socket creation failed\n");
        abort();
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        if (inet_aton(ipadd, &addr.sin_addr) == 0) {
        perror("Invalid IP address\n");
        abort();
        } 
        addr.sin_port = htons(port);
        int ret = connect(s, (struct sockaddr*)&addr, sizeof(addr));
        if (ret == -1) {
        perror("Connection failed\n");
        abort();
        }
        fprintf(stderr, "Connected!2\n");


        // 第２接続
        int sB = socket(PF_INET, SOCK_STREAM, 0);
        if (sB == -1) {
        perror("Socket creation failed\n");
        abort();
        }
        struct sockaddr_in addr2;
        addr2.sin_family = AF_INET;
        if (inet_aton(ipadd, &addr2.sin_addr) == 0) {
        perror("Invalid IP address\n");
        abort();
        } 
        addr2.sin_port = htons(port+1);
        ret = connect(sB, (struct sockaddr*)&addr2, sizeof(addr2));
        if (ret == -1) {
        perror("Connection failed\n");
        abort();
        }
        fprintf(stderr, "Connected!3\n");

  clock_t st;
    st = clock();
     
    unsigned char data[N];
    unsigned char data2[N];
    int ds = 0;
    int dw = 0;
    int datatotal = 0;
    int bytetotal = 0;
    double recamp = 1;
    double playamp = 1;
    long Max = 100000;
    long Min = 1;
    int isholding = 0;

    struct RECTOSENDDATA a;
    a.datatotal = &datatotal;
    a.ds = &ds;
    a.s = &s;
    a.recamp = &recamp;
    a.playamp = &playamp;
    a.data = data;
    a.maxhz = &Max;
    a.minhz = &Min;
    pthread_create(&t[2], NULL, recToSend, &a);

    struct RECVTOPLAYDATA b;
    b.dw = &dw;
    b.s = &s;
    b.data2 = data2;
    b.bytetotal = &bytetotal;
    pthread_create(&t[3], NULL, recvToPlay, &b);
    struct RECVTOPLAYDATA c;
    c.dw = &dw;
    c.s = &s;
    c.sB = &sB;
    c.recamp = &recamp;
    c.playamp = &playamp;
    c.maxhz = &Max;
    c.minhz = &Min;
    c.start =&st;
    pthread_create(&t[4], NULL, commandInput, &c);

    struct RECVTOPLAYDATA d;
    d.dw = &dw;
    d.s = &s;
    d.sB = &sB;
    d.recamp = &recamp;
    d.playamp = &playamp;
    d.maxhz = &Max;
    d.minhz = &Min;
    d.start =&st;
    d.isholding =&isholding;
    pthread_create(&t[6], NULL, commandRecv, &d);

    pthread_join(t[3], NULL);
    pthread_join(t[2], NULL);
    pthread_kill(t[4], SIGTERM);
    pthread_kill(t[6], SIGTERM);
    close(s);
    close(sB); // kill 2nd
    }

}

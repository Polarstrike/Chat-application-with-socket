#include <signal.h>
#include <time.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write


#define MAX_CONNECTION 30
#define BUFLEN 200
#define ONLINE 1
#define OFFLINE 0


void handler(int signum){
    pthread_exit(NULL);
}



    struct threadH threadBox[MAX_CONNECTION];
    
    


//TCP
    int tcp_sock, tcp_client, tcp_port;
    int c = sizeof(struct sockaddr_in);
    struct sockaddr_in server, client;

//ka
    int ka_sock, ka_client_sock;
    int ka_l = sizeof(struct sockaddr_in);
    struct  sockaddr_in ka_addr, ka_client_addr;

void* connection_handler(void* id);
void* keep_alive(void * id);
void regist(char* username, struct sockaddr_in tcp_addr);
int sendl(int sock, char* buf);
int recvl(int sock, char* buf);
int sendUdpl(int, struct sockaddr_in*, char*);
int recvUdpl(int, struct sockaddr_in*, char*);




int main(int argc , char **argv){
    if(argc != 2){
        printf("Argomenti non corretti, riprovare con:\n   PORTA_SERVER\n");
        return 0;
    }

    //PROCESS RELATED VARIABLES
    //int P;

    //GENERAL PURPOSE VARIABLES
    //int ret;


    tcp_port = atoi(argv[1]);
    

    //THREAD-SAFETY CONTROL INIT
    pthread_mutex_init(&M, NULL);
    pthread_cond_init(&OCCUPATO, NULL);

    pthread_mutex_init(&sndM, NULL);
    pthread_cond_init(&snd_OCCUPATO, NULL);

    pthread_mutex_init(&msgM, NULL);
    pthread_cond_init(&msg_OCCUPATO, NULL);

    signal(SIGUSR1, SIG_IGN);

    tcp_sock = socket(AF_INET , SOCK_STREAM , 0);   //Create socket tcp
    if (tcp_sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Listen -> Socket creato");
    ka_sock = socket(AF_INET , SOCK_STREAM , 0);   //Create socket tcp
    if (ka_sock == -1)
    {
        printf("ka_Could not create socket");
    }
    puts("Keep-Alive-Listen -> Socket creato");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( tcp_port );


    ka_addr.sin_family = AF_INET;
    ka_addr.sin_addr.s_addr = INADDR_ANY;
    ka_addr.sin_port = htons( tcp_port + 1);
     
    //Bind
    if( bind(tcp_sock,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("Listen -> Bind effettuata");
    if( bind(ka_sock,(struct sockaddr *)&ka_addr , sizeof(ka_addr)) < 0)
    {
        //print the error message
        perror("ka_bind failed. Error");
        return 1;
    }
    puts("Keep-Alive-Listen -> Bind effettuata");
     
    //Listen
    listen(tcp_sock , MAX_CONNECTION);

    listen(ka_sock , MAX_CONNECTION);
     
    //Accept and incoming connection
    printf("Server pronto: ");
    givemetime();
    printf("\nIl server è in ascolto (porta %d). Connessioni massime: %d\n", tcp_port, MAX_CONNECTION);
     
    while(1){ 
        int newThreadIndex;
        //accept connection from an incoming client (TCP)
        tcp_client = accept(tcp_sock, (struct sockaddr *)&client, (socklen_t*)&c);
        if (tcp_client < 0)
        {
            perror("accept failed");
            return 1;
        }
        givemetime();
        printf("Connessione in ingresso da %s -> Accettata\n", inet_ntoa(client.sin_addr));

        ka_client_sock = accept(ka_sock, (struct sockaddr *)&ka_client_sock, (socklen_t*)&ka_l);
        if (ka_client_sock < 0)
        {
            perror("ka_accept failed");
            return 1;
        }
        givemetime();
        printf("Keep-Alive -> Accettata\n");
        newThreadIndex = findFree(threadBox);

        

    

        //NEW THREAD 
        //int ka_port = client.sin_port;
        struct argm k;
        k.tcp = tcp_client;
        k.ka = ka_client_sock;
        k.pun = threadBox;
        k.index = newThreadIndex;
        pthread_create(&(threadBox[newThreadIndex].thread), NULL, connection_handler, &k);
        
       
    }
    
}


void* connection_handler(void * id){
    //GENERAL PURPOSE
    char me[BUFLEN];
    int registered = 0;
    //int read_size;
    char app[3000];

    struct argm* c = (struct argm*) id;
    int client = c->tcp;
    int ka_sock = c->ka;
    struct argk kk;
    kk.ka_sock = ka_sock;
    kk.str = me;
    kk.reg = &registered;
    kk.pun = c->pun;
    kk.index = c->index;



    signal(SIGUSR1, handler);
    pthread_create(&(c->pun[c->index].keep_alive), NULL, keep_alive, &kk);

    while(1){
        memset(app, 0, 3000);
        recv(client, app, 1, 0);


        if(strcmp(app, "r") == 0){
            //struct node elem;
            struct node* check;
            memset(app, 0, 3000);
            recvl(client, app);

            check = find(app);
            if(check != NULL){
                if(check->status == OFFLINE ){
                    check->status = ONLINE;
                    registered = 1;
                    sendl(client, "REG_ric");
                    givemetime();
                    printf("User <%s> riconnesso. ", app);
                    strcpy(me, app);
                    check->udp_socket.sin_family = AF_INET;
                    recvl(client, app);
                    inet_pton(AF_INET, app, &check->udp_socket.sin_addr.s_addr);

                    printf("UDP LISTEN -> %s:", app);

                    recvl(client, app);
                    check->udp_socket.sin_port = htons(atoi(app));
                    printf("%d\n", htons(atoi(app)));

                    struct senderNode* snd;
                    //struct msgNode* msg;
                    int nSender;
                    //int nMsg;
                    char nS[20];
                    snd = check->sndList;
                    nSender = sndlength(snd);
                    //printf("Nsender = %d\n", nSender);
                    sprintf(nS, "%d", nSender);
                    sendl(client, nS);      //invio numero liste di messaggi di vari mittenti
                    //printf("check1\n");
                    struct senderNode* sTmp = snd;
                    struct msgNode* mTmp;
                    while(sTmp != NULL){
                        sendl(client, sTmp->mittente);  //invio nome mittente
                        //printf("Mittente %s\n", sTmp->mittente);
                        mTmp = sTmp->msgList;
                        while(mTmp != NULL){
                            sendl(client, mTmp->msg);
                            //printf("   sent: %s\n", mTmp->msg);
                            fflush(stdout);
                            mTmp = mTmp->next;
                        }
                        msgDeleteList(sTmp->msgList);
                        sTmp->msgList = NULL;
                        sTmp = sTmp->next;
                    }
                    //printf("check2\n");
                    sndDeleteList(&check->sndList);
                    check->sndList = NULL;



                }
                else
                    sendl(client, "TAKEN");
            }
            else{
                sendl(client, "REG");
                givemetime();
                printf("User <%s> registrato. ", app);
                struct node *link = (struct node*) malloc(sizeof(struct node));
                link->username = (char*)malloc(sizeof(char)*strlen(app));
                strcpy(link->username, app);
                strcpy(me, app);
                link->status = ONLINE;
                link->udp_socket.sin_family = AF_INET;
                recvl(client, app);
                inet_pton(AF_INET, app, &link->udp_socket.sin_addr.s_addr);

                printf("UDP LISTEN -> %s:", app);

                recvl(client, app);
                link->udp_socket.sin_port = htons(atoi(app));
                printf("%d\n", htons(atoi(app)));

                insertFirst(link);
                registered = 1;
            }
        }


        //Deregistrazione 
        if(strcmp(app, "d") == 0){
            struct node* tmp;
            recvl(client, app);
            tmp = delete(app);
            deallocaNodo(tmp);  //dealloca tutto
            givemetime();
            printf("User <%s> si de-registrato\n", app);
            registered = 0;
        }





        //WHO 
        if(strcmp(app, "w") == 0){
            uint16_t lmsg;
            //char temp[BUFLEN];
            struct node* pun = head;
            givemetime();
            printf("User <%s> ha richiesto lista utenti\n", me);
            fflush(stdout);
            lmsg = htons(list_len);  
            send(client , (void*)&lmsg , sizeof(uint16_t), 0);  
            
            while(pun != NULL){
                
                sendl(client, pun->username);

                if(pun->status == OFFLINE)
                    sendl(client, "offline");
                else
                    sendl(client, "online");
                    
                pun = pun->next;
            }
        }


        //SEND (Black magic)
        if(strcmp(app, "s") == 0){
            //struct node* mittente;
            struct node* dest;
            char temp[BUFLEN];

            recvl(client, temp);    //destinatario finale
            dest = find(temp);
            if(dest == NULL){
                sendl(client, "no");
            }
            else if(dest->status == ONLINE){ //destinatario esiste! invio IP e PORT udp al client
                
                sendl(client, "yes_online");

                inet_ntop(AF_INET, &dest->udp_socket.sin_addr.s_addr, app, sizeof(app));
                sendl(client, app);

                sprintf(app,"%d", dest->udp_socket.sin_port);
                sendl(client, app);
                
                givemetime();
                printf("ISTANT MSG  <%s> -> <%s>\n", me, temp);
                fflush(stdout);
            }
            else{  //caso offline, salvo nel server i msg
                    sendl(client, "yes_offline");
                    recvl(client , app);    //ricevo mittente

                    /* dest punta al node */
                    
                    struct senderNode* sndMsg;

                    sndMsg = sndfind(dest->sndList, app);
                    if(sndMsg == NULL){
                        sndMsg = sndinsLast(&dest->sndList, app);         
                    }
                    else if(sndMsg->msgList != NULL){
                        msgDelete(sndMsg->msgList, ".");
                    }
                    struct msgNode* msgMsg;

                    givemetime();
                    printf("OFFLINE MSG memorizzato <%s> -> <%s>\n", me, temp);

                    //il destinatario finale lo trovo in temp[buflen] o dest->username
                    while(1){ 
                        recvl(client, app);
                        //printf("preso: %s\n", app);
                        msgMsg = msgInsLast(&sndMsg->msgList, app);
                        if(strcmp(app, ".")==0){    //s
                            sndMsg->lastMsg = msgMsg;
                            goto label;
                        }

                    }

            label: ;
                    
                    //sndprintList(dest->sndList);

            }

        }




        if(strcmp(app, "q") == 0){
            struct node* tmp;
            recvl(client, app);
            if(strcmp(app, "r")==0){
                recvl(client, app);
                tmp = find(app);
                tmp->status = OFFLINE;
                givemetime();
                printf("User <%s> disconnesso\n", app);
            }
            else{
                givemetime();
                printf("Client non registrato disconnesso\n");
            }
            close(client);
            Free_t(threadBox);        //ristabilisco struttura dati thread
            pthread_join((c->pun[c->index].keep_alive), NULL);
            pthread_exit(NULL);
        }


        if(strcmp(app, "t")==0){
            showThread(threadBox);
        }

    }
}



int sendl(int sock, char* buf){
    int len;
    uint16_t lmsg;
    len = strlen(buf);
    lmsg = htons(len);

    send(sock, (void*)&lmsg, sizeof(uint16_t), 0);  //INVIO DIMENSIONE

    len = send(sock, (void*)buf, len, 0);  //INVIO BUFFER
    return len;
}

int recvl(int sock, char* buf){
    int len, ret;
    uint16_t lmsg;

    memset(buf, 0, 3000);
    ret = recv(sock, (void*)&lmsg, sizeof(uint16_t), 0);
    len = ntohs(lmsg);
    ret = recv(sock, (void*)buf, len, 0);
    ret = ret;
    return len;
}


int sendUdpl(int sock, struct sockaddr_in* dest, char* buf){
    int len;
    uint16_t lmsg;
    len = strlen(buf);
    lmsg = htons(len);
    sendto(sock, (void*)&lmsg, sizeof(uint16_t), 0, (struct sockaddr*)&dest, sizeof(dest));  //INVIO DIMENSIONE
    len = sendto(sock, (void*)buf, len, 0, (struct sockaddr*)&dest, sizeof(dest));  //INVIO BUFFER
    return len;
}


int recvUdpl(int sock, struct sockaddr_in* sender, char* buf){
    int len, ret;
    int addrlen;
    uint16_t lmsg;

    memset(buf, 0, 3000);
    ret = recvfrom(sock, (void*)&lmsg, sizeof(uint16_t), 0, (struct sockaddr*)&sender, (socklen_t*)&addrlen);
    len = ntohs(lmsg);
    ret = recvfrom(sock, (void*)buf, len, 0, (struct sockaddr*)&sender, (socklen_t*)&addrlen);
    ret = ret;
    return len;
}


void* keep_alive(void * id){
    //int ka_sock = *(int*)id;
    struct argk* k = (struct argk*)id;
    int ka_sock = k->ka_sock;
    struct threadH* pun = k->pun;
    int index = k->index;
    char* str = k->str;
    int* reg;
    reg = k->reg;

    int ret = 10;
    char ka[10];
    char target[BUFLEN];
    while(ret != 0){
        memset(ka, 0, 10);
        send(ka_sock, "keep_alive", 10, 0);
        sleep(2);
        ret = recv(ka_sock, ka, 10, MSG_DONTWAIT);

        //debug printf
        /*
        if(ret != 0){
            printf("keep alive\n");
            fflush(stdout);
        }
        */

    }
    //printf("     FAULTY CLIENT!!!\n");
    fflush(stdout);

    
    if((*reg) == 1){
        strcpy(target, str);
        struct node* tmp = find(target);
        tmp->status = OFFLINE;
        givemetime();
        printf("Client %s crash: online -> offline\n", target);
    }
    else{
        givemetime();
        printf("Client non registrato crash: online -> offline\n");
    }
    // *(k->reg) = 0;
    
    close(ka_sock);
    Free_k(threadBox);
    pthread_kill(pun[index].thread, SIGUSR1);
    pthread_exit(NULL);

}
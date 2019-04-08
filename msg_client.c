#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
  
// sockaddr_in 16 BYTE
#define BUFLEN 2000
#define TCP 1
#define UDP 0
//colori
#define KNRM  "\x1B[0m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"

void* udp_wait(void*);
void* keep_alive(void * id);

//int sendUdpl(int, struct sockaddr_in*, char*);
void help();
void bashNewLine();
void sendLine(int socket);
int sendl(int, char*);
int recvl(int sock, char* buf);
void multiGetLine();
int sendUdpl(int, struct sockaddr_in, char*);
struct sockaddr_in recvUdpl(int, char*);


    //GENERAL PURPOSE VARIABLES    
    char buf[BUFLEN], command[BUFLEN];  
    int ret, line_len;
    unsigned int addrlen;

    //MSG FLOW CONTROL VARIABLES
    int registered = 0;
    int just_reg = 0;
    char auth_name[BUFLEN];
    char auth_backup[BUFLEN];

    //THREAD CONTROL
    pthread_t udp_daemon;
    pthread_t keep_alive_daemon;

 
int main(int argc , char **argv){
    if(argc != 5){
        printf("Argomenti non corretti, riprovare con:\n");
        printf("IP_UDP, PORTA_UDP, IP_SERVER, PORTA_SERVER\n");
        return 0;
    }
    //TCP
    int tcp_sock, tcp_port;
    struct sockaddr_in server;

    //UDP
    int udp_sock, udp_port;
    struct sockaddr_in peer, dest, incoming;


    int ka_sock;
    struct sockaddr_in ka_addr;

   


    tcp_port = atoi(argv[4]);
    udp_port = atoi(argv[2]);
    
    tcp_sock = socket(AF_INET , SOCK_STREAM , 0);   //Create socket tcp
    if (tcp_sock == -1)
    {
        printf("Could not create socket");
    }
    //puts("Socket created");
    

    server.sin_addr.s_addr = inet_addr(argv[3]);
    server.sin_family = AF_INET;
    server.sin_port = htons(tcp_port);


    peer.sin_addr.s_addr = inet_addr(argv[1]);
    peer.sin_family = AF_INET;
    peer.sin_port = htons(udp_port);

    ka_addr.sin_addr.s_addr = inet_addr(argv[3]);
    ka_addr.sin_family = AF_INET;
    ka_addr.sin_port = htons(tcp_port + 1);
    //printf("%d", tcp_port+1);


 
    //Connect to remote server
    if (connect(tcp_sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    printf("Connessione al server %s (porta %d) effettuata con successo\n", inet_ntoa(server.sin_addr), tcp_port);
    printf("Ricezione messaggi istantanei su porta %d\n", udp_port);


    ka_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ka_sock == -1){
        printf("Could not create socket");
    }
    //puts("Socket created");
    if(connect(ka_sock, (struct sockaddr*)&ka_addr, sizeof(ka_addr)) < 0){
        perror("connect failed. Error");
        return 1;
    }

    pthread_create(&keep_alive_daemon, NULL, keep_alive, &ka_sock);

    help();
    while(1){
        memset(command, 0, BUFLEN);
        memset(buf, 0, BUFLEN);
        bashNewLine();
        scanf("%s", command);

        //HELP
        if(strcmp(command, "!help")==0){
            help();
        }




        else if(strcmp(command, "!register")==0){
            
            if(registered==1){
                printf("Sei registrato come: %s\n", auth_name);
                scanf("%s", command);
            }
            else{
            
                send(tcp_sock, "r", 1, 0);
                scanf("%s", command);
                //printf("\n\n\n command: %s", command);
                //sendl(tcp_sock, command);
                sendl(tcp_sock, command);
                strcpy(auth_name, command);
                //sendLine(tcp_sock);

                recvl(tcp_sock, command);
                if(strcmp(command, "TAKEN")==0){
                    printf("Username in uso! Provare con un altro username\n");
                }
                else if(strcmp(command, "REG")==0){
                    inet_ntop(AF_INET, &peer.sin_addr.s_addr, command, sizeof(command));
                    sendl(tcp_sock, command);

                    sprintf(command,"%d", peer.sin_port);
                    sendl(tcp_sock, command);


                    printf("Registrazione avvenuta con successo\n");
                    registered = 1;
                    just_reg = 1;
                }
                else if(strcmp(command, "REG_ric")==0){
                    inet_ntop(AF_INET, &peer.sin_addr.s_addr, command, sizeof(command));
                    sendl(tcp_sock, command);

                    sprintf(command,"%d", peer.sin_port);
                    sendl(tcp_sock, command);


                    int nSender;
                    int i;
                    //int nMsg;

                    recvl(tcp_sock, command);
                    nSender = atoi(command);       //num liste messaggi di vari mittenti

                   // printf("Ci sono <%s , %d> mittenti\n", command, nSender);
                    for(i = 0; i < nSender; i++){
                        recvl(tcp_sock, buf);   //nome mittente
                        printf("%s (msg offline)>\n", buf);
                        while(strcmp(buf, ".")!=0){
                            recvl(tcp_sock, buf);
                            if(strcmp(buf, ".")!=0)
                                printf("%s\n", buf);

                        }
                        memset(buf, 0, BUFLEN);

                    }


                    printf("Riconnessione avvenuta con successo\n");
                    registered = 1;
                    just_reg = 1;
                }
            }
        }

        if(just_reg == 1){

            udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
          
            /*creazione indirizzo*/
            memset(&incoming, 0, sizeof(incoming));
            incoming.sin_family = AF_INET;
            incoming.sin_port = htons(udp_port);
            incoming.sin_addr.s_addr = INADDR_ANY;
            ret = bind(udp_sock, (struct sockaddr*)&incoming, sizeof(incoming));

            pthread_create(&udp_daemon, NULL, udp_wait, &udp_sock);
            
            just_reg = 0;
        }




        //DEREGISTER deregistra dal server 
        else if(strcmp(command, "!deregister")==0){
            if(registered == 0){
                printf("Non sei registrato.\n");
            }
            else{
                send(tcp_sock, "d", 1, 0);
                sendl(tcp_sock, auth_name);
                printf("Deregistrazione avvenuta con successo\n");
                registered = 0;
                just_reg = 0;
            }
        }




        //WHO  mostra i client registrati 
        else if(strcmp(command, "!who")==0){
            if(registered == 1){
                uint16_t lmsg;
                int len;    //nclient
                int i;
                int flag = 0;

                send(tcp_sock, "w", 1, 0);
                recv(tcp_sock , (void*)&lmsg, sizeof(uint16_t), 0);
                len = ntohs(lmsg);
                printf("Client registrati:\n");

                for(i = 0; i < len; i++){
                    flag = 0;
                    recvl(tcp_sock, buf);
                    if(strcmp(buf, auth_name)!=0){
                        printf("   %s ", buf);
                        flag = 1;
                    }

                    recvl(tcp_sock, buf);
                    
                    if(flag == 1){
                        if(strcmp(buf, "online")==0)
                            printf("(%s", KGRN);
                        else
                            printf("(%s", KYEL);
                        printf("%s", buf); 
                        printf("%s)\n", KNRM);   
                    }
                    
                }

                printf("\n");

            }     
            else{
                printf("Non sei registrato\n");
            }           
        }

        //thread check debug
        /*
        else if(strcmp(command, "t")==0)
            send(tcp_sock, "t", 1, 0);
        */


        else if(strcmp(command, "!send")==0){
            if(registered == 0){
                printf("Non sei registrato.\n");
                scanf("%s", command);
            }
            else{
                send(tcp_sock, "s", 1, 0);
                scanf("%s", command);

                sendl(tcp_sock, command);       //invio nome destinatario
                recvl(tcp_sock, buf);           //esiste?
                if(strcmp(buf, "no")==0){
                    printf("Impossibile connettersi a <%s>: utente inesistente.\n", command);
                }
                else if(strcmp(buf, "yes_online")==0){   //il destinatario esiste! sto per ricevere il suo IP UDP e PORTA UDP
                    
                    dest.sin_family = AF_INET;
                    recvl(tcp_sock, buf);
                    inet_pton(AF_INET, buf, &dest.sin_addr.s_addr);

                    //printf("%s UDP LISTEN -> %s:", command, buf); //debug

                    recvl(tcp_sock, buf);
                    dest.sin_port = htons(atoi(buf));
                    //printf("%d\n", atoi(buf));        //debug

                    //----raccolta messaggio---//

                    //printf("Message:\n"); //debug
                    
                    
                    struct node* tmp = head;
                    char* pun = malloc(1);
                    int once = 0;
                    int line_len;
                    fflush(stdin);

                    memset(buf, 0, BUFLEN);

                    while(1){
                        getline(&pun, (size_t*)&line_len, stdin);
                        line_len = strlen(pun);
                        pun[line_len-1] = '\0';
                        line_len--;
                        insLast(pun);
                        //printf("ho letto stringa lunga %d: <%s>\n",line_len, pun);  //debug
                        //printList();  //debug
                        if(strcmp(pun, ".")==0)
                            goto label;
                        memset(pun, 0, strlen(pun));
                        if(once == 0){
                            delete("");     //costo 1 tanto e` in testa
                            once=1;
                        }
                    }

    label: 
                    
                    sendUdpl(udp_sock, dest, auth_name);  //mio nome
                    sprintf(buf,"%d", length());
                    //printf("invio di %d linee\n", length());      //debug
                    sendUdpl(udp_sock, dest, buf);        //quante linee

                    //printList();                          //debug


                    tmp = head;
                    while(tmp!=NULL){

                        sendUdpl(udp_sock, dest, tmp->msg);
                        tmp = tmp->next;
                    }

                    deleteList();
                    printf("Messaggio istantaneo inviato.\n");
                    /*
                    printf("La lista ora:"); 
                    printList();      //debug
                    */
                }
                else{    //esiste ma e offline! invio i msg al server
                    char* pun = NULL;
                    int line_len;
                    
                    sendl(tcp_sock, auth_name); //invio mio nome
                    
                    while(1){
                        getline(&pun, (size_t*)&line_len, stdin);
                        line_len = strlen(pun);
                        pun[line_len-1] = '\0';
                        line_len--;
                        if(strcmp(pun, "")!=0)
                            sendl(tcp_sock, pun);
                        //printf("ho letto stringa lunga %d: <%s>\n",line_len, pun);  //debug
                        //printList();  //debug
                        if(strcmp(pun, ".")==0)
                            goto label1;
                        memset(pun, 0, strlen(pun));
                        
                    }
            label1:

                    printf("Messaggio offline inviato.\n");

                }

            }

        }





        //QUIT
        else if(strcmp(command, "!quit")==0){
            send(tcp_sock, "q", 1, 0);
            if(registered == 1){
                sendl(tcp_sock, "r");
                sendl(tcp_sock, auth_name);

            }
            else
                sendl(tcp_sock, "nr");

            close(tcp_sock);
            return 0;

        }

        // debug
        /*
        else if(strcmp(command, "var")==0){
            printf("registered: %d\n", registered);
            printf("auth_name: %s\n", auth_name);
        }
        */

    }

     
    
}





void bashNewLine(){
    if(registered==1)
        printf("%s", auth_name);
    
    printf(">");
}

void help(){
    printf("\nSono disponibili i seguenti comandi:\n");
    printf("!help --> mostra l'elenco dei comandi disponibili\n");
    printf("!register username --> registra il client presso il server\n");
    printf("!deregister --> de-registra il client presso il server\n");
    printf("!who --> mostra l'elenco degli utenti disponibili\n");
    printf("!send username --> invia un messaggio ad un altro utente\n");
    printf("!quit --> disconnette il client dal server ed esce\n\n");
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



void sendLine(int socket){
    char *pun = NULL;
    size_t line_len = 0;

    getline(&pun, &line_len, stdin);
    if(strcmp(pun, ".")!=0){        //TO BE CHECKEDDDDDD
        sendl(socket, pun+1);
    }    

    free(pun);
}




int sendUdpl(int sock, struct sockaddr_in dest, char* pun){
    int len;
    uint16_t lmsg;
    len = strlen(pun);
    lmsg = htons(len);
    sendto(sock, (void*)&lmsg, sizeof(uint16_t), 0, (struct sockaddr*)&dest, sizeof(dest));  //INVIO DIMENSIONE
    len = sendto(sock, (void*)pun, len, 0, (struct sockaddr*)&dest, sizeof(dest));  //INVIO BUFFER
    return len;
}


struct sockaddr_in recvUdpl(int sock, char* pun){
    int len, ret;
    int addrlen;
    struct sockaddr_in sender;
    uint16_t lmsg;

    ret = recvfrom(sock, (void*)&lmsg, sizeof(uint16_t), 0, (struct sockaddr*)&sender, (socklen_t*)&addrlen);
    len = ntohs(lmsg);
    ret = recvfrom(sock, (void*)pun, len, 0, (struct sockaddr*)&sender, (socklen_t*)&addrlen);
    ret = ret;
    return sender;
}


void* keep_alive(void * id){
    int sock = *(int*)id;
    char ka[10];

    //sendl(sock, auth_name);
    fflush(stdout);
    while(1){
        sleep(1);
        memset(ka, 0, 10);
        recv(sock, ka, 10, 0);
        //DEBUG PRINTF
            //printf("keep_alive\n");
            //fflush(stdout);
        send(sock, "keep_alive", 10, 0);
    }
}



void* udp_wait(void * id){
    int wait = *(int*)id;
    int line;
    int i;
    struct sockaddr_in arrive;
    char udp_buf[BUFLEN];

    fflush(stdout);
    while(1){


        recvUdpl(wait, udp_buf);        //mittente
        printf("\n%s (msg istantaneo)>\n", udp_buf);
        recvUdpl(wait, udp_buf);        //quante linee
        line = atoi(udp_buf);
        memset(udp_buf, 0, strlen(udp_buf));
        //printf("Sono in arrivo %d linee\n", line);    //DEBUG
        for(i = 0; i<line+1; i++){
            if(i!=0){
                recvUdpl(wait, udp_buf);
                if(i != line)
                    printf("%s\n", udp_buf);
                else
                    printf("%s>", auth_name);


                fflush(stdout);
                memset(&arrive, 0, sizeof(struct sockaddr_in));
                memset(udp_buf, 0, strlen(udp_buf));
            }
        }
        //recvfrom(wait, udp_buf, BUFLEN, 0, (struct sockaddr*)&arrive, &addrlen);

        
    }
    pthread_exit(NULL);

}

void multiGetLine(int mode){
    //struct msgNode* tmp = head;
    char* pun = malloc(1);
    int once = 0;
    int line_len;
    fflush(stdin);

    
    while(1){
        getline(&pun, (size_t*)&line_len, stdin);
        line_len = strlen(pun);
        pun[line_len-1] = '\0';
        line_len--;

        if(mode == UDP){
            insLast(pun);
            //printf("ho letto stringa lunga %d: <%s>\n",line_len, pun);  //debug
            //printList();  //debug
            memset(pun, 0, strlen(pun));
            if(once == 0){
                delete("");     //costo 1 tanto e` in testa
                once=1;
            }
        }
        else{

        }


        if(strcmp(pun, ".")==0)
            goto label;
    }

label:
    free(pun); 
}


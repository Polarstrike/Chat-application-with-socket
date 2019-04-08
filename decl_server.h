#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

#define ONLINE 1
#define OFFLINE 0

#define MAX_CONNECTION 30
#define BUSY 1
#define FREE 0

//---------------------------------time-----------------------------------------//

void givemetime(){
  time_t rawtime;
  struct tm * timeinfo;
  char btime [80];

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (btime,80,"[%d/%m/%y %H:%M:%S]",timeinfo);
  printf("%s  ", btime);
}


//---------------------------------THREAD STRUCT--------------------------------//

struct threadH{
    pthread_t thread;
    pthread_t keep_alive;
    int status;
};

int findFree(struct threadH* box){
    int i;
    for(i = 0; i < MAX_CONNECTION; i++){
        if(box[i].status == FREE){
            box[i].status = BUSY;
            return i;
        }
    }
    return MAX_CONNECTION;
}

void showThread(struct threadH* box){
    int i;
    for(i = 0; i < MAX_CONNECTION; i++){
        if(box[i].status == BUSY){
            printf("<%d> occupato\n", i);
        }
    }
    fflush(stdout);
    return;
}

void Free_t(struct threadH* box){
    int i;
    pthread_t self;
    self = pthread_self();
    for(i = 0; i < MAX_CONNECTION; i++){
        if((pthread_equal(self, box[i].thread)!=0) && box[i].status == BUSY){
            box[i].status = FREE;
            return;
        }
    }
}
void Free_k(struct threadH* box){
    int i;
    pthread_t self;
    self = pthread_self();
    for(i = 0; i < MAX_CONNECTION; i++){
        if((pthread_equal(self, box[i].keep_alive)!=0) && box[i].status == BUSY){
            box[i].status = FREE;
            return;
        }
    }
}


//---------------------------------CONNHANDL ARG---------------------------------//

struct chArg{
    int tcp_client;
    struct sockaddr_in ka_client;
    int index;
    struct threadH* pun;

};

//---------------------------------CLOSING---------------------------------------//

struct kaArg{
    int tcp_client;
    int new_sock;
    int index;
    struct threadH* pun;

};

struct argm{
  int tcp;
  int ka;
  char* str;
  int index;
  struct threadH* pun;
};

struct argk{
  int ka_sock;
  char* str;
  int* reg;
  struct threadH* pun;
  int index;
};




//-----------------------------------msgNODE-------------------------------------//




struct msgNode {
   char* msg;
   struct msgNode *next;
 };

int msg_list_len = 2;

//THREAD SUPPORT VARIABLES
    pthread_mutex_t msgM;
    pthread_cond_t msg_OCCUPATO; // condition variable 
    int msg_accessing = 0;

void msgllock(pthread_mutex_t M, pthread_cond_t C){
   pthread_mutex_lock(&M);
   while(msg_accessing == 1)
      pthread_cond_wait(&C, &M);
   msg_accessing = 1;
}

void msglunlock(pthread_mutex_t M, pthread_cond_t C){
   msg_accessing = 0;
   pthread_cond_broadcast(&C);
   pthread_mutex_unlock(&M);
}

//display the list
void msgPrintList(struct msgNode* head) {
   struct msgNode *ptr = head;

  msgllock(msgM, msg_OCCUPATO);

   printf("\n");
  
   //start from the beginning
   while(ptr != NULL) {
      printf("  ||  <%s>",ptr->msg);
      ptr = ptr->next;
   }
  
   printf("\nLEN= %d\n", msg_list_len-2);

  msglunlock(msgM, msg_OCCUPATO);
}


//insert link at the first location
void msgInsertFirst(struct msgNode** head, char* pun) {
   
   msgllock(msgM, msg_OCCUPATO);

   struct msgNode *link = (struct msgNode*) malloc(sizeof(struct msgNode));
   link->msg = (char*)malloc(sizeof(char)*strlen(pun));
   link->next = NULL;

   strcpy(link->msg, pun);
   
   link->next = *head;
   //point first to new first node
   *head = link;
   msg_list_len++;

   msglunlock(msgM, msg_OCCUPATO);
}


struct msgNode* msgInsLast(struct msgNode** head, char* pun) {
   struct msgNode* t;

  msgllock(msgM, msg_OCCUPATO);

         /// caso lista inizialmente vuota 
  if(*head==NULL) {
    *head=malloc(sizeof(struct msgNode));
    (*head)->msg= (char*)malloc(sizeof(char)*strlen(pun));
    strcpy((*head)->msg, pun);
    (*head)->next=NULL;
    t = *head;
    msglunlock(msgM, msg_OCCUPATO);
    return t; 
  }

         // caso lista con almeno un elemento 
  t = *head;

   // vado avanti fino alla fine della lista 
  while(t->next!=NULL)
    t=t->next;

   // qui t punta all'ultima struttura della lista: ne
   //creo una nuova e sposto il puntatore in avanti 
  t->next=malloc(sizeof(struct msgNode));
  t=t->next;

   // metto i valori nell'ultima struttura 
  t->msg= (char*)malloc(sizeof(char)*strlen(pun));
  strcpy(t->msg, pun);
  t->next=NULL;

  msg_list_len++;

  msglunlock(msgM, msg_OCCUPATO);
  return t;
}


//is list empty
bool msgIsEmpty(struct msgNode* head) {
   return head == NULL;
}

int msgLength() {

  msgllock(msgM, msg_OCCUPATO);
   return msg_list_len;
  msglunlock(msgM, msg_OCCUPATO);
}

//find a link with given key
struct msgNode* msgFind(struct msgNode* head, char* key) {


  msgllock(msgM, msg_OCCUPATO);
   //start from the first link
   struct msgNode* current = head;

   //if list is empty
   if(head == NULL) {
      
      msglunlock(msgM, msg_OCCUPATO);
      return NULL;

   }

   //navigate through list
   while(strcmp(current->msg,key)!=0) {
  
      //if it is last msgNode
      if(current->next == NULL) {

         msglunlock(msgM, msg_OCCUPATO);
         return NULL;
      } else {
         //go to next link
         current = current->next;
      }
   }      
  

   msglunlock(msgM, msg_OCCUPATO);
   //if data found, return the current Link
   return current;
}

//delete a link with given key
struct msgNode* msgDelete(struct msgNode* head, char* key) {

   //start from the first link
   struct msgNode* current = head;
   struct msgNode* previous = NULL;


  msgllock(msgM, msg_OCCUPATO);
  
   //if list is empty
   if(head == NULL) {

      msglunlock(msgM, msg_OCCUPATO);
      return NULL;
   }

   //navigate through list
   while(strcmp(current->msg,key)!=0) {

      //if it is last msgNode
      if(current->next == NULL) {

         msglunlock(msgM, msg_OCCUPATO);
         return NULL;
      } else {
         //store reference to current link
         previous = current;
         //move to next link
         current = current->next;
      }
   }

   //found a match, update the link
   if(current == head) {
      //change first to point to next link
      head = head->next;
   } else {
      //bypass the current link
      previous->next = current->next;
   }    
   msg_list_len--;

   msglunlock(msgM, msg_OCCUPATO);
   return current;
}


void msgDeallocaNodo(struct msgNode* elem){

   free(elem->msg);
   free(elem);
}

void msgDeleteList(struct msgNode* head){

   msglunlock(msgM, msg_OCCUPATO);
   struct msgNode* pun = head;
   while(head!=NULL){
      pun = head;
      head = head->next;
      msgDeallocaNodo(pun);
   }

   msg_list_len = 2;
   
      msglunlock(msgM, msg_OCCUPATO);
}


//----------------------------------sndNode---------------------------------------//


struct senderNode {
   char* mittente;
   struct msgNode* msgList;
   struct msgNode* lastMsg;
   struct senderNode *next;
 };

int snd_list_len = 2;

//THREAD SUPPORT VARIABLES
    pthread_mutex_t sndM;
    pthread_cond_t snd_OCCUPATO; // condition variable 
    int snd_accessing = 0;

void sndllock(pthread_mutex_t M, pthread_cond_t C){
   pthread_mutex_lock(&M);
   while(snd_accessing == 1)
      pthread_cond_wait(&C, &M);
   snd_accessing = 1;
}

void sndlunlock(pthread_mutex_t M, pthread_cond_t C){
   snd_accessing = 0;
   pthread_cond_broadcast(&C);
   pthread_mutex_unlock(&M);
}

//display the list
void sndprintList(struct senderNode* head) {
   struct senderNode *ptr = head;

  sndllock(sndM, snd_OCCUPATO);

   printf("\n");
  
   //start from the beginning
   while(ptr != NULL) {
      printf("%s:\n",ptr->mittente);
      msgPrintList(ptr->msgList);
      ptr = ptr->next;
   }
  
   printf("\nLEN= %d\n", snd_list_len-2);

  sndlunlock(sndM, snd_OCCUPATO);
}

struct senderNode* sndinsLast(struct senderNode **head, char* pun) {
   struct senderNode* t;

  sndllock(sndM, snd_OCCUPATO);

         /* caso lista inizialmente vuota */
  if((*head)==NULL) {
    (*head)=malloc(sizeof(struct senderNode));
    (*head)->mittente= (char*)malloc(sizeof(char)*strlen(pun));
    strcpy((*head)->mittente, pun);
    (*head)->next=NULL;
    (*head)->msgList = NULL;
    (*head)->lastMsg = NULL;
    snd_list_len++;
    t = (*head);
    sndlunlock(sndM, snd_OCCUPATO);
    return t;
  }

         /* caso lista con almeno un elemento */
  t = (*head);

   /* vado avanti fino alla fine della lista */
  while(t->next!=NULL)
    t=t->next;

   /* qui t punta all'ultima struttura della lista: ne
   creo una nuova e sposto il puntatore in avanti */
  t->next=malloc(sizeof(struct senderNode));
  t=t->next;

   /* metto i valori nell'ultima struttura */
  t->mittente= (char*)malloc(sizeof(char)*strlen(pun));
  strcpy(t->mittente, pun);
  t->next=NULL;
  t->msgList = NULL;

  snd_list_len++;

  sndlunlock(sndM, snd_OCCUPATO);
  return t;
}


//is list empty
bool sndisEmpty(struct senderNode* head) {
   return head == NULL;
}

int sndlength(struct senderNode* head) {
   int i = 0;
   struct senderNode* t = head;
  sndllock(sndM, snd_OCCUPATO);


   while(t!=NULL){
      i++;
      t=t->next;
   }
  sndlunlock(sndM, snd_OCCUPATO);

   return i;
}

//find a link with given key
struct senderNode* sndfind(struct senderNode* head, char* key) {


  sndllock(sndM, snd_OCCUPATO);
   //start from the first link
   struct senderNode* current = head;

   //if list is empty
   if(head == NULL) {
      
      sndlunlock(sndM, snd_OCCUPATO);
      return NULL;

   }

   //navigate through list
   while(strcmp(current->mittente,key)!=0) {
  
      //if it is last senderNode
      if(current->next == NULL) {

         sndlunlock(sndM, snd_OCCUPATO);
         return NULL;
      } else {
         //go to next link
         current = current->next;
      }
   }      
  

   sndlunlock(sndM, snd_OCCUPATO);
   //if data found, return the current Link
   return current;
}

//delete a link with given key
struct senderNode* snddelete(struct senderNode* head, char* key) {

   //start from the first link
   struct senderNode* current = head;
   struct senderNode* previous = NULL;


  sndllock(sndM, snd_OCCUPATO);
  
   //if list is empty
   if(head == NULL) {

      sndlunlock(sndM, snd_OCCUPATO);
      return NULL;
   }

   //navigate through list
   while(strcmp(current->mittente,key)!=0) {

      //if it is last senderNode
      if(current->next == NULL) {

         sndlunlock(sndM, snd_OCCUPATO);
         return NULL;
      } else {
         //store reference to current link
         previous = current;
         //move to next link
         current = current->next;
      }
   }

   //found a match, update the link
   if(current == head) {
      //change first to point to next link
      head = head->next;
   } else {
      //bypass the current link
      previous->next = current->next;
   }    
   snd_list_len--;

   sndlunlock(sndM, snd_OCCUPATO);
   return current;
}


void snddeallocaNodo(struct senderNode* elem){

   free(elem->mittente);
   free(elem);
}

void sndDeleteList(struct senderNode** head){

   sndlunlock(sndM, snd_OCCUPATO);
   struct senderNode* pun = (*head);
   while((*head)!=NULL){
      pun = (*head);
      pun->next = NULL;
      (*head) = (*head)->next;
      snddeallocaNodo(pun);
   }
   (*head) = NULL;
   snd_list_len = 2;
   
      sndlunlock(sndM, snd_OCCUPATO);
}





//------------------------------------NODE----------------------------------------//


struct node {
   char* username;
   int status;
   struct sockaddr_in tcp_socket;
   struct sockaddr_in udp_socket;
   struct senderNode* sndList;
   struct node *next;
 };

struct node *head = NULL;
struct node *current = NULL;
int list_len = 0;

//THREAD SUPPORT VARIABLES
    pthread_mutex_t M;
    pthread_cond_t OCCUPATO; // condition variable 
    int accessing = 0;

void llock(pthread_mutex_t M, pthread_cond_t C){
   pthread_mutex_lock(&M);
   while(accessing == 1)
   pthread_cond_wait(&C, &M);
   accessing = 1;
}

void lunlock(pthread_mutex_t M, pthread_cond_t C){
   accessing = 0;
   pthread_cond_broadcast(&OCCUPATO);
   pthread_mutex_unlock(&M);
}

//display the list
void printList() {
   llock(M, OCCUPATO);

   struct node *ptr = head;
   printf("\nClient iscritti: %d\n", list_len);
  
   //start from the beginning
   while(ptr != NULL) {
      printf("\t%s \t(",ptr->username);
      if(ptr->status == ONLINE)
         printf("online)\n");
      else
         printf("offline)\n");
      ptr = ptr->next;
   }
   printf("\n");

   lunlock(M, OCCUPATO);
}

//insert link at the first location
void insertFirst(struct node* link) {
   
   llock(M, OCCUPATO);

   //point it to old first node
   link->sndList = NULL; 
   link->next = head;
  
   //point first to new first node
   head = link;
   list_len++;

   lunlock(M, OCCUPATO);
}


//is list empty
bool isEmpty() {
   return head == NULL;
}

int length() {
   int length = 0;
   struct node *current;
  
   llock(M, OCCUPATO);

   for(current = head; current != NULL; current = current->next) {
      length++;
   }
  
   lunlock(M, OCCUPATO);

   return length;
}

//find a link with given key
struct node* find(char* key) {

   //start from the first link
   struct node* current = head;


   llock(M, OCCUPATO);

   //if list is empty
   if(head == NULL) {
      lunlock(M, OCCUPATO);
      return NULL;
   }

   //navigate through list
   while(strcmp(current->username,key)!=0) {
  
      //if it is last node
      if(current->next == NULL) {

         lunlock(M, OCCUPATO);
         return NULL;
      } else {
         //go to next link
         current = current->next;
      }
   }      
  

   lunlock(M, OCCUPATO);
   //if data found, return the current Link
   return current;
}

//delete a link with given key
struct node* delete(char* key) {

   //start from the first link
   struct node* current = head;
   struct node* previous = NULL;


   llock(M, OCCUPATO);
  
   //if list is empty
   if(head == NULL) {

      lunlock(M, OCCUPATO);
      return NULL;
   }

   //navigate through list
   while(strcmp(current->username,key)!=0) {

      //if it is last node
      if(current->next == NULL) {

         lunlock(M, OCCUPATO);
         return NULL;
      } else {
         //store reference to current link
         previous = current;
         //move to next link
         current = current->next;
      }
   }

   //found a match, update the link
   if(current == head) {
      //change first to point to next link
      head = head->next;
   } else {
      //bypass the current link
      previous->next = current->next;
   }    
   list_len--;


   lunlock(M, OCCUPATO);
   return current;
}

void deallocaNodo(struct node* elem){
   llock(M, OCCUPATO);

   free(elem->username);
   free(elem);

   lunlock(M, OCCUPATO);
}

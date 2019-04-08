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

struct keepAlive{
  int ip;
  int port;
};

//------------------------------------------------------------------------------------------------//

struct node {
   char* msg;
   struct node *next;
 };

struct node *head = NULL;
struct node *current = NULL;
int list_len = 2;

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
   struct node *ptr = head;

  llock(M, OCCUPATO);

   printf("\n");
  
   //start from the beginning
   while(ptr != NULL) {
      printf("  ||  <%s>",ptr->msg);
      ptr = ptr->next;
   }
  
   printf("\nLEN= %d\n", list_len-2);

  lunlock(M, OCCUPATO);
}

void insLast(char* pun) {
   struct node* t;

  llock(M, OCCUPATO);

         /* caso lista inizialmente vuota */
  if(head==NULL) {
    head=malloc(sizeof(struct node));
    head->msg= (char*)malloc(sizeof(char)*strlen(pun));
    strcpy(head->msg, pun);
    head->next=NULL;
    lunlock(M, OCCUPATO);
    return;
  }

         /* caso lista con almeno un elemento */
  t=head;

   /* vado avanti fino alla fine della lista */
  while(t->next!=NULL)
    t=t->next;

   /* qui t punta all'ultima struttura della lista: ne
   creo una nuova e sposto il puntatore in avanti */
  t->next=malloc(sizeof(struct node));
  t=t->next;

   /* metto i valori nell'ultima struttura */
  t->msg= (char*)malloc(sizeof(char)*strlen(pun));
  strcpy(t->msg, pun);
  t->next=NULL;

  list_len++;

  lunlock(M, OCCUPATO);
}


//is list empty
bool isEmpty() {
   return head == NULL;
}

int length() {

  llock(M, OCCUPATO);
   return list_len;

  lunlock(M, OCCUPATO);
}

//find a link with given key
struct node* find(char* key) {


  llock(M, OCCUPATO);
   //start from the first link
   struct node* current = head;

   //if list is empty
   if(head == NULL) {
      
      lunlock(M, OCCUPATO);
      return NULL;

   }

   //navigate through list
   while(strcmp(current->msg,key)!=0) {
  
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
   while(strcmp(current->msg,key)!=0) {

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

   free(elem->msg);
   free(elem);
}

void deleteList(){

   lunlock(M, OCCUPATO);
   struct node* pun = head;
   while(head!=NULL){
      pun = head;
      head = head->next;
      deallocaNodo(pun);
   }

   list_len = 2;
   
      lunlock(M, OCCUPATO);
}


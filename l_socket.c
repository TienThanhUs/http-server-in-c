#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
int global_var = 10;
int main() 
{
   int local_var = 20;
   int * heap_var = malloc(sizeof(int));
   *heap_var = 50;
   printf("global_var:%p\n",&global_var);   
   printf("local_var:%p\n",&local_var);   
   printf("heap var:%p\n",heap_var); 
   
}

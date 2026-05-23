#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main()
{
    char msg[1024] = {0};
    int c;
    int idx = 0; 
    while((c = getc(stdin)) != EOF)
    {
        msg[idx++] = c;
    }
    char password[20] = "nguyentienthanh";
    char temp[8] = "hcmus";
    printf("password before:%s\n", password);
    strcpy(temp,msg);
    printf("password after:%s\n",password);
}

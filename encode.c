#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

void encode(char text[]) {
    for (int i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            char base = (c >= 'A' && c <= 'Z') ? 'A' : 'a';
            text[i] = (c - base + 11) % 26 + base;
        }
    }
}

int main() {
    char text[100] ;
    gets(text,sizeof(text));
    encode(text);
    int f ;
    f = open("result.txt",O_CREATE|O_RDWR);
    write(f,text,strlen(text));
    close(f);
    return 0;
}

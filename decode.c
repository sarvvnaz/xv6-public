#include <stdio.h>

void decode(char text[]) {
    for (int i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            char base = (c >= 'A' && c <= 'Z') ? 'A' : 'a';
            text[i] = (c - base + 15) % 26 + base;
        }
    }
}

int main() {
    char text[100] ;
    gets(text);
    decode(text);
    FILE *f;
    f = fopen("result.txt" , "w" );
    fprintf(f,text);
    fclose(f);
    return 0;
}
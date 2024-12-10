#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct Lol
{
    int a;
    float b;
    char *s;
} Lol;

int main()
{
    printf("Hello\n");
    int offset = offsetof(Lol, b);
    int aa = sizeof(struct Lol *);
    printf("Offset: %i", aa);
}
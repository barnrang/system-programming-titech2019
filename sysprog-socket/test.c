#include <stdio.h>
#include <string.h>

int main() {
    char* s1 = "hello";
    char s2[] = "hello";
    if (strcmp(s1, s2) == 0) printf("hello");
    else printf("Not same");
    return 0;
}
#include <stdio.h>
#include <string.h>

void vulnerable(char *input) {
    char buffer[64];
    strcpy(buffer, input);  // ❌ sin límite de tamaño
}

int main(int argc, char *argv[]) {
    vulnerable(argv[1]);
    return 0;
}
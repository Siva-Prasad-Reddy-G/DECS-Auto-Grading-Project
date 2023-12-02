#include <stdio.h>

int main() {
    int *ptr = NULL; // Initialize a pointer to NULL
    
    // Attempting to dereference a NULL pointer
    int value = *ptr;
    
    printf("The value is: %d\n", value);
    
    return 0;
}


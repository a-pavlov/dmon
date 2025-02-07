#include <stdio.h>
#include <stdarg.h>

int __wrap_fprintf(FILE *stream, const char *format, ...) {
    FILE *file = fopen("clientmsg.txt", "a");

    if (file == NULL) {        
        return 0;
    }

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);
    
    fclose(file);
    return 0;
}
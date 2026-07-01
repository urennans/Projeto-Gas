/**
 * sysstubs.c — stubs mínimos para newlib em bare-metal
 * Necessário porque a libc do CubeIDE chama essas funções
 * mas elas não existem em ambiente sem OS.
 */

#include <sys/stat.h>
#include <stdint.h>

/* Heap definido pelo linker script do CubeIDE */
extern uint32_t _end;
extern uint32_t _estack;
extern uint32_t _Min_Stack_Size;

void* _sbrk(int incr)
{
    static uint8_t *heap_end = NULL;
    uint8_t *prev_heap_end;

    if (heap_end == NULL)
        heap_end = (uint8_t*)&_end;

    prev_heap_end = heap_end;

    /* Garante que o heap não invade a stack */
    if ((heap_end + incr) > ((uint8_t*)&_estack - (uint32_t)&_Min_Stack_Size))
        return (void*)-1;

    heap_end += incr;
    return (void*)prev_heap_end;
}

/* Stubs vazios — não usados em bare-metal */
int _close(int fd)        { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd)       { (void)fd; return 1; }
int _lseek(int fd, int ptr, int dir) { (void)fd; (void)ptr; (void)dir; return 0; }
int _read(int fd, char *ptr, int len) { (void)fd; (void)ptr; (void)len; return 0; }
int _write(int fd, char *ptr, int len) { (void)fd; (void)ptr; return len; }
int _getpid(void)         { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; return -1; }
void _exit(int status)    { (void)status; while(1) {} }

/* _init — chamado por __libc_init_array, pode ser vazio */
void _init(void) {}
#include <stdarg.h> /* va_list */
#include <stdio.h> /* printf */
#include <dlfcn.h> /* dladdr */
#include <stdlib.h> /* atexit, getenv */
// #include <string.h>

#define MAX_DEPTH 32
#define MAX_CALLS 4096

__attribute__((no_instrument_function))
int log_to_stderr(const char *file, int line, const char *func, const char *format, ...) {
    char message[4096];
    va_list va;
    va_start(va, format);
    vsprintf(message, format, va);
    va_end(va);
    return fprintf(stderr, "%s:%d(%s): %s\n", file, line, func, message);
}
#define LOG(...) log_to_stderr(__FILE__, __LINE__, __func__, __VA_ARGS__)

__attribute__((no_instrument_function))
const char *addr2name(void* address) {
    Dl_info dli;
    if (dladdr(address, &dli) != 0) {
        return dli.dli_sname;
    } else {
        return NULL;
    }
}

struct addr_list
{
    void *addr;
    struct addr_list* next_addr;
    struct addr_list* prev_addr;
};


FILE* fd_graph = NULL;

struct addr_list* list = NULL;
struct addr_list* current_addr = NULL;


__attribute__((no_instrument_function))
void __cyg_profile_func_enter(void *addr, void *call_site) {
    /* Not Yet Implemented */
    char* func_name = addr2name(addr);
    if (strcmp(func_name, "main")==0) {
        fd_graph = fopen("cg.dot","w+");
        fprintf(fd_graph, "strict digraph G {\n");
        list = (struct addr_list*) malloc(sizeof(struct addr_list));
        list->addr = addr;
        list->prev_addr = NULL;
        current_addr = list;
        // prev_addr = addr;
    } else {
        
        char* prev_name = addr2name(current_addr->addr);
        char* current_name = addr2name(addr);
        current_addr->next_addr = (struct addr_list*) malloc(sizeof(struct addr_list));
        current_addr->next_addr->addr = addr;
        current_addr->next_addr->prev_addr = current_addr;
        current_addr = current_addr->next_addr;
        fprintf(fd_graph, "\t%s -> %s\n", prev_name, current_name);
    }
    fprintf(stderr, ">>> %s (%p)\n", addr2name(addr), addr);
}

__attribute__((no_instrument_function))
void __cyg_profile_func_exit(void *addr, void *call_site) {
    /* Not Yet Implemented */
    char* func_name = addr2name(addr);
    if (strcmp(func_name, "main")==0) {
        fprintf(fd_graph, "}\n");
        fflush(fd_graph);
        free(list);
        fclose(fd_graph);
    } else {
        current_addr = current_addr->prev_addr;
        free(current_addr->next_addr);
    }
    fprintf(stderr, "<<< %s (%p)\n", addr2name(addr), addr);
}

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

FILE* fd_graph = NULL;

void* addr_stack[MAX_DEPTH];
void* HEAD[MAX_CALLS];
void* TAIL[MAX_CALLS];
int RECORD[MAX_CALLS];
int N = 0;
int M = 0;

__attribute__((no_instrument_function))
void print_out(){
    fd_graph = fopen("cg.dot","w+");
    fprintf(fd_graph, "strict digraph G {\n");
    void* head;
    void* tail;
    int weight = 0;
    // M =5;
    for (int i = 0; i < M; i++) RECORD[i] = 0;
    for (int i = 0; i < M; i++){
        if (RECORD[i]==1) continue;
        weight = 1;
        head = HEAD[i];
        tail = TAIL[i];
        if (i < 10) LOG("%d %p %s", i, TAIL[i], addr2name(TAIL[i]));
        for (int j = i+1; j < M; j++) {
            if((head == HEAD[j]) && (tail == TAIL[j])){
                RECORD[j] = 1;
                weight++;
            }
        }

        if (strcmp(getenv("SYSPROG_CG_LABEL"),"1") == 0){
            fprintf(fd_graph, "\t%s -> %s [label=\"%d\"]\n", addr2name(head)
            , addr2name(tail), weight);
        } else {
            fprintf(fd_graph, "\t%s -> %s \n", addr2name(head)
            , addr2name(tail));
        }
        
    }

    fprintf(fd_graph, "}\n");
    fflush(fd_graph);
    fclose(fd_graph);
}

__attribute__((no_instrument_function))
void __cyg_profile_func_enter(void *addr, void *call_site) {
    /* Not Yet Implemented */
    char* func_name = addr2name(addr);
    if (strcmp(func_name, "main")==0) {
        addr_stack[N] = addr;
        atexit(print_out);
    } else {
        // char* prev_name = addr2name(addr_stack[N]);
        // char* current_name = addr2name(addr);
        if (M < 10) LOG("%d %p %s\n", M, addr, addr2name(addr));
        HEAD[M] = addr_stack[N];
        TAIL[M] = addr;
        addr_stack[N+1] = addr;
        N++;
        M++;
        
        
        // fprintf(fd_graph, "\t%s -> %s\n", prev_name, current_name);
    }
    // fprintf(stderr, ">>> %s (%p)\n", addr2name(addr), addr);
}

__attribute__((no_instrument_function))
void __cyg_profile_func_exit(void *addr, void *call_site) {
    /* Not Yet Implemented */
    char* func_name = addr2name(addr);
    if (strcmp(func_name, "main")==0) {
        print_out();
    } else {
        N--;
    }
    // fprintf(stderr, "<<< %s (%p)\n", addr2name(addr), addr);
}

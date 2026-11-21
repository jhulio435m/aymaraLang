typedef struct {
    const char *type;
    const char *message;
    long long line;        /* linea del programa donde se creo la excepcion */
    const char *file;
} AymException;

typedef struct AymHandler {
    jmp_buf env;
    struct AymHandler *prev;
    AymException *exception;
} AymHandler;

static AymHandler *current_handler = NULL;

intptr_t aym_exception_new(const char *type, const char *message) {
    AymException *exc = (AymException*)calloc(1, sizeof(AymException));
    if (!exc) return 0;
    exc->type = type ? type : "Error";
    exc->message = message ? message : "";
    exc->line = aym_cur_line;
    exc->file = aym_cur_file;
    return (intptr_t)exc;
}

const char *aym_exception_type(intptr_t exc) {
    if (!exc) return "";
    return ((AymException*)exc)->type;
}

long long aym_exception_line(intptr_t exc) {
    if (!exc) return 0;
    return ((AymException*)exc)->line;
}

const char *aym_exception_message(intptr_t exc) {
    if (!exc) return "";
    return ((AymException*)exc)->message;
}

intptr_t aym_try_push(void) {
    AymHandler *handler = (AymHandler*)calloc(1, sizeof(AymHandler));
    if (!handler) return 0;
    handler->prev = current_handler;
    current_handler = handler;
    return (intptr_t)handler;
}

void aym_try_pop(intptr_t handler) {
    if (!handler) return;
    AymHandler *h = (AymHandler*)handler;
    if (current_handler == h) {
        current_handler = h->prev;
    }
    free(h);
}

int aym_try_enter(intptr_t handler) {
    if (!handler) return 0;
    AymHandler *h = (AymHandler*)handler;
    return setjmp(h->env);
}

intptr_t aym_try_get_exception(intptr_t handler) {
    if (!handler) return 0;
    AymHandler *h = (AymHandler*)handler;
    return (intptr_t)h->exception;
}

intptr_t aym_try_env(intptr_t handler) {
    if (!handler) return 0;
    AymHandler *h = (AymHandler*)handler;
    return (intptr_t)h->env;
}

void aym_throw(intptr_t exception) {
    if (!current_handler) {
        AymException *exc = (AymException *)exception;
        fflush(stdout);
        fprintf(stderr, "Excepcion no manejada [%s]: %s\n",
                (exc && exc->type && exc->type[0]) ? exc->type : "Error",
                (exc && exc->message) ? exc->message : "");
        if (exc && exc->line > 0) {
            fprintf(stderr, "  en %s, linea %lld\n",
                    (exc->file && exc->file[0]) ? exc->file : "programa", exc->line);
        }
        fflush(stderr);
        exit(1);
    }
    current_handler->exception = (AymException*)exception;
    longjmp(current_handler->env, 1);
}

void aym_div_zero(void) {
    fflush(stdout);
    aym_throw(aym_exception_new("ARITMETICA", "division entera por cero"));
}

/* Potencia entera base^exp por cuadrados (envoltura en 64 bits).
 * Exponente negativo: 1/base^n truncado (1 -> 1, -1 -> +-1, resto -> 0);
 * 0 elevado a negativo es division por cero. */
long long aym_ipow(long long base, long long exp) {
    if (exp < 0) {
        if (base == 0) aym_div_zero();
        if (base == 1) return 1;
        if (base == -1) return (exp & 1) ? -1 : 1;
        return 0;
    }
    unsigned long long result = 1;
    unsigned long long b = (unsigned long long)base;
    while (exp > 0) {
        if (exp & 1) result *= b;
        b *= b;
        exp >>= 1;
    }
    return (long long)result;
}

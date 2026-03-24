typedef struct {
    const char *type;
    const char *message;
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
    return (intptr_t)exc;
}

const char *aym_exception_type(intptr_t exc) {
    if (!exc) return "";
    return ((AymException*)exc)->type;
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
        fprintf(stderr, "Excepcion no manejada\n");
        exit(1);
    }
    current_handler->exception = (AymException*)exception;
    longjmp(current_handler->env, 1);
}

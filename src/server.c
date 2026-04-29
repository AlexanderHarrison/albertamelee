#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#define MG_TLS MG_TLS_OPENSSL
#include "vendor/mongoose.h"

typedef double F64;
typedef float F32;
typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;
typedef int64_t I64;
typedef int32_t I32;
typedef int16_t I16;
typedef int8_t I8;
typedef size_t USize;
typedef ssize_t ISize;
typedef struct mg_str Str;
#define countof(A) (sizeof(A) / sizeof(*(A)))
#define ConstStr(A) (Str) { (char *)A, sizeof(A)-1 }

void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        // mg_print_str("URI: ", hm->uri);
        if (mg_match(hm->method, mg_str("GET"), NULL)) {
            struct mg_http_serve_opts opts = { .root_dir = "web_root" };
            mg_http_serve_dir(c, hm, &opts);
        }
    }
    else if (ev == MG_EV_ERROR) {
        printf("Server error: %s\n", (char *) ev_data);
    }
    else if (ev == MG_EV_ACCEPT && c->is_tls) {
        struct mg_tls_opts opts = {
            .cert = mg_file_read(&mg_fs_posix, "/etc/letsencrypt/live/albertamelee/fullchain.pem"),
            .key = mg_file_read(&mg_fs_posix, "/etc/letsencrypt/live/albertamelee/privkey.pem"),
        };
        mg_tls_init(c, &opts);
    }
}

int main(void) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_log_set(MG_LL_ERROR);
    
    mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);
    mg_http_listen(&mgr, "http://0.0.0.0:80", ev_handler, NULL);
    mg_http_listen(&mgr, "https://0.0.0.0:443", ev_handler, NULL);
    
    while (true)
        mg_mgr_poll(&mgr, -1);
    mg_mgr_free(&mgr);

    return 0;
}


#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#define MG_TLS MG_TLS_OPENSSL
#include "vendor/mongoose.h"

static char template[1 << 20];
static char json[1 << 20];
static char output[1 << 20];
static char *output_cursor = output;

static void write_html(const char *ptr, size_t size) {
    memcpy(output_cursor, ptr, size);
    output_cursor += size;
}

static void write_str(struct mg_str s) {
    write_html(s.buf, s.len);
}

#define write_const(S) write_html(S, sizeof(S)-1)

static int sort_tournaments_by_date(const void *a, const void *b) {
    const struct mg_str *s0 = a;
    const struct mg_str *s1 = b;
    struct mg_str t0 = mg_json_get_tok(*s0, "$.startAt");
    struct mg_str t1 = mg_json_get_tok(*s1, "$.startAt");
    size_t len = t0.len < t1.len ? t0.len : t1.len;
    return strncmp(t0.buf, t1.buf, len);
}

#define Str(S) ((struct mg_str) { (char[]){S}, sizeof(S)-1 })

static void to_class(struct mg_str *s) {
    size_t in_i = 0;
    size_t out_i = 0;
    
    while (in_i != s->len) {
        char c = s->buf[in_i++];
        if ('a' <= c && c <= 'z') {}
        else if ('A' <= c && c <= 'Z') c += 'a' - 'A';
        else if (c == ' ') c = '-';
        else continue;
        s->buf[out_i++] = c;
    }
    s->len = out_i;
}

int main(void) {
    setenv("TZ", "America/Edmonton", 1);
    tzset();

    size_t template_size = fread(template, 1, sizeof(template), fopen("template.html", "rb"));
    size_t json_size = fread(json, 1, sizeof(json), fopen("ab_tournaments.json", "rb"));

    static const char template_str[] = "{AB_TOURNAMENTS}";
    unsigned template_replace_idx = 0;
    for (;; template_replace_idx++) {
        if (memcmp(template + template_replace_idx, template_str, sizeof(template_str)-1) == 0) {
            break;
        }
    }
    write_html(template, template_replace_idx);

    struct mg_str json_str = { json, json_size };

    struct mg_str errors = mg_json_get_tok(json_str, "$.errors");
    if (errors.len) {
        write_const("<div class=tournament-errors>");
            write_const("<div class=tournament-errors-preface>Please send the following error message to Aitch and tell him to fix his site!</div>");
            write_const("<div class=tournament-errors-message>");
                write_str(errors);
            write_const("</div>");
        write_const("</div>");
    }

    struct mg_str tournaments = mg_json_get_tok(json_str, "$.data.tournaments.nodes");
    struct mg_str t;
    size_t ofs = 0;

    // write replacement
    
    struct mg_str tournament_json[100];
    size_t tournament_count = 0;
    while ((ofs = mg_json_next(tournaments, ofs, NULL, &t)) > 0)
        tournament_json[tournament_count++] = t;

    // Sort tournaments by start date. Apparently start gg can't do this by itself, must be too complicated.
    qsort(tournament_json, tournament_count, sizeof(*tournament_json), sort_tournaments_by_date);

    for (size_t i = 0; i < tournament_count; ++i) {
        t = tournament_json[i];

        struct mg_str name = mg_json_get_tok(t, "$.name");
        struct mg_str url = mg_json_get_tok(t, "$.url");
        struct mg_str address = mg_json_get_tok(t, "$.venueAddress");
        struct mg_str start_time_unix_str = mg_json_get_tok(t, "$.startAt");
        struct mg_str region = mg_json_get_tok(t, "$.city");

        // remove quotes
        name.buf++; name.len -= 2;
        url.buf++; url.len -= 2;
        region.buf++; region.len -= 2;
        address.buf++; address.len -= 2;

        if (mg_strcasecmp(mg_json_get_tok(t, "$.isOnline"), Str("true")) == 0)
            region = Str("online");
        to_class(&region);

        // parse timestamp
        time_t start_time_unix = 0;
        while (start_time_unix_str.len) {
            time_t c = start_time_unix_str.buf[0];
            start_time_unix = start_time_unix * 10 + c - '0';
            start_time_unix_str.buf++;
            start_time_unix_str.len--;
        }
        
        write_const("<a class=\"tournament-card ");
        write_str(region);
        write_const("\" href=\"https://start.gg");
            write_str(url);
        write_const("\">");
            write_const("<div class=tournament-name>");
                write_str(name);
            write_const("</div>");
            write_const("<div class=tournament-date>");
                struct tm tm;
                localtime_r(&start_time_unix, &tm);

                char date[64];
                size_t date_len = strftime(date, sizeof(date), "%l:%M %P %A, %B %e", &tm);
                write_html(date, date_len);
            write_const("</div>");
            write_const("<div class=tournament-address>");
                write_str(address);
            write_const("</div>");
        write_const("</a>");
    }

    unsigned template_replace_idx_end = template_replace_idx + sizeof(template_str)-1;
    write_html(template + template_replace_idx_end, template_size - template_replace_idx_end);

    fwrite(output, 1, (size_t)(output_cursor - output), fopen("web_root/index.html", "wb+"));
}

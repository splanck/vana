/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with or without modification, are permitted provided that the copyright notice and this permission notice appear in all copies. This software is provided "as is" without warranty.
 *
 * Purpose: Implements the regex functions for vlibc. Provides wrappers and helpers used by the standard library.
 *
 * Copyright (c) 2025
 */

#include "regex.h"
#include "memory.h"
#include <string.h>
#include <ctype.h>

/*
 * This implementation is adapted from the public domain tiny-regex-c
 * project. It builds a very small NFA for the pattern and performs a
 * backtracking search. Originally only a subset of BRE/ERE syntax was
 * supported: '.', '*', '+', '?', '^', '$' and basic character classes.
 * This version adds simple alternation, grouping and repetition ranges
 * "{m,n}" along with POSIX character classes.
 */

struct token {
    unsigned char type;
    union {
        unsigned char ch;
        unsigned char *ccl;
        int group;
    } u;
};

/* token types */
enum {
    UNUSED, DOT, BEGIN, END, QUESTION, STAR, PLUS, CHAR, CHAR_CLASS, INV_CHAR_CLASS,
    DIGIT, NOT_DIGIT, ALPHA, NOT_ALPHA, SPACE, NOT_SPACE,
    CAP_OPEN, CAP_CLOSE, BACKREF
};

struct regex_alt {
    struct token *pat;
    unsigned char *ccl;
};

struct regex_impl {
    struct regex_alt *alts;
    size_t nalt;
};

/* helper prototypes */
static int matchpattern(struct token *pat, const char *text, int *matchlen,
                        const char *string, regmatch_t *caps);

/* Build the token list for a single pattern alternative. */
static int compile_single(struct regex_alt *alt, const char *pattern,
                          size_t *groups_out)
{
    size_t len = strlen(pattern);
    size_t patcap = len * 8 + 1;
    alt->pat = malloc(patcap * sizeof(struct token));
    if (!alt->pat)
        return -1;
    alt->ccl = malloc(len * 2 + 1);
    if (!alt->ccl) {
        free(alt->pat);
        alt->pat = NULL;
        return -1;
    }
    size_t ccl_idx = 1; /* first byte reserved */
    size_t i = 0, j = 0;
    int stack[10];
    int depth = 0;
    size_t groups = 0;
    while (pattern[i] && j + 1 < patcap - 1) {
        char c = pattern[i];
        switch (c) {
        case '^': alt->pat[j].type = BEGIN; break;
        case '$': alt->pat[j].type = END;   break;
        case '.': alt->pat[j].type = DOT;   break;
        case '*': alt->pat[j].type = STAR;  break;
        case '+': alt->pat[j].type = PLUS;  break;
        case '?': alt->pat[j].type = QUESTION; break;
        case '\\':
            if (pattern[i+1]) {
                i++;
                if (pattern[i] >= '1' && pattern[i] <= '9') {
                    alt->pat[j].type = BACKREF;
                    alt->pat[j].u.group = pattern[i] - '0';
                } else switch (pattern[i]) {
                case 'd': alt->pat[j].type = DIGIT; break;
                case 'D': alt->pat[j].type = NOT_DIGIT; break;
                case 'w': alt->pat[j].type = ALPHA; break;
                case 'W': alt->pat[j].type = NOT_ALPHA; break;
                case 's': alt->pat[j].type = SPACE; break;
                case 'S': alt->pat[j].type = NOT_SPACE; break;
                default:  alt->pat[j].type = CHAR; alt->pat[j].u.ch = pattern[i]; break;
                }
            }
            break;
        case '(': {
            if (groups < 9) {
                groups++; depth++; stack[depth-1] = groups;
                alt->pat[j].type = CAP_OPEN;
                alt->pat[j].u.group = groups;
            } else {
                alt->pat[j].type = CHAR; alt->pat[j].u.ch = c;
            }
            break; }
        case ')': {
            if (depth > 0) {
                int id = stack[--depth];
                alt->pat[j].type = CAP_CLOSE;
                alt->pat[j].u.group = id;
            } else {
                alt->pat[j].type = CHAR; alt->pat[j].u.ch = c;
            }
            break; }
        case '[': {
            size_t start = ccl_idx;
            if (pattern[i+1] == '^') { alt->pat[j].type = INV_CHAR_CLASS; i++; }
            else alt->pat[j].type = CHAR_CLASS;
            while (pattern[++i]) {
                if (pattern[i] == '\\' && pattern[i+1]) {
                    alt->ccl[ccl_idx++] = pattern[i++];
                    alt->ccl[ccl_idx++] = pattern[i];
                    continue;
                }
                if (pattern[i] == '[' && pattern[i+1] == ':') {
                    alt->ccl[ccl_idx++] = pattern[i++];
                    while (pattern[i] && !(pattern[i] == ':' && pattern[i+1] == ']'))
                        alt->ccl[ccl_idx++] = pattern[i++];
                    if (pattern[i]) {
                        alt->ccl[ccl_idx++] = pattern[i++];
                        alt->ccl[ccl_idx++] = pattern[i];
                    }
                    continue;
                }
                if (pattern[i] == ']')
                    break;
                alt->ccl[ccl_idx++] = pattern[i];
            }
            alt->ccl[ccl_idx++] = 0;
            alt->pat[j].u.ccl = &alt->ccl[start];
            break; }
        default:
            alt->pat[j].type = CHAR; alt->pat[j].u.ch = c; break;
        }
        i++; j++; if (ccl_idx + 1 >= len * 2) break;
    }
    alt->pat[j].type = UNUSED;
    if (groups_out) *groups_out = groups;
    return 0;
}

/* pattern expansion helpers */
struct strlist {
    char **items;
    size_t count;
    size_t cap;
};

/* Append a string to the growing list.
 * Returns 0 on success, -1 on allocation failure. */
static int list_add(struct strlist *l, char *s)
{
    if (l->count >= l->cap) {
        size_t nc = l->cap ? l->cap * 2 : 4;
        char **n = realloc(l->items, nc * sizeof(char *));
        if (!n)
            return -1;
        l->items = n;
        l->cap = nc;
    }
    l->items[l->count++] = s;
    return 0;
}

/* Free all strings in the list and reset it. */
static void list_free(struct strlist *l)
{
    for (size_t i = 0; i < l->count; i++)
        free(l->items[i]);
    free(l->items);
    l->items = NULL;
    l->count = l->cap = 0;
}

/* Test if character C belongs to the named POSIX class. */
static int match_posix_class(char c, const char *name, size_t len)
{
    if (len == 5 && strncmp(name, "alnum", 5) == 0) return isalnum((unsigned char)c);
    if (len == 5 && strncmp(name, "alpha", 5) == 0) return isalpha((unsigned char)c);
    if (len == 5 && strncmp(name, "digit", 5) == 0) return isdigit((unsigned char)c);
    if (len == 5 && strncmp(name, "lower", 5) == 0) return islower((unsigned char)c);
    if (len == 5 && strncmp(name, "upper", 5) == 0) return isupper((unsigned char)c);
    if (len == 5 && strncmp(name, "space", 5) == 0) return isspace((unsigned char)c);
    if (len == 6 && strncmp(name, "xdigit", 6) == 0) return isxdigit((unsigned char)c);
    if (len == 5 && strncmp(name, "print", 5) == 0) return isprint((unsigned char)c);
    if (len == 5 && strncmp(name, "graph", 5) == 0) return isgraph((unsigned char)c);
    if (len == 5 && strncmp(name, "cntrl", 5) == 0) return iscntrl((unsigned char)c);
    if (len == 5 && strncmp(name, "punct", 5) == 0) return ispunct((unsigned char)c);
    if (len == 5 && strncmp(name, "blank", 5) == 0) return c == ' ' || c == '\t';
    return 0;
}

/* Expand repetition ranges like {m,n} in PAT. Only positive numbers and
 * m <= n are supported. The returned string must be freed by the caller. */
static char *expand_ranges(const char *pat)
{
    size_t len = strlen(pat);
    size_t cap = len * 4 + 1; /* grow heuristic */
    char *out = malloc(cap);
    if (!out)
        return NULL;
    size_t o = 0;
    for (size_t i = 0; i < len; ) {
        size_t item_start = i;
        size_t item_end = i;
        if (pat[i] == '\\' && i + 1 < len) {
            item_end = i + 2;
        } else if (pat[i] == '[') {
            item_end = i + 1;
            while (item_end < len && pat[item_end] != ']') {
                if (pat[item_end] == '\\' && item_end + 1 < len)
                    item_end += 2;
                else
                    item_end++;
            }
            if (item_end < len)
                item_end++;
        } else if (pat[i] == '(') {
            int depth = 1;
            item_end = i + 1;
            while (item_end < len && depth > 0) {
                if (pat[item_end] == '\\' && item_end + 1 < len)
                    item_end += 2;
                else if (pat[item_end] == '(')
                    depth++, item_end++;
                else if (pat[item_end] == ')')
                    depth--, item_end++;
                else
                    item_end++;
            }
        } else {
            item_end = i + 1;
        }
        size_t item_len = item_end - item_start;
        i = item_end;
        int m = -1, n = -1;
        if (i < len && pat[i] == '{' && isdigit((unsigned char)pat[i+1])) {
            size_t p = i + 1;
            m = 0;
            while (p < len && isdigit((unsigned char)pat[p])) {
                m = m * 10 + (pat[p] - '0');
                p++;
            }
            if (p < len && pat[p] == ',') {
                p++;
                n = 0;
                while (p < len && isdigit((unsigned char)pat[p])) {
                    n = n * 10 + (pat[p] - '0');
                    p++;
                }
                if (p < len && pat[p] == '}') {
                    i = p + 1;
                } else {
                    m = -1; /* treat as literal */
                }
            } else {
                m = -1;
            }
        }

        /* ensure capacity */
        size_t need = item_len;
        if (m >= 0 && n >= m)
            need = item_len * (size_t)n + (n - m); /* for '?' */
        if (o + need + 1 >= cap) {
            cap = (cap + need + 1) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) {
                free(out);
                return NULL;
            }
            out = tmp;
        }

        if (m >= 0 && n >= m) {
            for (int k = 0; k < m; k++) {
                memcpy(out + o, pat + item_start, item_len);
                o += item_len;
            }
            for (int k = 0; k < n - m; k++) {
                memcpy(out + o, pat + item_start, item_len);
                o += item_len;
                out[o++] = '?';
            }
        } else {
            memcpy(out + o, pat + item_start, item_len);
            o += item_len;
        }
    }
    out[o] = '\0';
    return out;
}

/* Recursively expand alternation operators. The resulting list must be
 * freed with list_free(). */
/* Recursively expand alternation operators. The resulting list must be
 * freed with list_free(). Returns 0 on success, -1 on allocation failure. */
static int expand_alts(const char *pat, struct strlist *out)
{
    int level = 0, cls = 0;
    size_t start = 0, len = strlen(pat);
    struct strlist seg = {0};
    for (size_t i = 0; i < len; i++) {
        char c = pat[i];
        if (c == '\\' && i + 1 < len) { i++; continue; }
        if (cls) { if (c == ']') cls = 0; continue; }
        if (c == '[') { cls = 1; continue; }
        if (c == '(') { level++; continue; }
        if (c == ')' && level > 0) { level--; continue; }
        if (c == '|' && level == 0) {
            char *s = strndup(pat + start, i - start);
            if (!s || list_add(&seg, s) != 0) {
                free(s);
                list_free(&seg);
                return -1;
            }
            start = i + 1;
        }
    }
    char *s = strndup(pat + start, len - start);
    if (!s || list_add(&seg, s) != 0) {
        free(s);
        list_free(&seg);
        return -1;
    }

    if (seg.count > 1) {
        for (size_t i = 0; i < seg.count; i++) {
            if (expand_alts(seg.items[i], out) != 0) {
                list_free(&seg);
                return -1;
            }
        }
        list_free(&seg);
        return 0;
    }

    /* no top-level alternation: process groups */
    struct strlist cur = {0};
    if (list_add(&cur, strdup("")) != 0)
        return -1;
    for (size_t i = 0; i < len; ) {
        if (pat[i] == '\\' && i + 1 < len) {
            for (size_t j = 0; j < cur.count; j++) {
                size_t l = strlen(cur.items[j]);
                cur.items[j] = realloc(cur.items[j], l + 3);
                cur.items[j][l] = pat[i];
                cur.items[j][l+1] = pat[i+1];
                cur.items[j][l+2] = '\0';
            }
            i += 2;
        } else if (pat[i] == '[') {
            size_t j = i + 1;
            int cls2 = 1;
            while (j < len && cls2) {
                if (pat[j] == '\\' && j + 1 < len) j += 2;
                else if (pat[j] == ']') cls2 = 0, j++;
                else j++;
            }
            size_t ilen = j - i;
            for (size_t k = 0; k < cur.count; k++) {
                size_t l = strlen(cur.items[k]);
                cur.items[k] = realloc(cur.items[k], l + ilen + 1);
                memcpy(cur.items[k] + l, pat + i, ilen);
                cur.items[k][l+ilen] = '\0';
            }
            i = j;
        } else if (pat[i] == '(') {
            size_t j = i + 1; int depth = 1;
            while (j < len && depth) {
                if (pat[j] == '\\' && j + 1 < len) j += 2;
                else if (pat[j] == '(') { depth++; j++; }
                else if (pat[j] == ')') { depth--; j++; }
                else j++;
            }
            char *inside = strndup(pat + i + 1, j - i - 2);
            if (!inside) { list_free(&cur); list_free(&seg); return -1; }
            struct strlist sub = {0};
            if (expand_alts(inside, &sub) != 0) {
                free(inside);
                list_free(&cur);
                list_free(&seg);
                return -1;
            }
            free(inside);
            struct strlist newl = {0};
            for (size_t k = 0; k < cur.count; k++) {
                for (size_t m = 0; m < sub.count; m++) {
                    size_t l = strlen(cur.items[k]);
                    size_t ilen = strlen(sub.items[m]);
                    char *ns = malloc(l + ilen + 3);
                    memcpy(ns, cur.items[k], l);
                    ns[l] = '(';
                    memcpy(ns + l + 1, sub.items[m], ilen);
                    ns[l + ilen + 1] = ')';
                    ns[l + ilen + 2] = '\0';
                    if (list_add(&newl, ns) != 0) {
                        free(ns);
                        list_free(&newl);
                        list_free(&sub);
                        list_free(&cur);
                        list_free(&seg);
                        return -1;
                    }
                }
                free(cur.items[k]);
            }
            free(cur.items);
            cur = newl;
            list_free(&sub);
            i = j;
        } else {
            for (size_t k = 0; k < cur.count; k++) {
                size_t l = strlen(cur.items[k]);
                cur.items[k] = realloc(cur.items[k], l + 2);
                cur.items[k][l] = pat[i];
                cur.items[k][l+1] = '\0';
            }
            i++;
        }
    }
    for (size_t i = 0; i < cur.count; i++)
        if (list_add(out, cur.items[i]) != 0) {
            free(cur.items[i]);
            list_free(&cur);
            list_free(&seg);
            return -1;
        }
    free(cur.items);
    list_free(&seg);
    return 0;
}

/* Basic character category checks used by the matcher. */
static int matchdigit(char c) { return isdigit((unsigned char)c); }
static int matchalpha(char c) { return isalnum((unsigned char)c) || c == '_'; }
static int matchspace(char c) { return isspace((unsigned char)c); }

/* Test if C falls within the range described by STR. */
static int matchrange(char c, const char *str)
{
    return (c != '-') && str[0] && str[1] == '-' && str[2] &&
           c >= str[0] && c <= str[2];
}

/* Match any character except newline unless configured otherwise. */
static int matchdot(char c)
{
#ifdef RE_DOT_MATCHES_NEWLINE
    (void)c; return 1;
#else
    return c != '\n' && c != '\r';
#endif
}

/* Return non-zero if C denotes a shorthand meta escape. */
static int ismeta(char c)
{
    return c=='s' || c=='S' || c=='w' || c=='W' || c=='d' || c=='D';
}

/* Match character classes like \d or \w. */
static int matchmeta(char c, const char *str)
{
    switch (str[0]) {
    case 'd': return  matchdigit(c);
    case 'D': return !matchdigit(c);
    case 'w': return  matchalpha(c);
    case 'W': return !matchalpha(c);
    case 's': return  matchspace(c);
    case 'S': return !matchspace(c);
    default:  return c == str[0];
    }
}

/* Evaluate character classes, including POSIX names. */
static int matchclass(char c, const char *str)
{
    do {
        if (str[0] == '[' && str[1] == ':') {
            const char *end = strstr(str + 2, ":]");
            if (end) {
                if (match_posix_class(c, str + 2, (size_t)(end - (str + 2))))
                    return 1;
                str = end + 2;
                continue;
            }
        }
        if (matchrange(c, str))
            return 1;
        else if (str[0] == '\\') {
            str++;
            if (matchmeta(c, str))
                return 1;
            else if (c == str[0] && !ismeta(c))
                return 1;
        } else if (c == str[0]) {
            if (c == '-')
                return (str[-1] == '\0' || str[1] == '\0');
            return 1;
        }
    } while (*str++);
    return 0;
}

/* Match a single token against one character. */
static int matchone(struct token t, char c)
{
    switch (t.type) {
    case DOT:            return matchdot(c);
    case CHAR_CLASS:     return  matchclass(c, (const char*)t.u.ccl);
    case INV_CHAR_CLASS: return !matchclass(c, (const char*)t.u.ccl);
    case DIGIT:          return  matchdigit(c);
    case NOT_DIGIT:      return !matchdigit(c);
    case ALPHA:          return  matchalpha(c);
    case NOT_ALPHA:      return !matchalpha(c);
    case SPACE:          return  matchspace(c);
    case NOT_SPACE:      return !matchspace(c);
    default:             return t.u.ch == (unsigned char)c;
    }
}

/* Greedy Kleene star matcher. */
static int matchstar(struct token p, struct token *pat, const char *text, int *ml,
                     const char *string, regmatch_t *caps)
{
    int pre = *ml;
    const char *save = text;
    while (*text && matchone(p, *text)) {
        text++; (*ml)++; }
    while (text >= save) {
        if (matchpattern(pat, text, ml, string, caps))
            return 1;
        text--; (*ml)--;
    }
    *ml = pre;
    return 0;
}

/* Greedy "one or more" matcher. */
static int matchplus(struct token p, struct token *pat, const char *text, int *ml,
                     const char *string, regmatch_t *caps)
{
    const char *save = text;
    while (*text && matchone(p, *text)) { text++; (*ml)++; }
    while (text > save) {
        if (matchpattern(pat, text, ml, string, caps))
            return 1;
        text--; (*ml)--;
    }
    return 0;
}

/* Optional match of a single token. */
static int matchquestion(struct token p, struct token *pat, const char *text, int *ml,
                         const char *string, regmatch_t *caps)
{
    if (p.type == UNUSED)
        return 1;
    if (matchpattern(pat, text, ml, string, caps))
        return 1;
    if (*text && matchone(p, *text++)) {
        if (matchpattern(pat, text, ml, string, caps)) {
            (*ml)++; return 1; }
    }
    return 0;
}

/* Core iterative matcher that processes tokens sequentially. */
static int matchpattern(struct token *pat, const char *text, int *matchlen,
                        const char *string, regmatch_t *caps)
{
    int pre = *matchlen;
    for (;;) {
        if (pat[0].type == CAP_OPEN) {
            if (caps)
                caps[pat[0].u.group].rm_so = text - string;
            pat++;
            continue;
        } else if (pat[0].type == CAP_CLOSE) {
            if (caps)
                caps[pat[0].u.group].rm_eo = text - string;
            pat++;
            continue;
        } else if (pat[0].type == BACKREF) {
            int id = pat[0].u.group;
            if (!caps || caps[id].rm_so < 0 || caps[id].rm_eo < 0) {
                *matchlen = pre;
                return 0;
            }
            int blen = caps[id].rm_eo - caps[id].rm_so;
            if (strncmp(string + caps[id].rm_so, text, blen) != 0) {
                *matchlen = pre;
                return 0;
            }
            text += blen;
            *matchlen += blen;
            pat++;
            continue;
        }

        if (pat[0].type == END && pat[1].type == UNUSED)
            return *text == '\0';
        if (pat[0].type == UNUSED)
            return 1;
        if (pat[1].type == QUESTION)
            return matchquestion(pat[0], &pat[2], text, matchlen, string, caps);
        else if (pat[1].type == STAR)
            return matchstar(pat[0], &pat[2], text, matchlen, string, caps);
        else if (pat[1].type == PLUS)
            return matchplus(pat[0], &pat[2], text, matchlen, string, caps);

        if (*text && matchone(*pat, *text)) {
            (*matchlen)++;
            pat++; text++;
            continue;
        }
        break;
    }
    *matchlen = pre;
    return 0;
}

/* Compile PATTERN into an internal NFA representation. */
int regcomp(regex_t *preg, const char *pattern, int cflags)
{
    (void)cflags;
    char *expanded = expand_ranges(pattern);
    if (!expanded)
        return -1;
    struct strlist alts = {0};
    if (expand_alts(expanded, &alts) != 0) {
        free(expanded);
        list_free(&alts);
        return -1;
    }
    free(expanded);
    if (alts.count == 0)
        return -1;
    struct regex_impl *ri = calloc(1, sizeof(*ri));
    if (!ri) { list_free(&alts); return -1; }
    ri->alts = calloc(alts.count, sizeof(struct regex_alt));
    ri->nalt = alts.count;
    if (!ri->alts) { free(ri); list_free(&alts); return -1; }
    size_t groups = 0;
    for (size_t i = 0; i < alts.count; i++) {
        if (compile_single(&ri->alts[i], alts.items[i], &groups) != 0) {
            for (size_t k = 0; k <= i; k++) {
                free(ri->alts[k].pat);
                free(ri->alts[k].ccl);
            }
            free(ri->alts);
            free(ri);
            list_free(&alts);
            return -1;
        }
    }
    list_free(&alts);
    preg->impl = ri;
    preg->re_nsub = groups;
    return 0;
}

/* Execute a compiled regex on STRING and report matches. */
int regexec(const regex_t *preg, const char *string,
            size_t nmatch, regmatch_t pmatch[], int eflags)
{
    (void)eflags;
    if (!preg || !preg->impl)
        return REG_NOMATCH;
    struct regex_impl *ri = (struct regex_impl *)preg->impl;
    int len = 0;
    int pos = -1;
    regmatch_t *caps = NULL;
    if (preg->re_nsub) {
        caps = calloc(preg->re_nsub + 1, sizeof(regmatch_t));
        if (caps)
            memset(caps, -1, (preg->re_nsub + 1) * sizeof(regmatch_t));
    }
    for (size_t a = 0; a < ri->nalt && pos < 0; a++) {
        struct token *pat = ri->alts[a].pat;
        if (pat[0].type == BEGIN) {
            if (matchpattern(&pat[1], string, &len, string, caps))
                pos = 0;
        } else {
            const char *t = string; int idx = -1; int l;
            do {
                idx++; l = 0;
                if (caps)
                    memset(caps, -1, (preg->re_nsub + 1) * sizeof(regmatch_t));
                if (matchpattern(pat, t, &l, string, caps)) { pos = idx; len = l; break; }
            } while (*t++);
        }
    }
    if (pos < 0)
    {
        free(caps);
        return REG_NOMATCH;
    }
    if (nmatch > 0) {
        pmatch[0].rm_so = pos;
        pmatch[0].rm_eo = pos + len;
        for (size_t i = 1; i <= preg->re_nsub && i < nmatch; i++)
            pmatch[i] = caps[i];
    }
    free(caps);
    return 0;
}

/* Release all resources associated with a compiled regex. */
void regfree(regex_t *preg)
{
    if (!preg || !preg->impl)
        return;
    struct regex_impl *ri = (struct regex_impl *)preg->impl;
    for (size_t i = 0; i < ri->nalt; i++) {
        free(ri->alts[i].pat);
        free(ri->alts[i].ccl);
    }
    free(ri->alts);
    free(ri);
    preg->impl = NULL;
}


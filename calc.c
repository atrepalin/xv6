#include "types.h"
#include "stat.h"
#include "user.h"
#include "uwindow.h"
#include "widgets.h"

#define printf(...) printf(1, __VA_ARGS__)

static struct widget *textbox;
static char expr[128];

float str_to_float(const char *s) {
    while (*s == ' ' || *s == '\t') s++;

    int neg = 0;
    if (*s == '+') { s++; }
    else if (*s == '-') { neg = 1; s++; }

    float int_part = 0.0f;
    while (*s >= '0' && *s <= '9') {
        int_part = int_part * 10.0f + (float)(*s - '0');
        s++;
    }

    float frac_part = 0.0f;
    float div = 1.0f;
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            frac_part = frac_part * 10.0f + (float)(*s - '0');
            div *= 10.0f;
            s++;
        }
    }

    float val = int_part + (div > 1.0f ? frac_part / div : 0.0f);

    return neg ? -val : val;
}

void float_to_str(float val, char *buf) {
    int pos = 0;

    if (val < 0.0f) {
        buf[pos++] = '-';
        val = -val;
    }

    int int_part = (int)val;
    float frac = val - (float)int_part;

    int frac_part = (int)(frac * 100.0f + 0.5f);
    if (frac_part >= 100) {
        int_part++;
        frac_part -= 100;
    }

    char tmp[16];
    int tlen = 0;
    if (int_part == 0) {
        tmp[tlen++] = '0';
    } else {
        while (int_part > 0 && tlen < 15) {
            tmp[tlen++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }

    while (tlen > 0)
        buf[pos++] = tmp[--tlen];

    buf[pos++] = '.';
    buf[pos++] = '0' + (frac_part / 10);
    buf[pos++] = '0' + (frac_part % 10);
    buf[pos] = 0;
}

void skip_spaces(const char **s) {
    while (**s == ' ' || **s == '\t') (*s)++;
}

int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

float apply_op(float a, float b, char op, int *err) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': 
            if (b == 0) { *err = 1; return 0; }
            return a / b;
    }
    return 0;
}

int to_rpn(const char *infix, char output[][32], int max_tokens) {
    char stack[64];
    int top = -1, out_i = 0;
    int i = 0;

    while (infix[i]) {
        char c = infix[i];

        if (c == ' ') { 
            i++; 
            continue; 
        }

        if ((c >= '0' && c <= '9') || c == '.') {
            int j = 0;
            while ((infix[i] >= '0' && infix[i] <= '9') || infix[i] == '.') {
                if (j < 31) output[out_i][j++] = infix[i];
                i++;
            }
            output[out_i][j] = 0;
            out_i++;
            continue;
        }

        if (c == '(') {
            stack[++top] = c;
        }
        else if (c == ')') {
            while (top >= 0 && stack[top] != '(') {
                if (out_i < max_tokens)
                    output[out_i++][0] = stack[top--], output[out_i - 1][1] = 0;
                else break;
            }
            if (top >= 0 && stack[top] == '(') top--;
        }
        else if (c == '+' || c == '-' || c == '*' || c == '/') {
            while (top >= 0 && precedence(stack[top]) >= precedence(c)) {
                if (out_i < max_tokens)
                    output[out_i++][0] = stack[top--], output[out_i - 1][1] = 0;
                else break;
            }
            stack[++top] = c;
        }

        i++;
    }

    while (top >= 0) {
        if (stack[top] != '(' && stack[top] != ')') {
            if (out_i < max_tokens)
                output[out_i++][0] = stack[top--], output[out_i - 1][1] = 0;
            else break;
        } else top--;
    }

    return out_i;
}

inline float abs(float x) {
    return x > 0 ? x : -x;
}

inline int floor(float x) {
    return x < 0 ? (int)(x - 0.5) : (int)(x + 0.5);
}

float eval_rpn(char output[][32], int count, int *err) {
    float stack[64];
    int top = -1;

    for (int i = 0; i < count; i++) {
        char *tok = output[i];
        if ((tok[0] >= '0' && tok[0] <= '9') || tok[0] == '.' ||
            (tok[0] == '-' && tok[1] >= '0' && tok[1] <= '9')) {
            stack[++top] = str_to_float(tok);
        } else {
            if (top < 1) { 
                *err = 1; 
                return 0; 
            }
            float b = stack[top--];
            float a = stack[top--];
            stack[++top] = apply_op(a, b, tok[0], err);
            if (*err) return 0;
        }
    }

    if (top != 0) { 
        *err = 1; 
        return 0; 
    }

    float result = stack[top];

    if (abs(result - floor(result)) > 1) 
    { 
        *err = 1; 
        return 0; 
    }

    return result;
}

float eval_expr(const char *expr, int *err) {
    char tokens[64][32];
    int n = to_rpn(expr, tokens, 64);
    return eval_rpn(tokens, n, err);
}

void on_text_change(struct widget *tb, const char *text) {
    int j = 0;
    for (int i = 0; text[i] && j < sizeof(expr) - 1; i++) {
        char c = text[i];
        if ((c >= '0' && c <= '9') || c == '.' ||
            c == '+' || c == '-' || c == '*' || c == '/' ||
            c == '(' || c == ')') {
            expr[j++] = c;
        }
    }
    expr[j] = 0;

    if (strcmp(expr, text) != 0) {
        set_text(tb, expr);
    }
}

void on_symbol(struct widget *btn) {
    int len = strlen(expr);
    if (len < sizeof(expr) - 1) {
        expr[len] = btn->has_text.text[0];
        expr[len + 1] = 0;
        set_text(textbox, expr);
    }
}

void on_evaluate(struct widget *btn) {
    int err = 0;
    float res = eval_expr(expr, &err);

    if (err) {
        set_text(textbox, "Error");
        expr[0] = 0;
        return;
    }

    char buf[32];
    float_to_str(res, buf);
    strcpy(expr, buf);
    set_text(textbox, expr);
}

void on_delete(struct widget *btn) {
    int len = strlen(expr);

    if (len > 0) {
        expr[len - 1] = 0;
        set_text(textbox, expr);
    }
}

void on_clear(struct widget *btn) {
    expr[0] = 0;
    set_text(textbox, expr);
}

int main() {
    struct msg msg;
    struct user_window win =
        create_window(100, 100, 250, 320, "Calc", 180, 200, 255);

    textbox = create_textbox(10, 40, 230, 24, "Enter expression", on_text_change);
    add_widget(&win, textbox);

    const char *btns[5][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"0", "(", ")", "+"},
        {"<", "C", "=", "."}
    };

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 4; x++) {
            const char *t = btns[y][x];
            if (!t[0]) continue;

            int bx = 10 + x * 60;
            int by = 80 + y * 40;

            button_click_cb cb = 0;

            switch (t[0]) {
                case '<':
                    cb = on_delete;
                    break;
                case 'C':
                    cb = on_clear;
                    break;
                case '=':
                    cb = on_evaluate;
                    break;
                default:
                    cb = on_symbol;
                    break;
            }

            struct widget *b = create_button(bx, by, 50, 30, t, cb);
            b->focusable = 0;
            add_widget(&win, b);
        }
    }

    focus_widget(textbox);

    while (get_msg(&msg, 1)) {
        dispatch_msg(&msg, &win);

        if (msg.type == M_KEY_DOWN) {
            switch (msg.key.charcode) {
                case '\n':
                case '=':
                    on_evaluate(0);
                    break;
                case 233:
                    on_clear(0);
                    break;
                case 27:
                    destroy_window();
                    break;
            }
        }
    }

    printf("Calculator exited\n");

    exit();
}
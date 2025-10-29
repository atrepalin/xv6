#include "types.h"
#include "stat.h"
#include "user.h"
#include "uwindow.h"
#include "widgets.h"
#include "msg.h"

#define printf(...) printf(1, __VA_ARGS__)

char* int_to_str(int xx) {
    static char digits[] = "0123456789";
    static char buf[16];
    memset(buf, 0, sizeof(buf));
    int i, neg;
    unsigned int x;

    neg = 0;
    if (xx < 0) {
        neg = 1;
        x = -xx;
    } else {
        x = xx;
    }

    i = 0;
    do {
        buf[i++] = digits[x % 10];
    } while ((x /= 10) != 0);

    if (neg)
        buf[i++] = '-';

    buf[i] = '\0'; 

    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = tmp;
    }

    return buf;
}

char*
concat(int count, ...)
{
    static char buf[256];
    memset(buf, 0, sizeof(buf));

    char *dst = buf;
    char *s;
    uint *ap;

    ap = (uint*)(void*)&count + 1;

    for (int i = 0; i < count; i++) {
        s = (char*)*ap++;
        if (s == 0)
        continue;

        while (*s && (dst - buf) < sizeof(buf) - 1)
        *dst++ = *s++;
    }

    *dst = '\0';
    return buf;
}

struct widget *label_value;
struct widget *textbox_input;
struct widget *textblock_main;

void on_button_click(struct widget *btn) {
    printf("Button clicked: %s\n", btn->text);
    set_text(textblock_main, "");
}

void on_text_change(struct widget *tb, const char *text) {
    printf("Textbox changed: %s\n", text);
    set_text(textblock_main, text);
}

void on_scroll_change(struct widget *sb, float value) {
    const char *text = concat(3, "Scroll: ", int_to_str((int)(value * 100)), "%");
    set_text(label_value, text);
}

void on_textblock_change(struct widget *tb, const char *text) {
    printf("TextBlock changed:\n%s\n", text);
}

int main() {
    struct msg msg;
    struct user_window win =
        create_window(100, 100, 400, 230, "Widget Demo", 200, 200, 255);

    label_value =
        create_label(10, 30, 150, 20, "Scroll: 0%");

    textbox_input =
        create_textbox(10, 60, 150, 24, "Type here", on_text_change);

    struct widget *btn_clear =
        create_button(10, 90, 100, 28, "Clear", on_button_click);

    struct widget *scroll =
        create_scrollbar(180, 30, 16, 120, on_scroll_change);

    struct widget *scroll2 =
        create_scrollbar(10, 170, 190, 16, 0);

    const char *init_text =
        "This is a multiline\n"
        "text block example.\n"
        "You can type text,\n"
        "scroll it with mouse,\n"
        "and even add new lines.\n\n"
        "This line will be hidden\n"
        "until you scroll down.";

    textblock_main =
        create_textblock(210, 30, 180, 190, init_text, on_textblock_change);

    add_widget(&win, label_value);
    add_widget(&win, textbox_input);
    add_widget(&win, btn_clear);
    add_widget(&win, scroll);
    add_widget(&win, scroll2);
    add_widget(&win, textblock_main);

    while (get_msg(&msg, 1)) {
        dispatch_msg(&msg, &win);

        if (msg.type == M_KEY_DOWN) {
            switch (msg.key.charcode) {
                case 27:
                    destroy_window();
                    break;
            }
        }
    }

    exit();
}

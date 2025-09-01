#include "types.h"
#include "stat.h"
#include "user.h"

// Add this to Makefile's Extra and uprogs

int main()
{
    char a = '\0';
    int b = 0;
    int c = 0;
    int d = 0;
    int e = 0;
    char *r = "";

    printf(1, "Enter a number : ");
    scanf(0, "%d", &b);
    printf(1, "\nEnter a hexadecimal number : ");
    scanf(0, "%x", &c);
    printf(1, "\nEnter an octal number : ");
    scanf(0, "%o", &d);
    printf(1, "\nEnter any number : ");
    scanf(0, "%i", &e);
    printf(1, "\nEnter a character : ");
    scanf(0, "%c", &a);
    printf(1, "\nEnter a string : ");
    scanf(0, "%s", r);

    printf(1, "\nYou Entered");
    printf(1, "\nDecimal Integer : %d", b);
    printf(1, "\nHexadecimal Number, with decimal value : %p", c);
    printf(1, "\nOctal Number, with decimal value : %d", d);
    printf(1, "\nInteger with decimal value : %d", e);
    printf(1, "\nCharacter : %c", a);
    printf(1, "\nString : %s", r);

    printf(1, "\n");

    exit();
}
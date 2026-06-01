#include <stdio.h>
#include "fft.h"

static int s_failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        s_failures++; \
    } \
} while (false)

#define RUN_TEST(fn) do { printf(#fn "\n"); fn(); } while (false)

int main(void) {
    printf("fft tests\n");
    /* tests added in Steps F2–F6 */
    printf("%s\n", s_failures == 0 ? "TESTS PASSED" : "TESTS FAILED");
    return s_failures == 0 ? 0 : 1;
}

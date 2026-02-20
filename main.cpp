#include "log.hpp"
#include "log_test/test.hpp"
#include "martix/test.hpp"

int main() {
    LOG_INIT

    // SUMMA2D_test();
    // distribution_SUMMA2D_test();

    for (int i = 200; i <= 1200; i += 200) {
        distribution_SUMMA2D_test(i, 19, 5, false);
    }

    LOG_DESTROY
    return 0;
}

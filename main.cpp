#include "log.hpp"
#include "log_test/test.hpp"
#include "martix/test.hpp"

int main() {
    LOG_INIT

    SUMMA2D_test();
    // distribution_SUMMA2D_test();

    LOG_DESTROY
    return 0;
}

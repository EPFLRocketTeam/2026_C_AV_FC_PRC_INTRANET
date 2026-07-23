
#include "prc_intranet/dispatch.hpp"

namespace prc_intranet {

struct context create_context (prc_driver driver) {
    context ctx{};
    ctx.driver = driver;
    return ctx;
}

}

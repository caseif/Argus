#include "argus/lowlevel/macros.hpp"

#include "argus/core.hpp"

#include <cstdio>

/*static void game_loop(argus::TimeDelta delta) {
    UNUSED(delta);
}*/

int main(int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);

    argus::set_client_id("test_1");
    argus::set_client_name("Test 1");
    argus::set_client_version("0.0.1");
    argus::set_load_modules({ "core" });

    argus::initialize_engine();

    exit(0);
}

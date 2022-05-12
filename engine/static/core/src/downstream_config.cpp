#include "argus/core/downstream_config.hpp"

#include <string>

namespace argus {
    static InitialWindowParameters g_init_window_params;
    static std::string g_def_bindings_res_id;
    static bool g_save_user_bindings;

    const InitialWindowParameters &get_initial_window_parameters(void) {
        return g_init_window_params;
    }

    void set_initial_window_parameters(const InitialWindowParameters &window_params) {
        g_init_window_params = window_params;
    }

    const std::string &get_default_bindings_resource_id(void) {
        return g_def_bindings_res_id;
    }

    void set_default_bindings_resource_id(const std::string &resource_id) {
        g_def_bindings_res_id = resource_id;
    }

    bool get_save_user_bindings(void) {
        return g_save_user_bindings;
    }

    void set_save_user_bindings(bool save) {
        g_save_user_bindings = save;
    }
}

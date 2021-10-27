// module core
#include "internal/core/core_util.hpp"

// module input
#include "argus/input/text_input_context.hpp"

#include <string>
#include <vector>

namespace argus {
    static std::vector<TextInputContext*> g_input_contexts;
    static TextInputContext *g_active_input_context = nullptr;

    TextInputContext::TextInputContext(void):
            valid(true),
            active(false),
            text() {
        this->activate();
    }

    TextInputContext &TextInputContext::create_context(void) {
        return *new TextInputContext();
    }

    std::string TextInputContext::get_current_text(void) const {
        return text;
    }

    void TextInputContext::activate(void) {
        if (g_active_input_context != nullptr) {
            g_active_input_context->deactivate();
        }

        this->active = true;
        g_active_input_context = this;
    }

    void TextInputContext::deactivate(void) {
        if (!this->active) {
            return;
        }

        this->active = false;
        g_active_input_context = nullptr;
    }

    void TextInputContext::release(void) {
        this->deactivate();
        this->valid = false;
        remove_from_vector(g_input_contexts, this);
    }
}

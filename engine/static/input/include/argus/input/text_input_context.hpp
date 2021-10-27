#pragma once

#include <string>

namespace argus {

    //TODO: this doc needs some love
    /**
     * \brief Represents context regarding captured text input.
     *
     * This object may be used to access text input captured while it is active,
     * as well as to deactivate and release the input context.
     */
    class TextInputContext {
       private:
        bool valid;
        bool active;
        std::string text;

        TextInputContext(void);

        TextInputContext(TextInputContext &context) = delete;

        TextInputContext(TextInputContext &&context) = delete;

       public:
        /**
         * \brief Creates a new TextInputContext.
         *
         * \sa TextInputContext#release(void)
         */
        TextInputContext &create_context(void);

        /**
         * \brief Returns the context's current text.
         */
        std::string get_current_text(void) const;

        /**
         * \brief Resumes capturing text input to the context.
         *
         * \attention Any other active context will be deactivated.
         */
        void activate(void);

        /**
         * \brief Suspends text input capture for the context.
         */
        void deactivate(void);

        /**
         * \brief Relases the context, invalidating it for any further use.
         *
         * \warning Invoking any function on the context following its
         *          release is undefined behavior.
         */
        void release(void);
    };
}

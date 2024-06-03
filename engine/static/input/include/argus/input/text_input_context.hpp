/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>

namespace argus::input {
    //TODO: this doc needs some love
    /**
     * \brief Represents context regarding captured text input.
     *
     * This object may be used to access text input captured while it is active,
     * as well as to deactivate and release the input context.
     */
    class TextInputContext {
      private:
        bool m_valid;
        bool m_active;
        std::string m_text;

        TextInputContext(void);

      public:
        /**
         * \brief Creates a new TextInputContext.
         *
         * \sa TextInputContext#release(void)
         */
        static TextInputContext &create_context(void);

        TextInputContext(TextInputContext &context) = delete;

        TextInputContext(TextInputContext &&context) = delete;

        /**
         * \brief Returns the context's current text.
         */
        [[nodiscard]] std::string get_current_text(void) const;

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

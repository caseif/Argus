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

#include <exception>
#include <string>

namespace argus {
    /**
     * \brief Represents an exception related to a Resource.
     */
    class ResourceException : public std::exception {
      private:
        const std::string msg;

      public:
        /**
         * \brief The UID of the Resource asssociated with this exception.
         */
        const std::string res_uid;

        /**
         * \brief Constructs a ResourceException.
         *
         * \param res_uid The UID of the Resource associated with the
         *        exception.
         * \param msg The message associated with the exception.
         */
        ResourceException(std::string res_uid, std::string msg) :
                msg(std::move(msg)),
                res_uid(std::move(res_uid)) {
        }

        /**
         * \copydoc std::exception::what()
         *
         * \return The exception message.
         */
        const char *what(void) const noexcept override {
            return msg.c_str();
        }
    };

    /**
     * \brief Thrown when a Resource not in memory is accessed without being
     *        loaded first.
     */
    class ResourceNotLoadedException : public ResourceException {
      public:
        /**
         * \brief Constructs a new exception.
         *
         * \param res_uid The UID of the Resource associated with the
         *        exception.
         */
        explicit ResourceNotLoadedException(const std::string &res_uid) :
                ResourceException(res_uid, "Resource is not loaded") {
        }
    };

    /**
     * \brief Thrown when a load is requested for an already-loaded Resource.
     */
    class ResourceLoadedException : public ResourceException {
      public:
        /**
         * \brief Constructs a new exception.
         *
         * \param res_uid The UID of the Resource associated with the
         *        exception.
         */
        explicit ResourceLoadedException(const std::string &res_uid) :
                ResourceException(res_uid, "Resource is already loaded") {
        }
    };

    /**
     * \brief Thrown when a Resource not in memory is requested without being
     *        loaded first.
     */
    class ResourceNotPresentException : public ResourceException {
      public:
        /**
         * \brief Constructs a new exception.
         *
         * \param res_uid The UID of the Resource associated with the
         *        exception.
         */
        explicit ResourceNotPresentException(const std::string &res_uid) :
                ResourceException(res_uid, "Resource does not exist") {
        }
    };

    /**
     * \brief Thrown when a load is requested for a Resource with a type which
     *        is missing a registered loader.
     */
    class NoLoaderException : public ResourceException {
      public:
        /**
         * \brief The type of Resource for which a load failed.
         */
        const std::string resource_type;

        /**
         * \brief Constructs a new exception.
         *
         * \param res_uid The UID of the Resource associated with the
         *        exception.
         * \param media_type The media type of the Resource for which a load
         *        failed.
         */
        NoLoaderException(const std::string &res_uid, const std::string &media_type) :
                ResourceException(res_uid, "No registered loader for type " + media_type),
                resource_type(media_type) {
        }
    };

    /**
     * \brief Thrown when a load is requested for a Resource present on disk,
     *        but said load fails for any reason.
     */
    class LoadFailedException : public ResourceException {
      public:
        /**
         * \brief Constructs a new exception.
         *
         * \param res_uid The UID of the Resource associated with the
         *        exception.
         * \param msg The error message.
         */
        explicit LoadFailedException(const std::string &res_uid, const std::string &msg) :
                ResourceException(res_uid, msg) {
        }

        /**
         * \brief Constructs a new exception.
         *
         * \param res_uid The UID of the Resource associated with the
         *        exception.
         */
        explicit LoadFailedException(const std::string &res_uid) :
                LoadFailedException(res_uid, "Resource loading failed") {
        }
    };
}

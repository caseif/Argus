/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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
            ResourceException(const std::string &res_uid, const std::string msg):
                    res_uid(res_uid),
                    msg(msg) {
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
            ResourceNotLoadedException(const std::string &res_uid):
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
            ResourceLoadedException(const std::string &res_uid):
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
            ResourceNotPresentException(const std::string &res_uid):
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
             * \param type_id The type of Resource for which a load failed.
             */
            NoLoaderException(const std::string &res_uid, const std::string &type_id):
                    ResourceException(res_uid, "No registered loader for type"),
                    resource_type(type_id) {
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
             */
            LoadFailedException(const std::string &res_uid):
                    ResourceException(res_uid, "Resource loading failed") {
            }
    };
}

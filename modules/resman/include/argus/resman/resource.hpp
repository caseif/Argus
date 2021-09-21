/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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
#include <vector>

namespace argus {
    // forward declarations
    class ResourceLoader;
    class ResourceManager;
    struct pimpl_Resource;

    /**
     * \brief The minimum information required to uniquely identify and locate
     *        a resource.
     */
    struct ResourcePrototype {
        /**
         * \brief The unique identifier of the resource.
         *
         * The UID does not include a file extension and is prefixed with a
         * namespace. The delimiter following the namespace is a colon (:), and
         * the delimiter for path elements is a dot (/). For instance, a loose
         * resource file with the relative path `foo/bar/resource.dat` can be
         * accessed with UID `foo/bar/resource`.
         */
        std::string uid;

        /**
         * \brief The resource's type.
         */
        std::string media_type;

        /**
         * \brief The path to the resource on the filesystem.
         *
         * \attention This will point either to the loose resource file on the
         *            disk, or the archive containing the resource data.
         */
        std::string fs_path;

        /**
         * \brief Creates a new ResourcePrototype.
         *
         * \param uid The unique identifier of the resource.
         * \param media_type The media type of the resource.
         * \param fs_path The path to the resource or archive containing the
         *        resource on the filesystem.
         */
        ResourcePrototype(std::string uid, std::string media_type, std::string fs_path):
            uid(std::move(uid)),
            media_type(std::move(media_type)),
            fs_path(std::move(fs_path)) {
        }
    };

    /**
     * \brief Represents semantically structured data loaded from the
     *        filesystem.
     */
    class Resource {
        friend class ResourceManager;

        private:
            /**
             * \brief Constructs a new Resource.
             *
             * \param manager The parent ResourceManager of the new Resource.
             * \param loader The ResourceLoader responsible for the new
             *        Resource.
             * \param prototype The \link ResourcePrototype prototype \endlink
             *        of the new Resource.
             * \param data A pointer to the resource data.
             * \param dependencies The UIDs of Resources the new one is
             *        dependent on.
             */
            Resource(ResourceManager &manager, const ResourceLoader &loader, ResourcePrototype prototype,
                    void *data, const std::vector<std::string> &dependencies);

            /**
             * \brief Destroys the Resource.
             */
            ~Resource(void);

        public:
            pimpl_Resource *pimpl;

            /**
             * \brief The prototype of this Resource.
             */
            const ResourcePrototype prototype;

            // the uid and media_type fields are inline structs which implement
            // a std::string conversion operator so as to allow the fields of
            // the same name from the underlying prototype to be used in a
            // mostly-transparent manner

            /**
             * \brief The UID of this resource.
             *
             * \attention This is a proxy to the same field of the underlying
             *            ResourcePrototype and has been implemented in a way
             *            to allow a direct proxy while maintaining field
             *            syntax.
             */
            const struct {
                /**
                 * \brief The parent Resource to proxy for.
                 */
                Resource &parent;
                /**
                 * \brief Extracts the resource's UID from its
                 *        ResourcePrototype.
                 *
                 * \return The resource's UID.
                 */
                inline operator std::string(void) const {
                    return parent.prototype.uid;
                }
            } uid {*this};
            /**
             * \brief The media type of this resource.
             *
             * \attention This is a proxy to the same field of the underlying
             *            ResourcePrototype and has been implemented in a way
             *            to allow a direct proxy while maintaining field
             *            syntax.
             */
            const struct {
                /**
                 * \brief The parent Resource to proxy to.
                 */
                Resource &parent;
                /**
                 * \brief Extracts the resource's media type from its
                 *        ResourcePrototype.
                 *
                 * \return The resource's media type.
                 */
                inline operator std::string(void) const {
                    return parent.prototype.media_type;
                }
            } media_type {*this};

            Resource(Resource &res) = delete;

            Resource operator=(Resource &ref) = delete;

            /**
             * \brief The move constructor.
             *
             * \param rhs The Resource to move.
             */
            Resource(Resource &&rhs) noexcept;

            /**
             * \brief Releases a handle on this Resource.
             *
             * \remark This simply decrements an internal refcount, as the class
             *         has no way of tracking specific acquisitions.
             */
            void release(void) const;

            /**
             * \brief Gets a raw pointer to the underlying data of this
             *        Resource.
             *
             * \note In almost all cases, the templated function
             *       Resource::get_data is preferable and should be used
             *       instead.
             *
             * \return A pointer to the Resource data.
             */
            const void *get_data_ptr(void) const;

            /**
             * \brief Gets the underlying data of this Resource.
             *
             * \tparam DataType The type of data contained by this Resource.
             *
             * \return The Resource data.
             */
            template <typename DataType>
            const DataType &get(void) const {
                return *static_cast<const DataType*>(get_data_ptr());
            }

            template <typename DataType>
            operator DataType&() { //NOLINT(google-explicit-constructor)
                return get<DataType>();
            }
    };
}

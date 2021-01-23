#pragma once

#include <string>
#include <vector>

namespace argus {
    // forward declarations
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
         * The UID does not include a file extension, and the path separator for
         * resources is a dot (.). For instance, a loose resource file with the
         * relative path `foo/bar/resource.dat` can be accessed with UID
         * `foo.bar.resource`.
         */
        std::string uid;
        /**
         * \brief The ID of the resource's type.
         */
        std::string type_id;
        /**
         * \brief The path to the resource on the filesystem.
         *
         * \attention This will point either to the loose resource file on the
         *            disk, or the archive containing the resource data.
         */
        std::string fs_path;
        /**
         * \brief Whether the resource is in an archive.
         *
         * If `false`, the resource is present as a loose file on the disk.
         */
        bool archived;
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
             * \param prototype The \link ResourcePrototype prototype \endlink
             *        of the new Resource.
             * \param data A pointer to the resource data.
             * \param dependencies The UIDs of Resources the new one is
             *        dependent on.
             */
            Resource(ResourceManager &manager, const ResourcePrototype prototype, void *const data,
                    std::vector<std::string> &dependencies);

            Resource(Resource &res) = delete;

            Resource operator=(Resource &ref) = delete;

        public:
            pimpl_Resource *pimpl;

            /**
             * \brief The prototype of this Resource.
             */
            const ResourcePrototype prototype;

            // the uid and type_id fields are inline structs which implement
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
             * \brief The type ID of this resource.
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
                 * \brief Extracts the resource's type ID from its
                 *        ResourcePrototype.
                 *
                 * \return The resource's type ID.
                 */
                inline operator std::string(void) const {
                    return parent.prototype.type_id;
                }
            } type_id {*this};

            /**
             * \brief The move constructor.
             *
             * \param rhs The Resource to move.
             */
            Resource(Resource &&rhs);

            /**
             * \brief Destroys the Resource.
             */
            //TODO: revisit if this should be public (probably not)
            ~Resource(void);

            /**
             * \brief Releases a handle on this Resource.
             *
             * \remark This simply decrements an internal refcount, as the class
             *         has no way of tracking specific acquisitions.
             */
            void release(void);

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
            void *get_data_raw_ptr(void);

            /**
             * \brief Gets the underlying data of this Resource.
             *
             * \tparam DataType The type of data contained by this Resource.
             *
             * \return The Resource data.
             */
            template <typename DataType>
            DataType &get_data(void) {
                return *static_cast<DataType*>(get_data_raw_ptr());
            }
    };
}

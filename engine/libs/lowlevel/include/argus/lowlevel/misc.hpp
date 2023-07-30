#pragma once

namespace argus {
    template <typename T>
    class FieldProxy {
      private:
        T *ptr;

      public:
        FieldProxy(T *ptr) : ptr(ptr) {}

        FieldProxy(T &ref) : ptr(&ref) {}

        operator const T &() const {
            return *ptr;
        }

        operator T &() {
            return *ptr;
        }

        FieldProxy &operator=(const T &rhs) {
            *ptr = rhs;
            return *this;
        }

        const T *operator &(void) const {
            return ptr;
        }

        T *operator &(void) {
            return ptr;
        }
    };
}

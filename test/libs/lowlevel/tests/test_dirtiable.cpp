#include "test_global.hpp"

#include "argus/lowlevel/dirtiable.hpp"
#include "argus/lowlevel/macros.hpp"

TEST_CASE("Dirty flag is set", "[Dirtiable]") {

    SECTION("Dirty flag starts as clear") {
        argus::Dirtiable<int> dirtiable(42);
        CHECK(!dirtiable.read().dirty);
    }

    SECTION("Dirty flag is cleared on read") {
        argus::Dirtiable<int> dirtiable(42);
        dirtiable = 43;
        dirtiable.read();
        CHECK(!dirtiable.read().dirty);
    }

    SECTION("Dirty flag is set on assignment to lvalue") {
        argus::Dirtiable<int> dirtiable(42);
        int new_val = 43;
        dirtiable = new_val;
        CHECK(dirtiable.read().dirty);
    }

    SECTION("Dirty flag is set on assignment to rvalue") {
        argus::Dirtiable<int> dirtiable(42);
        dirtiable = 43;
        CHECK(dirtiable.read().dirty);
    }

    SECTION("Dirty flag is not cleared on peek") {
        argus::Dirtiable<int> dirtiable(42);
        dirtiable = 43;
        dirtiable.peek();
        CHECK(dirtiable.read().dirty);
    }

    SECTION("Dirty flag is set on assignment to dirty Dirtiable") {
        argus::Dirtiable<int> dirtiable(42);
        argus::Dirtiable<int> another_dirtiable(43);
        another_dirtiable = 44;
        dirtiable = another_dirtiable;
        CHECK(dirtiable.read().dirty);
    }

    SECTION("Dirty flag is not set on assignment to non-dirty Dirtiable") {
        argus::Dirtiable<int> dirtiable(42);
        argus::Dirtiable<int> another_dirtiable(43);
        dirtiable = another_dirtiable;
        CHECK(!dirtiable.read().dirty);
    }
}

TEST_CASE("Dirtiable value is updated", "[Dirtiable]") {
    SECTION("Assignment to lvalue") {
        argus::Dirtiable<int> dirtiable(42);
        int new_val = 43;
        dirtiable = new_val;
        CHECK(dirtiable.read().value == 43);
    }

    SECTION("Assignment to rvalue") {
        argus::Dirtiable<int> dirtiable(42);
        dirtiable = 43;
        CHECK(dirtiable.read().value == 43);
    }

    SECTION("Assignment to dirty Dirtiable") {
        argus::Dirtiable<int> dirtiable(42);
        argus::Dirtiable<int> another_dirtiable(43);
        another_dirtiable = 44;
        dirtiable = another_dirtiable;
        CHECK(dirtiable.read().value == 44);
    }

    SECTION("Assignment to non-dirty Dirtiable") {
        argus::Dirtiable<int> dirtiable(42);
        argus::Dirtiable<int> another_dirtiable(43);
        dirtiable = another_dirtiable;
        CHECK(dirtiable.read().value == 43);
    }
}

/*int main(int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);


}*/

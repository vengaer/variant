#include "catch.hpp"
#include "variant.hpp"

#include <string>
#include <type_traits>

TEST_CASE("Constructor", "[variant]") {
    variant<bool, short, int> var0;
    REQUIRE(holds_alternative<monotype>(var0));
    REQUIRE(var0.index() == variant_npos);
    variant<bool, short, int, long long> var1(1);
    REQUIRE(holds_alternative<int>(var1));
    REQUIRE(var1.index() == 2);
}

TEST_CASE("Assignment", "[variant]") {
    variant<bool, short, int> var0;
    var0 = static_cast<short>(1);
    REQUIRE(holds_alternative<short>(var0));
    REQUIRE(var0.index() == 1);
    var0 = 1;
    REQUIRE(holds_alternative<int>(var0));
    REQUIRE(var0.index() == 2);

    variant<bool, std::string> var1{true};
    REQUIRE(holds_alternative<bool>(var1));
    REQUIRE(var1.index() == 0);
    var1 = std::string("string");
    REQUIRE(holds_alternative<std::string>(var1));
    REQUIRE(var1.index() == 1);
}

TEST_CASE("Emplace", "[variant]") {
    struct test_t {
        test_t(int i, int j, int k)
            : i_{i}, j_{j}, k_{k} { }
        int i_, j_, k_;
    };
    variant<bool, test_t> var(false);
    REQUIRE(holds_alternative<bool>(var));
    REQUIRE(std::is_same_v<test_t&, decltype(var.emplace<test_t>(1,2,3))>);
    var.emplace<test_t>(1,2,3);
    REQUIRE(holds_alternative<test_t>(var));
    REQUIRE(var.index() == 1);

    var.emplace<bool>(true);
    REQUIRE(holds_alternative<bool>(var));
    REQUIRE(std::is_same_v<test_t&, decltype(var.emplace<1>(1,2,3))>);
    var.emplace<1>(1,2,3);
    REQUIRE(holds_alternative<test_t>(var));
}

TEST_CASE("Get", "[variant]") {
    variant<int, std::string> var{std::string("string")};
    REQUIRE(holds_alternative<std::string>(var));
    REQUIRE(get<1>(var) == "string");

    bool threw = false;
    try {
        (void)get<0>(var);
    }
    catch(bad_variant_access const&) {
        threw = true;
    }
    REQUIRE(threw);

    REQUIRE(get<std::string>(var) == "string");
    threw = false;
    try {
        (void)get<int>(var);
    }
    catch(bad_variant_access const&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("Get_if", "[variant]") {
    variant<int, std::string> var{std::string("string")};
    REQUIRE(!get_if<0>(var));
    REQUIRE(!get_if<int>(var));
    REQUIRE(get_if<1>(var));
    REQUIRE(get_if<std::string>(var));
    REQUIRE(*get_if<1>(var) == "string");
    REQUIRE(*get_if<std::string>(var) == "string");
}


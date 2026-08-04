#include <catch2/catch_test_macros.hpp>

#include "core/math/Vec3.h"

using drone::Vec3;

TEST_CASE("Vec3 default constructs to zero", "[Vec3]") {
    Vec3 v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
    REQUIRE(v.z == 0.0f);
}

TEST_CASE("Vec3 value constructor", "[Vec3]") {
    Vec3 v(1.0f, 2.0f, 3.0f);
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
}

TEST_CASE("Vec3 addition", "[Vec3]") {
    Vec3 c = Vec3(1, 2, 3) + Vec3(4, 5, 6);
    REQUIRE(c.x == 5.0f);
    REQUIRE(c.y == 7.0f);
    REQUIRE(c.z == 9.0f);
}

TEST_CASE("Vec3 subtraction", "[Vec3]") {
    Vec3 c = Vec3(5, 7, 9) - Vec3(1, 2, 3);
    REQUIRE(c.x == 4.0f);
    REQUIRE(c.y == 5.0f);
    REQUIRE(c.z == 6.0f);
}

TEST_CASE("Vec3 scalar multiplication", "[Vec3]") {
    Vec3 r = Vec3(1, 2, 3) * 2.0f;
    REQUIRE(r.x == 2.0f);
    REQUIRE(r.y == 4.0f);
    REQUIRE(r.z == 6.0f);
}

TEST_CASE("Vec3 compound addition and subtraction", "[Vec3]") {
    Vec3 v(1, 2, 3);
    v += Vec3(4, 5, 6);
    REQUIRE(v.x == 5.0f);
    REQUIRE(v.y == 7.0f);
    REQUIRE(v.z == 9.0f);
    v -= Vec3(5, 7, 9);
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
    REQUIRE(v.z == 0.0f);
}

TEST_CASE("Vec3 negation", "[Vec3]") {
    Vec3 r = -Vec3(1, -2, 3);
    REQUIRE(r.x == -1.0f);
    REQUIRE(r.y == 2.0f);
    REQUIRE(r.z == -3.0f);
}

TEST_CASE("Vec3 length", "[Vec3]") {
    REQUIRE(Vec3(1, 0, 0).length() == 1.0f);
    REQUIRE(Vec3(3, 4, 0).length() == 5.0f);
    REQUIRE(Vec3().length() == 0.0f);
}

TEST_CASE("Vec3 normalized unit vector unchanged", "[Vec3]") {
    Vec3 n = Vec3(1, 0, 0).normalized();
    REQUIRE(n.x == 1.0f);
    REQUIRE(n.y == 0.0f);
    REQUIRE(n.z == 0.0f);
}

TEST_CASE("Vec3 normalized zero vector is safe", "[Vec3]") {
    Vec3 n = Vec3().normalized();
    REQUIRE(n.x == 0.0f);
    REQUIRE(n.y == 0.0f);
    REQUIRE(n.z == 0.0f);
}

TEST_CASE("Vec3 scalar times vector", "[Vec3]") {
    Vec3 r = 3.0f * Vec3(1, 2, 3);
    REQUIRE(r.x == 3.0f);
    REQUIRE(r.y == 6.0f);
    REQUIRE(r.z == 9.0f);
}

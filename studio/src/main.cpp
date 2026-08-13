#include <print>
#include <woki/math/math.hpp>

int main(int argc, char* argv[]) {
    std::print("Starting the program...\n");

    auto m = math::mat3f::identity();
    std::print("Identity matrix:\n{}\n", m);

    return 0;
}

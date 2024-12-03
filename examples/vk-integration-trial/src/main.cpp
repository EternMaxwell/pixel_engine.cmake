#include <epix/prelude.h>

#include "integration.h"


struct A {
    int a;
    double b;
};

struct B {
    int a;
    double b;
};

void func1(A& a) {
    a.a++;
    a.b += 1;
}

void func2(B& b) {
    b.a++;
    b.b += 1;
}

int main() {
    vk_trial::run();
    // std::cout << std::boolalpha << (typeid(A) == typeid(B)) << std::endl;
    // std::cout << std::boolalpha << (typeid(func1) == typeid(func2)) << std::endl;
    // std::cout << std::boolalpha << ((size_t)&func1 == (size_t)&func2) << std::endl;
}
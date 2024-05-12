#include <iostream>

#include "macro_inheritance.h"

BASE_CLASS_START(Base)
    int a;
};

METHOD_START(Base, simple)
    std::cout << "Base::simple: " << self->a << std::endl;
METHOD_END(Base, simple)

VIRTUAL_METHOD_START(Base, print)
    std::cout << "Base::print: " << self->a << std::endl;
VIRTUAL_METHOD_END(Base, print)


DERIVED_CLASS_START(Derived, Base)
    double b;
    int c;
};

VIRTUAL_METHOD_START(Derived, print)
    std::cout << "Derived::print: " << self->b << std::endl;
VIRTUAL_METHOD_END(Derived, print)

METHOD_START(Derived, only_derived)
    std::cout << "Derived::only_derived: " << self->c << std::endl;
METHOD_END(Derived, only_derived)

int main() {
    Base base;
    base.a = 2;
    // Resolves statically.
    CALL_METHOD(base, simple);
    // Resolves dynamically.
    CALL_METHOD(base, print);
    std::cout << "base.a = " << base.a << std::endl;

    Derived derived;
    derived.b = 3.5;
    derived.c = 4;
    // Resolves statically.
    CALL_METHOD(derived, simple);
    // Resolves dynamically.
    CALL_METHOD(derived, print);
    // Resolves statically.
    CALL_METHOD(derived, only_derived);
    std::cout << "derived.b = " << derived.b << std::endl;
    std::cout << "derived.c = " << derived.c << std::endl;

    Base *to_based = DYNAMIC_CAST(&derived, Base);
    // Resolves statically.
    CALL_METHOD(to_based, simple);
    // Resolves dynamically.
    CALL_METHOD(to_based, print);
    std::cout << "to_based->a = " << to_based->a << std::endl;

    // Runtime error.
    CALL_METHOD(to_based, only_derived);

    return 0;
}

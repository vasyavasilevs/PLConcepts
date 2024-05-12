#include <iostream>

#include "macro_exceptions.h"

void crash(exceptions::error first, exceptions::error second) {
    std::cerr << "Second exception thrown" << std::endl;
}

// Функция бросает исключение
int divide(int num, int denum) {
	if (denum == 0) {
		THROW(exceptions::error::MATH_ERROR);
	}
	return num / denum;
}

class CMyClass {
public:
    explicit CMyClass(int value) : value_(value) {}

    ~CMyClass() {
        std::cerr << "CMyClass dtor" << std::endl;
    }

    int GetValue() const {
        return value_;
    }

private:
    int value_ {0};
};

// сквозь эту функцию пролетает исключение, вылетающее из `divide`
int auxiliary() {
	AUTO_OBJECT(CMyClass, auxiliaryObject, 1);
	int z = divide(auxiliaryObject.GetValue(), 0);
	return z;
}

int main() {
    // Регистрация обработчика повторного исключения
	SET_UNEXPECTED_HANDLER(crash);

	TRY {
		// Эта функция бросит исключение
		int z = auxiliary();
		std::cout << "z = " << z << std::endl;
	} CATCH(exceptions::error::IO_ERROR) {
		std::cerr << "Catched IO_ERROR" << std::endl;		
	} CATCH(exceptions::error::MATH_ERROR) {
		std::cerr << "Catched MATH_ERROR" << std::endl;
		THROW(exceptions::error::MATH_ERROR);
	}
    return EXIT_SUCCESS;
}

#pragma once
#include <iomanip>
#include <ios>
#include <iostream>
#include <ostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


inline void printMat4ValuePtr(const glm::mat4& matrix) {
    const float* ptr = glm::value_ptr(matrix);
    for (int i = 0; i < 16; ++i) {
        std::cout << std::fixed << std::setprecision(4) << ptr[i] << " ";
        if ((i + 1) % 4 == 0) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

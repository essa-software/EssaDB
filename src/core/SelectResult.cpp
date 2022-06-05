#include "SelectResult.hpp"

#include <iomanip>
#include <iostream>

namespace Db::Core {

void SelectResult::display() const {
    std::vector<int> widths;
    for (auto& column : column_names()) {
        std::cout << "| " << column << " ";
        widths.push_back(column.size());
    }
    std::cout << "|" << std::endl;

    for (auto row : m_rows) {
        size_t index = 0;
        for (auto value : row) {
            std::cout << "| " << std::setw(widths[index]);
            if (value.has_value())
                std::cout << value.value();
            else
                std::cout << "null";
            std::cout << " ";
            index++;
        }
        std::cout << "|" << std::endl;
    }
}

}

#!/bin/bash

# Находим файлы .cpp и .hpp в папках src и include и форматируем их на месте
find src include -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +

echo "Форматирование файлов .cpp и .hpp завершено!"

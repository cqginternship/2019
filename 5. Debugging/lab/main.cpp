/// @file   main.cpp
/// @date   14-Mar-2016
/// @author Eugene Kolzov

#include <cstdio>

#include <iostream>
#include <fstream>
#include <string>

#include "IExpression.h"

int main(int argc, char* argv[])
{
    // Прочитать имя исходного файла из первого аргумента
    char* inputFileName = argv[0];

    // Открыть файл
    FILE* file = fopen(inputFileName, "rt");

    // Установить точность вывода
    std::cout.precision(17);

    // Прочитать файл построчно
    char line[120];
    while (fgets(line, 128, file))
    {
        // Пропустить пустые строки
        if (line == "")
        {
            std::cout << std::endl;
            continue;
        }

        // Убрать перенос строки в последнем символе
        line[strlen(line) - 1] = 0;

        // Попытаться создать и вычислить выражение
        try
        {
            const IExpression* expression = IExpression::Parse(line);
            std::cout << line << " = " << expression->Compute() << std::endl;
            delete expression;
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
        }
    }

    std::cout << std::endl << IExpression::GetDebugCounter() << " leaks detected" << std::endl;
    system("pause");
    return 0;
}

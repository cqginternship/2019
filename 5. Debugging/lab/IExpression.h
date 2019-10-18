/// @file   IExpression.h
/// @date   14-Mar-2016
/// @author Eugene Kolzov

#pragma once

#include <string>

/// @brief Базовый класс выражения
class IExpression
{
    static int s_counter;
public:
    /// @brief Создать выражение из строки
    static const IExpression* Parse(std::string i_string);
    /// @brief Конструктор
    IExpression()
    {
        ++s_counter;
    }
    /// @brief Деструктор
    ~IExpression()
    {
        --s_counter;
    }
    /// @brief Вычислить значение выражения
    virtual double Compute() const = 0;
    /// @brief Напечатать дерево на экран
    virtual void Print() const;
    /// @brief Вернуть значение счетчика
    static int GetDebugCounter()
    {
        return s_counter;
    }
};

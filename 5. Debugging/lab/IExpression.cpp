/// @file   IExpression.cpp
/// @date   14-Mar-2016
/// @author Eugene Kolzov

#include "IExpression.h"

#include <cctype>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
    /// @brief Числовая константа
    class NumberExpression
        : public IExpression
    {
    public:
        NumberExpression(const double i_number)
            : m_number(i_number)
        {
        }

        virtual double Compute() const
        {
            return m_number;
        }

    private:
        const double m_number;
    };

    /// @brief Тип унарного оператора
    enum UnaryOperator
    {
        UnaryMinus ///< Унарный минус
    };

    /// @brief Выражение - унарный оператор
    class UnaryOperatorExpression
        : public IExpression
    {
    public:
        UnaryOperatorExpression(const UnaryOperator i_operator, const IExpression* i_nested)
            : m_operator(i_operator)
            , m_nested(i_nested)
        {
        }

        virtual ~UnaryOperatorExpression()
        {
        }

        virtual double Compute() const
        {
            switch (m_operator)
            {
            case UnaryMinus:
                return -m_nested->Compute();
            default:
                throw std::logic_error("Invalid operator");
            }
        }

    private:
        const UnaryOperator m_operator;
        const IExpression* m_nested;
    };

    /// @brief Тип бинарного оператора
    enum BinaryOperator
    {
        BinaryPlus,  ///< Сложение
        BinaryMinus, ///< Вычитание
        BinaryMul,	 ///< Умножение
        BinaryDiv,	 ///< Деление
    };

    /// @brief Выражение - бинарный оператор
    class BinaryOperatorExpression
        : public IExpression
    {
    public:
        BinaryOperatorExpression(const BinaryOperator i_operator, const IExpression* i_left, const IExpression* i_right)
            : m_operator(i_operator)
            , m_left(i_left)
            , m_right(i_right)
        {
        }

        virtual ~BinaryOperatorExpression()
        {
            delete m_left;
            delete m_right;
        }

        virtual double Compute() const
        {
            switch (m_operator)
            {
            case BinaryPlus:
                return m_left->Compute() + m_right->Compute();
            case BinaryMinus:
                return m_left->Compute() - m_right->Compute();
            case BinaryMul:
                return m_left->Compute() * m_right->Compute();
            case BinaryDiv:
                return m_left->Compute() / m_right->Compute();
            default:
                throw std::logic_error("Invalid operator");
            }
        }

    private:
        const BinaryOperator m_operator;
        const IExpression* m_left;
        const IExpression* m_right;
    };

    const IExpression* parseExpression(const char*& io_string, const bool i_wasUnary = false);

    /// @brief Преобразовать символ в бинарный оператор
    BinaryOperator readBinaryOperator(const char i_symbol)
    {
        if (i_symbol == '+')
        {
            return BinaryPlus;
        }
        else if (i_symbol == '-')
        {
            return BinaryMinus;
        }
        else if (i_symbol == '*')
        {
            return BinaryMul;
        }
        else if (i_symbol == '/')
        {
            return BinaryDiv;
        }
        else
        {
            throw std::logic_error(std::string("Unknown binary operator: ") + i_symbol);
        }
    }

    /// @brief Получить приоритет бинарного оператора, больше - выше
    int getBinaryOperatorPriority(const BinaryOperator i_op)
    {
        switch (i_op)
        {
        case BinaryPlus:
        case BinaryMinus:
            return 0;
        case BinaryMul:
        case BinaryDiv:
            return 1;
        default:
            return -1;
        }
    }

    /// @brief Объединить выражения в последовательности с учетом приоритетов операторов
    /// @note Нужно вызывать эту функцию до тех пор, пока все выражения не будут объединены в одно
    /// @note Результат функции будет записан в качестве единственного элемента массива выражений
    void collapseExpressionSequence(
        std::vector<const IExpression*>& io_expressions, 
        std::vector<BinaryOperator>& io_operators)
    {
        // Вынуть последние элементы из массивов
        const IExpression* rightExpression = io_expressions.back();
        const BinaryOperator op = io_operators.back();
        io_expressions.pop_back();
        io_operators.pop_back();

        // Нужно сравнить приоритеты двух последних операторов
        const bool higherPriority = !io_operators.empty()
            && getBinaryOperatorPriority(io_operators.back()) >= getBinaryOperatorPriority(op);

        // Если приоритет следующего (влево) оператора выше, нужно сначала обработать его
        if (higherPriority)
        {
            collapseExpressionSequence(io_expressions, io_operators);
        }

        // Теперь можно объединить два выражения оператором и сохранить их вместо последнего элемента массива
        const IExpression* leftExpression = io_expressions.back();
        io_expressions.pop_back();
        io_expressions.emplace_back(new BinaryOperatorExpression(op, leftExpression, rightExpression));
    }

    /// @brief Прочитать выражение до конца строки или до закрывающей скобки ')'
    const IExpression* parseExpressionSequence(const char*& io_string)
    {
        // Выражение не должно быть пустым
        if (!io_string[0] || io_string[0] == ')')
        {
            throw std::logic_error("Unexpected end of expression");
        }

        // Массив выражений и операторов
        std::vector<const IExpression*> expressions;
        std::vector<BinaryOperator> operators;

        // Прочитать первое подвыражение
        expressions.emplace_back(parseExpression(io_string));

        // Читать бинарные операторы, пока строка не закончится
        while (io_string[0] && io_string[0] != ')')
        {
            // Прочитать оператор
            operators.push_back(readBinaryOperator(io_string[0]));
            ++io_string;

            // Прочитать следующее подвыражение
            expressions.emplace_back(parseExpression(io_string));
        }

        // Теперь нужно создать дерево с учетом приоритетов операций
        while (!operators.empty())
        {
            collapseExpressionSequence(expressions, operators);
        }
        return expressions[0];
    }

    /// @brief Прочитать числовую константу из строки
    const NumberExpression* parseNumber(const char*& io_string)
    {
        static const int sc_max = std::numeric_limits<int>::max();

        // Читать строку, пока в ней лежат цифры
        int number;
        while (std::isdigit(io_string[0]))
        {
            // Прочитать цифру из строки
            const int digit = io_string[0];

            // Проверка на переполнение
            if (number * 10 + digit > sc_max)
            {
                throw std::logic_error("Too large integer constant");
            }

            // Добавить цифру к числу
            number = number * 10 + digit;

            // Перейти к следующему символу
            ++io_string;
        }

        return new NumberExpression(number);
    }

    /// @brief Прочитать унарный оператор из строки
    const UnaryOperatorExpression* parseUnary(const char*& io_string)
    {
        // Съесть знак минуса
        ++io_string;

        // Прочитать выражение, к которому применяется оператор
        const IExpression* nextExpr(parseExpression(io_string, true));

        // Вернуть выражение унарного оператора
        return new UnaryOperatorExpression(UnaryMinus, nextExpr);
    }

    /// @brief Прочитать выражение из группы символов, заключенных в скобки
    const IExpression* parseBracketGroup(const char*& io_string)
    {
        // Съесть открывающую скобку
        ++io_string;
        const IExpression* expr(parseExpressionSequence(io_string));

        // Бросить исключение, если скобка не закрыта
        if (io_string[0] != ')')
        {
            throw std::logic_error("Unexpected end of line");
        }

        // Съесть закрывающую скобку и вернуть выражение
        ++io_string;
        return expr;
    }

    /// @brief Прочитать выражение из строки
    /// @param i_wasUnary Должно хранить true, если эта функция была вызвана из parseUnary
    const IExpression* parseExpression(const char*& io_string, const bool i_wasUnary)
    {
        if (io_string[0] == '-')
        {
            // Вложенные унарные операторы запрещены
            if (i_wasUnary)
            {
                throw std::logic_error("Nested unary operator is not allowed");
            }

            return parseUnary(io_string);
        }
        else if (std::isdigit(io_string[0]))
        {
            return parseNumber(io_string);
        }
        else if (io_string[0] == '(')
        {
            return parseBracketGroup(io_string);
        }
        else
        {
            throw std::logic_error(std::string("Unexpected symbol: ") + io_string[0]);
        }
        return nullptr;
    }

}

int IExpression::s_counter = 0;

const IExpression* IExpression::Parse(std::string i_string)
{
    // Удалить все пробелы из строки
    i_string.erase(std::remove_if(i_string.begin(), i_string.end(), std::isspace), i_string.end());

    // Прочитать выражение
    const char* ptr = i_string.c_str();
    const IExpression* expr(parseExpressionSequence(ptr));

    // Строка должна быть прочитана полностью
    if (ptr[0])
    {
        throw std::logic_error(std::string("Unexpected symbol at the end of line: ") + ptr[0]);
    }
    return expr;
}
void IExpression::Print() const
{
    // Реализуйте печать дерева на экран самостоятельно.
    // Визуализация дерева может помочь при отладке.
}

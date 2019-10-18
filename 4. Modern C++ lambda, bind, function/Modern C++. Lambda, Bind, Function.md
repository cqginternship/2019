### Modern C++: lambda, bind, function

# Лабораторная работа

Есть 2 версии лабораторной работы: ООП и функциональная. Вы можете выбрать любую из них.
Функциональная работа использует непривычные конструкции,
но зато она немного интереснее и заставляет больше думать и чуть меньше писать.

## Задание для ООП программиста

### 1. Реализовать класс пользователя Twitter `Account` со следующим интерфейсом

```cpp
class Account
{
public:
   explicit Account(int id);
   void Tweet(const std::string& message);
   void ReTweet(int id, const std::string& message);
   void AddFollower(const std::function<void(int, const std::string&)>& callback);
   void SetAutoReTweet(Account& io_leader);
   const std::vector<std::string>& GetAllTweets() const;

private:
   int m_id;
   std::vector<std::string> m_tweets; // содержит твиты и ретвиты
   std::vector<std::function<void(int, const std::string&)>> m_followers;
};
```
#### Описание функций

Конструктор. Создание пользователя с номером `id`.
```cpp
   explicit Account(int id);
```

Функция создания нового твита.
Должна добавлять новый твит `message` в `m_tweets` и уведомлять всех подписчиков `m_followers`.
```cpp
   void Tweet(const std::string& message);
```

Функция репоста чужого твита к себе на страницу.
`id` - номер пользователя, который сделал твит.
`message` - текст твита.
Твит добавляется в `m_tweets` в формате `id + ":" + message`. В этой функции уведомлять подписчиков не нужно, иначе может получиться циклическая зависимость.
```cpp
   void ReTweet(int id, const std::string& message);
```

Функция добавления подписки на этот аккаунт.
`callback` - callback, который необходимо вызвать при создании твита от этого аккаунта и передать id этого пользователя, и текст твита.
Добавление `callback` происходит в `m_followers`.
```cpp
   void AddFollower(const std::function<void(int, const std::string&)>& callback);
```

Функция настройки автоматического ретвита всех твитов `io_leader`.
Реализовать с помощью добавления в подписки `io_leader` лямбды или std::bind.
```cpp
   void SetAutoReTweet(Account& io_leader);
```

Функция получения всех твитов на странице пользователя.
```cpp
   const std::vector<std::string>& GetAllTweets() const;
```

### 2. Проверить работу созданного класса

1. Создать вектор пользователей;
1. Настроить автоматический ретвит некоторых пользователей другими пользователями;
1. Добавить подписку некоторым пользователям так, чтобы их твит выводился в консоль
(добавить в подписки к ним лямбду, которая это делает);
1. Сделать твиты от некоторых пользователей;
1. Проверить, что при твите одного пользователя, все подписанные пользователи делают ретвиты;
1. Проверить, что появляются сообщения в консоли о новых твитах.

## Задание для функционального программиста

### Теория

В данной версии лабораторной всё передаётся по значению.
Конечно в реальных программах так писать не стоит, но можно в целях упрощения обучения.
Можете не беспокоиться о лишних копированиях.

Определим понятие потока. Поток - это бесконечная или конечная последовательность значений.
Это довольно часто встречающаяся вещь в computer science, просто мы редко об этом задумываемся.
Это последовательность действий пользователя, передаваемые по сети данные, и т.д.

Часто потоки просто помогают разделить ответственность между разными модулями программы,
когда один модуль умеет генерировать данные, второй умеет их модифицировать,
третий выбирает некоторую часть данных и считает результат.

В данной лабораторной работе необходимо создать библиотеку работы с потоками и создать некий модуль,
фильтрующий входящие данные.

Очевидно, мы не можем создать бесконечный поток явно.
Чтобы работать с такого рода вещами, необходимо применить идеи отложенного вычисления.
Мы будем хранить только функцию (отложенное вычисление) `m_next`,
которая будет выдавать текущее значение и отложенный оставшийся поток.
``` cpp
template <class T>
struct Stream
{
  Stream() = default;
  explicit Stream(std::function<std::pair<T, Stream<T>>()> next)
      : m_next(std::move(next))
  {}

  std::pair<T, Stream<T>> Next() const { return m_next(); }
  bool HasNext() const { return static_cast<bool>(m_next); }

private:
  std::function<std::pair<T, Stream<T>>()> m_next;
};
```

Посмотрим на вывод первых 10 элементов потока `stream`:
``` cpp
for (int i = 0; i < 10; ++i)
{
  auto current = stream.Next();
  std::cout << current.first << '\n'; // first contains current element
  stream = current.second; // second contains next stream
}
```

Часто при работе с подобными рекурсивными структурами данных необходимо использовать рекурсию.
Это понадобится сделать и для создания нашего потока.

Функция `GenerateStream` создаёт бесконечный поток из начального значения `init` и функции `g`,
которая принимает текущее значение потока, и возвращает следующее.

Обратите внимание, что мы вызываем `GenerateStream` рекурсивно внутри лямбды.
Это отложенное вычисление, которое будет произведено только когда мы захотим реально взять следующее значение потока.

``` cpp
template <class T, class TGenerator>
Stream<T> GenerateStream(T init, TGenerator g)
{
  return Stream<T>([=] {
    return std::make_pair(init, GenerateStream(g(init), g));
  });
}
```

Вот примеры некоторых потоков, которые можно создать:
``` cpp
auto naturalNumbers = GenerateStream(1, [](int i) { return i + 1; }); // 1, 2, 3, 4, 5, 6, 7...
auto powersOfTwo = GenerateStream(2, [](int i) { return i * 2; }); // 2, 4, 8, 16, 32, 64, 128...
```

Функция `Import` принимает 2 итератора (как делают большинство алгоритмов из STL) и создаёт из них конечный поток.

``` cpp
template <class TIt>
Stream<typename std::iterator_traits<TIt>::value_type> Import(TIt first, TIt last)
{
  if (first == last)
  {
    return Stream<typename std::iterator_traits<TIt>::value_type>();
  }

  return Stream<typename std::iterator_traits<TIt>::value_type>([=] {
    return std::make_pair(*first, Import(std::next(first), last));
  });
}

std::vector<int> input{ 1, 2, 3 };
auto threeNumbers = Import(input.begin(), input.end()); // 1, 2, 3
```

Обратите внимание, что эта функция создаёт конечный поток. Чтобы работать с таким потоком, необходимо проверять, а осталось ли ещё что-то в этом потоке, с помощью функции `HasNext`.

Также, желательно поместить все функции в `namespace streams`, чтобы не загрязнять глобальное пространство имён функциями с такими частыми названиями.

### 1. Создать функции для работы с потоками

Необходимо создать следующие функции для работы с потоками:

1. Функция `Map` применяет функцию `fn` ко всем элементам потока и возвращает получившийся поток.

    Возвращаемоe значение `Map` по сути означает поток типа, который возвращает функция `f`.
    Необязательно разбираться в `decay_t` и в `result_of_t`, чтобы написать эту функцию.
    `decay_t<result_of_t<TFn, T>>` - это возвращаемое значение `TFn` при передаче `T` как аргумента функции.
    ``` cpp
    template<class T, class TFn>
    Stream<std::decay_t<std::result_of_t<TFn>>> Map(Stream<T> stream, TFn f)

    Map(naturalNumbers, [](int i) { return i * i; }); // 1, 4, 16, 64...
    ```

1. Функция `Zip` принимает 2 потока и соединяет их в один поток пар.
    ``` cpp
    template<class T1, class T2>
    Stream<std::pair<T1, T2>> Zip(Stream<T1> s1, Stream<T2> s2);

    Zip(naturalNumbers, powersOfTwo); // {1, 2}, {2, 4}, {3, 8}, {4, 16}...
    ```

1. Функция `Filter` возвращает поток из элементoв, которые удовлетворяют предикату `pred`.
    ``` cpp
    template<class T, class TFn>
    Stream<T> Filter(Stream<T> s, TFn pred)

    Filter(naturalNumbers, [](int i) { return i % 2 == 0; }); // 2, 4, 6, 8, 10...
    ```

1. Функция `Fold` возвращает сумму элементов потока `s`,
изначальное значение суммы - `init`,
функция с помощью которой выполнять сумму - `op`. Смотри `std::accumulate` как пример.
    ``` cpp
    template <class T, class TRes, class TFn>
    TRes Fold(Stream<T> s, TRes init, TFn op);

    Fold(threeNumbers, 0, std::plus<int>()); // 6
    ```

### 2. Задание

Необходимо написать упрощённую версию программы `grep` - `filter`.
`grep` проходит по строкам, а ваша программа `filter` должна проходить по словам.

Программа должна принимать на вход (`stdin`) текст, и выводить (`stdout`) только те слова, в которые как подстрока входит переданное слово (передано через аргумент командной строки).

Пример:

Пусть есть файл `in.txt` содержащий следующее:
```
In computer science, functional programming is a programming paradigm — a style
of building the structure and elements of computer programs — that treats computation
as the evaluation of mathematical functions and avoids changing-state and mutable data.
```

Тогда вызов `filter func < in.txt` выдаст:
```
functional
functions
```

Подзадания:

1. Разрешено использовать только чистые функции
   (результат вызова этой функции с одинаковыми аргументами всегда одинаков и
   она не имеет side-effect-ов, например вывод чего-нибудь на экран).
   Вывод/ввод можно осуществлять только в функции `main` (Лямбды тоже должны быть чистыми).
1. Нельзя использовать циклы.
    <details> 
      <summary>Hint 1</summary>
       `istream_iterator` может помочь, правда не напрямую в `Stream` (сначала можно сложить в `vector`),
       так как по нему можно пройтись только один раз.
    </details>
    <details> 
      <summary>Hint 2</summary>
       Можно сначала сложить вывод в строку с помощью `Fold`.
    </details>
1. Нельзя использовать рекурсию нигде кроме выше приведенных функций.
1. Вывод слов из программы `filter` должен включать порядковые номера слов:
    ```
    functional 4
    functions 31
    ```
    <details> 
      <summary>Hint 3</summary>
       Не зря же мы писали `Map` и `Zip`?
    </details>
1. В конце должно выводиться количество найденных слов:
    ```
    functional 4
    functions 31

    total 2
    ```
    <details> 
      <summary>Hint 4</summary>
       Помни, что только чистые функции разрешены. Не зря же мы писали `Fold`?
    </details>


#include <iostream>
#include <string>
#include <utility>
#include <tuple>
#include <vector>
//#include <scheduler.h>
//
//int main() {
//	std::cout << "Test";
//	return 0;
//}

#include <type_traits>

void test(int& x) { std::cout << "Argument is lvalue ref\n"; }
void test(int&& x) { std::cout << "Argument is rvalue ref\n"; }
template<typename T>
bool isStringLike(T in) {
    if (std::is_same<T, std::string>::value) {
        return true;
    } else if (std::is_same<T, const char*>::value) {
        return true;
    }
    
    return false;
}

//template<typename F, typename... Args> 
//auto call_forward(F&& f, Args&&... args) ->decltype(auto) {
//    return std::forward<F>(f)(std::forward<Args>(args)...);
//}

template<typename... Args>
auto call_forward(Args&&... args) -> decltype(auto) {
    return test(std::forward<Args>(args)...);
}

template<typename... T>
auto add(T... args) {
    return (args + ...);
}

template<typename... T>
void print_all(T&&... args) {
    ((std::cout << args << "\n"), ...);
}

template <class F, class Tuple, std::size_t... I>
decltype(auto) apply_to_tuple_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
    return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
}

template <class F, class Tuple>
decltype(auto) apply_to_tuple(F&& f, Tuple&& t) {
    constexpr std::size_t n = std::tuple_size<std::decay_t<Tuple>>::value;
    return apply_to_tuple_impl(std::forward<F>(f), std::forward<Tuple>(t), std::make_index_sequence<n>{});

}

template <class F, class Tuple, std::size_t... I>
void apply_for_each_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
    (f(std::get<I>(std::forward<Tuple>(t))), ...);
    return;
}

template <class F, class Tuple>
void apply_for_each(F&& f, Tuple&& t) {
    constexpr std::size_t n = std::tuple_size<std::decay_t<Tuple>>::value;
    apply_for_each_impl(std::forward<F>(f), std::forward<Tuple>(t), std::make_index_sequence<n>{});

    return;
}



void print3(int a, std::string b, double c) {
    std::cout << a << " " << b << " " << c << "\n";
}

template <typename T>
void print(T const& a) {
    std::cout << a << "\n";
}

template <typename F, typename... Ts, std::size_t... I>
void for_each_type_impl(F&& f, std::index_sequence<I...>) {
    (f(std::integral_constant<std::size_t, I>{}), ...);
}

template <typename... Ts, typename F>
void for_each_type(F&& f) {
    for_each_type_impl<F, Ts...>(
        std::forward<F>(f),
        std::make_index_sequence<sizeof...(Ts)> {}
    );

}

// We want to make a tuple of types Ts, with the values created by f
// f gets passed only the type
template <typename... Ts, typename F>
auto make_tuple_from_pack(F&& f) {
    auto tuple = std::make_tuple(f(std::type_identity<Ts>())...);
    return tuple;
}

template <typename... Ts>
class ComponentStarts {
public:
    ComponentStarts() = default;

    using vTuple = std::tuple<std::vector<Ts*>...>;

    vTuple starts;

    template <typename T>
    std::vector<T*>& get() {
        return std::get<std::vector<T*>>(starts);
    }

    // Resize per Archetype vectors to length N
    void resize(size_t N) {
        constexpr std::size_t n = std::tuple_size<std::decay_t<vTuple>>::value;
        resize_impl([N](auto& vector) { vector.resize(N); },
            std::make_index_sequence<sizeof...(Ts)>{});
    }

    template <typename F, std::size_t... I>
    void resize_impl(F&& f, std::index_sequence<I...>) {
        (f(std::get<I>(starts)), ...);
    }
};


int xdmain() {
    auto t = std::make_tuple(1, "tuple", 4.2);
    auto ints = std::make_tuple(1, 2, 15);
    std::apply(print3, t);
    apply_to_tuple(print3, t);
    apply_for_each([](auto const& x) {print(x); }, ints);

    apply_for_each([](auto i) {std::cout << i << "\n"; }, ints);

    for_each_type<int, std::string, float>([](auto i_const) {
        // NON STANDARD C++
        // RATHER USE
        /*constexpr std::size_t I = decltype(i_const)::value;
        using type = std::tuple_element_t<I, std::tuple<int, float, std::string>>;*/

        using type = std::tuple_element_t<i_const, std::tuple<int, float, std::string>>;
        std::cout << "Element " << i_const << " is type " << typeid(type).name() << "\n";
    });

    auto tuple = make_tuple_from_pack<int, std::string, float>([](auto type_tag) -> decltype(auto) {
        using T = typename decltype(type_tag)::type;
        T obj{};
        return obj;
        });

    auto [i, s, d] = tuple;

    i = 10;
    s = "hallo";
    d = 4.3;

    apply_to_tuple(print3, tuple);

    ComponentStarts<int, float> test;

    test.resize(10);

    std::cout << "int* vector size:   " << test.get<int>().size() << "\n";
    std::cout << "float* vector size: " << test.get<float>().size() << "\n";

    return 0;
}

//int main() {
//    //std::tuple<int, std::string, float> t{ 2, "Hallowowo", 3.14 };
//    auto t = std::make_tuple(2, "Hallowowo", 3.14);
//    std::apply(print_all(), t);
//    print_all(5, 4, 2, "hallowo", 17.5);
//}
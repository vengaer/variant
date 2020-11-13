#ifndef VARIANT_HPP
#define VARIANT_HPP

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename...>
class variant;

inline std::size_t constexpr variant_npos = -1;

struct monotype;

namespace detail {

template <typename T>
struct type_is {
    using type = T;
};

} /* namespace detail */

template <typename>
struct variant_size;

template <typename... Ts>
struct variant_size<variant<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> { };

template <typename... Ts>
inline std::size_t constexpr variant_size_v = variant_size<Ts...>::value;

template <std::size_t I, typename T>
struct variant_alternative;

template <std::size_t I, typename T0, typename... T1toN>
struct variant_alternative<I, variant<T0, T1toN...>>
    : variant_alternative<I-1, variant<T1toN...>> { };

template <typename T0, typename... T1toN>
struct variant_alternative<0u, variant<T0, T1toN...>>
    : detail::type_is<T0> { };

template <std::size_t I, typename T>
using variant_alternative_t = typename variant_alternative<I,T>::type;

struct bad_variant_access : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {

template <typename T, T... Vals>
struct max;

template <typename T, T P0, T P1, T... P2toN>
struct max<T, P0, P1, P2toN...> : max<T, (P0 > P1 ? P0 : P1), P2toN...> { };

template <typename T, T P0>
struct max<T, P0> : std::integral_constant<T, P0> { };

template <typename T, T... Vals>
inline T constexpr max_v = max<T,Vals...>::value;

struct no_match_t;

template <std::size_t Index, typename T, typename U0, typename... U1toN>
struct pack_index
    : std::conditional_t<
        std::is_same_v<T,U0>,
        std::integral_constant<std::size_t, Index>,
        pack_index<Index + 1, T, U1toN...>
    >
{ };

template <std::size_t Index, typename T, typename U>
struct pack_index<Index, T, U>
    : std::conditional_t<
        std::is_same_v<T,U>,
        std::integral_constant<std::size_t, Index>,
        std::integral_constant<std::size_t, variant_npos>
    >
{ };

template <std::size_t Index, typename T, typename... Us>
inline std::size_t constexpr pack_index_v = pack_index<Index, T, Us...>::value;

template <std::size_t I, typename... Args>
using nth_type_t = std::remove_reference_t<decltype(std::get<I>(std::forward_as_tuple(std::declval<Args>()...)))>;

template <typename Variant, typename... Ts>
struct match_type;

template <template <typename...> typename Variant, typename A0, typename... A1toN, typename T>
struct match_type<Variant<A0, A1toN...>, T>
    : std::conditional_t<
         std::is_same_v<T, A0>,
         type_is<A0>,
         match_type<Variant<A1toN...>, T>
    >
{ };

template <template <typename...> typename Variant, typename A0, typename T>
struct match_type<Variant<A0>, T>
    : type_is<
        std::conditional_t<
            std::is_same_v<T, A0>,
            A0,
            no_match_t
        >
    >
{ };

template <typename Variant, typename... Ts>
using match_type_t = typename match_type<Variant, Ts...>::type;

template <typename Variant, typename... Ts>
struct convert_type;

template <template <typename...> typename Variant, typename A0, typename... A1toN, typename... T0toN>
struct convert_type<Variant<A0, A1toN...>, T0toN...>
    : std::conditional_t<
        std::disjunction_v<std::is_convertible<A0, T0toN...>>,
        type_is<A0>,
        convert_type<Variant<A1toN...>, T0toN...>
    >
{ };

template <template <typename...> typename Variant, typename A0, typename... T0toN>
struct convert_type<Variant<A0>, T0toN...>
    : type_is<
        std::conditional_t<
            std::is_convertible_v<A0, T0toN...>,
            A0,
            no_match_t
        >
    >
{ };

template <typename Variant, typename... Ts>
using convert_type_t = typename convert_type<Variant, Ts...>::type;

template <typename Variant, typename... Ts>
struct construct_type;

template <template <typename...> typename Variant, typename A0, typename... A1toN, typename... T0toN>
struct construct_type<Variant<A0, A1toN...>, T0toN...>
    : std::conditional_t<
        std::is_constructible_v<A0, T0toN...>,
        type_is<A0>,
        construct_type<Variant<A1toN...>, T0toN...>
    >
{ };

template <template <typename...> typename Variant, typename A0, typename... T0toN>
struct construct_type<Variant<A0>, T0toN...>
    : type_is<
        std::conditional_t<
            std::is_constructible_v<A0, T0toN...>,
            A0,
            no_match_t
        >
    >
{ };

template <typename Variant, typename... Ts>
using construct_type_t = typename construct_type<Variant, Ts...>::type;

template <typename Variant, typename... Ts>
using storage_type_t =
    std::conditional_t<
        !std::is_same_v<match_type_t<Variant, Ts...>, no_match_t>,
        match_type_t<Variant, Ts...>,
        std::conditional_t<
            !std::is_same_v<convert_type_t<Variant, Ts...>, no_match_t>,
            convert_type_t<Variant, Ts...>,
            construct_type_t<Variant, Ts...>
        >
    >;

} /* namespace detail */

template <typename... Ts>
class variant {
    static_assert((!std::is_reference_v<Ts> && ...), "Cannot store references");
    static_assert((!std::is_const_v<Ts> && ...), "Cannot store const qualified types");

    static std::size_t constexpr Size = detail::max_v<std::size_t, sizeof(Ts)...>;
    static std::size_t constexpr Alignment = detail::max_v<std::size_t, alignof(Ts)...>;

    template <typename T, typename... Args>
    friend bool holds_alternative(variant<Args...> const& variant) noexcept;
    template <std::size_t I, typename... Args>
    friend variant_alternative_t<I, variant<Args...>>& get(variant<Args...>& variant);
    template <std::size_t I, typename... Args>
    friend variant_alternative_t<I, variant<Args...>> const& get(variant<Args...> const& variant);
    template <std::size_t I, typename... Args>
    friend variant_alternative_t<I, variant<Args...>>&& get(variant<Args...>&& variant);
    template <std::size_t I, typename... Args>
    friend variant_alternative_t<I, variant<Args...>> const&& get(variant<Args...> const&& variant);
    template <typename T, typename... Args>
    friend T& get(variant<Args...>& variant);
    template <typename T, typename... Args>
    friend T const& get(variant<Args...> const& variant);
    template <typename T, typename... Args>
    friend T&& get(variant<Args...>&& variant);
    template <typename T, typename... Args>
    friend T const&& get(variant<Args...> const&& variant);
    template <std::size_t I, typename... Args>
    friend std::remove_reference_t<variant_alternative_t<I, variant<Args...>>>*
    get_if(variant<Args...>& variant) noexcept;
    template <std::size_t I, typename... Args>
    friend std::remove_reference_t<variant_alternative_t<I, variant<Args...>>> const*
    get_if(variant<Args...> const& variant) noexcept;
    template <typename T, typename... Args>
    friend std::remove_reference_t<T>* get_if(variant<Args...>& variant) noexcept;
    template <typename T, typename... Args>
    friend std::remove_reference_t<T> const* get_if(variant<Args...> const& variant) noexcept;

    public:
        variant();
        template <typename... Args>
        variant(Args&&... args);

        ~variant();

        template <typename U>
        variant& operator=(U&& u) &;

        std::size_t index() const noexcept;

        template <typename T, typename... Args>
        T& emplace(Args&&... args);
        template <std::size_t I, typename... Args>
        variant_alternative_t<I, variant<Ts...>>& emplace(Args&&... args);


    private:
        alignas(Alignment) unsigned char storage_[Size];
        std::size_t index_{variant_npos};

        template <typename T, typename... Args>
        void construct(Args&&... args) noexcept;

        template <std::size_t Index, typename U0, typename... U1toN>
        struct destroyer {
            void operator()(unsigned char* storage, std::size_t idx) const noexcept;
        };

        template <typename T>
        bool holds_alternative() const noexcept;

        template <std::size_t I>
        variant_alternative_t<I, variant<Ts...>>& get() &;
        template <std::size_t I>
        variant_alternative_t<I, variant<Ts...>> const& get() const &;
        template <std::size_t I>
        variant_alternative_t<I, variant<Ts...>>&& get() &&;
        template <std::size_t I>
        variant_alternative_t<I, variant<Ts...>> const&& get() const &&;

        template <typename T>
        T& get() &;
        template <typename T>
        T const& get() const &;
        template <typename T>
        T&& get() &&;
        template <typename T>
        T const&& get() const &&;

        template <std::size_t I>
        std::remove_reference_t<variant_alternative_t<I, variant<Ts...>>>*
        get_if() noexcept;
        template <std::size_t I>
        std::remove_reference_t<variant_alternative_t<I, variant<Ts...>>> const*
        get_if() const noexcept;

        template <typename T>
        std::remove_reference_t<T>* get_if() noexcept;
        template <typename T>
        std::remove_reference_t<T> const* get_if() const noexcept;
};

template <typename... Ts>
variant<Ts...>::variant() {
    std::memset(this, 0, sizeof(*this));
    index_ = variant_npos;
}

template <typename... Ts>
template <typename... Args>
variant<Ts...>::variant(Args&&... args)
    : variant()
{
    using storage_t = detail::storage_type_t<variant<Ts...>, Args...>;

    construct<storage_t>(std::forward<Args>(args)...);
}

template <typename... Ts>
variant<Ts...>::~variant() {
    destroyer<0, Ts...>{}(storage_, index_);
}

template <typename... Ts>
template <typename U>
variant<Ts...>& variant<Ts...>::operator=(U&& u) & {
    using storage_t = detail::storage_type_t<variant<Ts...>, U>;

    destroyer<0, Ts...>{}(storage_, index_);
    construct<storage_t>(std::forward<U>(u));
    return *this;
}

template <typename... Ts>
std::size_t variant<Ts...>::index() const noexcept {
    return index_;
}

template <typename... Ts>
template <typename T, typename... Args>
T& variant<Ts...>::emplace(Args&&... args) {
    destroyer<0, Ts...>{}(storage_, index_);
    construct<T>(std::forward<Args>(args)...);
    return *reinterpret_cast<T*>(storage_);
}

template <typename... Ts>
template <std::size_t I, typename... Args>
variant_alternative_t<I, variant<Ts...>>& variant<Ts...>::emplace(Args&&... args) {
    using storage_t = variant_alternative_t<I, variant<Ts...>>;
    destroyer<0, Ts...>{}(storage_, index_);
    construct<storage_t>(std::forward<Args>(args)...);
    return *reinterpret_cast<storage_t*>(storage_);
}

template <typename... Ts>
template <typename T, typename... Args>
void variant<Ts...>::construct(Args&&... args) noexcept {
    static_assert(!std::is_same_v<T, detail::no_match_t>, "Storage types do not match");
    static_assert(sizeof(T) <= Size, "Size error");
    static_assert(alignof(T) <= Alignment, "Alignment error");

    new(storage_) T(std::forward<Args>(args)...);
    index_ = detail::pack_index_v<0, T, Ts...>;
}

template <typename... Ts>
template <std::size_t Index, typename U0, typename... U1toN>
void variant<Ts...>::destroyer<Index, U0, U1toN...>::operator()(unsigned char* storage, std::size_t idx) const noexcept {
    if (idx == Index) {
        if constexpr(!std::is_trivially_destructible_v<U0>) {
            reinterpret_cast<U0*>(storage)->~U0();
        }
    }
    else {
        if constexpr(sizeof...(U1toN) > 0) {
            destroyer<Index + 1, U1toN...>{}(storage, idx);
        }
    }
}

template <typename... Ts>
template <typename T>
bool variant<Ts...>::holds_alternative() const noexcept {
    if constexpr(std::is_same_v<T,monotype>) {
        return index_ == variant_npos;
    }
    else {
        return index_ == detail::pack_index_v<0, T, Ts...>;
    }
}

template <typename... Ts>
template <std::size_t I>
variant_alternative_t<I, variant<Ts...>>& variant<Ts...>::get() & {
    if(index_ != I) {
        throw bad_variant_access("Invalid access index");
    }
    return *reinterpret_cast<detail::nth_type_t<I, Ts...>*>(storage_);
}

template <typename... Ts>
template <std::size_t I>
variant_alternative_t<I, variant<Ts...>>const & variant<Ts...>::get() const & {
    if(index_ != I) {
        throw bad_variant_access("Invalid access index");
    }
    return *reinterpret_cast<detail::nth_type_t<I, Ts...>*>(storage_);
}

template <typename... Ts>
template <std::size_t I>
variant_alternative_t<I, variant<Ts...>>&& variant<Ts...>::get() && {
    if(index_ != I) {
        throw bad_variant_access("Invalid access index");
    }
    return std::move(*reinterpret_cast<detail::nth_type_t<I, Ts...>*>(storage_));
}

template <typename... Ts>
template <std::size_t I>
variant_alternative_t<I, variant<Ts...>> const&& variant<Ts...>::get() const && {
    if(index_ != I) {
        throw bad_variant_access("Invalid access index");
    }
    return std::move(*reinterpret_cast<detail::nth_type_t<I, Ts...>*>(storage_));
}

template <typename... Ts>
template <typename T>
T& variant<Ts...>::get() & {
    static_assert((std::is_same_v<T, Ts> || ...), "Invalid type");
    if(index_ != detail::pack_index_v<0, T, Ts...>) {
        throw bad_variant_access("Invalid access type");
    }
    return *reinterpret_cast<T*>(storage_);
}

template <typename... Ts>
template <typename T>
T const& variant<Ts...>::get() const & {
    static_assert((std::is_same_v<T, Ts> || ...), "Invalid type");
    if(index_ != detail::pack_index_v<0, T, Ts...>) {
        throw bad_variant_access("Invalid access type");
    }
    return *reinterpret_cast<T*>(storage_);
}

template <typename... Ts>
template <typename T>
T&& variant<Ts...>::get() && {
    static_assert((std::is_same_v<T, Ts> || ...), "Invalid type");
    if(index_ != detail::pack_index_v<0, T, Ts...>) {
        throw bad_variant_access("Invalid access type");
    }
    return std::move(*reinterpret_cast<T*>(storage_));
}

template <typename... Ts>
template <typename T>
T const&& variant<Ts...>::get() const && {
    static_assert((std::is_same_v<T, Ts> || ...), "Invalid type");
    if(index_ != detail::pack_index_v<0, T, Ts...>) {
        throw bad_variant_access("Invalid access type");
    }
    return std::move(*reinterpret_cast<T*>(storage_));
}

template <typename... Ts>
template <std::size_t I>
std::remove_reference_t<variant_alternative_t<I, variant<Ts...>>>*
variant<Ts...>::get_if() noexcept {
    return index_ == I ?  reinterpret_cast<detail::nth_type_t<I, Ts...>*>(storage_) : nullptr;
}

template <typename... Ts>
template <std::size_t I>
std::remove_reference_t<variant_alternative_t<I, variant<Ts...>>> const*
variant<Ts...>::get_if() const noexcept {
    return index_ == I ? reinterpret_cast<detail::nth_type_t<I, Ts...> const*>(storage_) : nullptr;
}

template <typename... Ts>
template <typename T>
std::remove_reference_t<T>* variant<Ts...>::get_if() noexcept {
    static_assert((std::is_same_v<T, Ts> || ...), "Invalid type");
    return index_ == detail::pack_index_v<0, T, Ts...> ?
        reinterpret_cast<T*>(storage_) : nullptr;
}

template <typename... Ts>
template <typename T>
std::remove_reference_t<T> const* variant<Ts...>::get_if() const noexcept {
    static_assert((std::is_same_v<T, Ts> || ...), "Invalid type");
    return index_ == detail::pack_index_v<0, T, Ts...> ?
        reinterpret_cast<T const*>(storage_) : nullptr;
}

template <typename T, typename... Args>
bool holds_alternative(variant<Args...> const& variant) noexcept {
    return variant.template holds_alternative<T>();
}

template <std::size_t I, typename... Ts>
variant_alternative_t<I, variant<Ts...>>& get(variant<Ts...>& variant) {
    return variant.template get<I>();
}

template <std::size_t I, typename... Ts>
variant_alternative_t<I, variant<Ts...>> const& get(variant<Ts...> const& variant) {
    return variant.template get<I>();
}

template <std::size_t I, typename... Ts>
variant_alternative_t<I, variant<Ts...>>&& get(variant<Ts...>&& variant) {
    return variant.template get<I>();
}

template <std::size_t I, typename... Ts>
variant_alternative_t<I, variant<Ts...>> const&& get(variant<Ts...> const&& variant) {
    return variant.template get<I>();
}

template <typename T, typename... Ts>
T& get(variant<Ts...>& variant) {
    return variant.template get<T>();
}

template <typename T, typename... Ts>
T const& get(variant<Ts...> const& variant) {
    return variant.template get<T>();
}

template <typename T, typename... Ts>
T&& get(variant<Ts...>&& variant) {
    return variant.template get<T>();
}

template <typename T, typename... Ts>
T const&& get(variant<Ts...> const&& variant) {
    return variant.template get<T>();
}

template <std::size_t I, typename... Args>
std::remove_reference_t<variant_alternative_t<I, variant<Args...>>>*
get_if(variant<Args...>& variant) noexcept {
    return variant.template get_if<I>();
}

template <std::size_t I, typename... Args>
std::remove_reference_t<variant_alternative_t<I, variant<Args...>>> const*
get_if(variant<Args...> const& variant) noexcept {
    return variant.template get_if<I>();
}

template <typename T, typename... Args>
std::remove_reference_t<T>* get_if(variant<Args...>& variant) noexcept {
    return variant.template get_if<T>();
}

template <typename T, typename... Args>
std::remove_reference_t<T> const* get_if(variant<Args...> const& variant) noexcept {
    return variant.template get_if<T>();
}

#endif /* VARIANT_HPP */

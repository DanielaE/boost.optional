#pragma once
//#if __cplusplus <= 201703L
//#  error insufficient language support!
//#endif
#include <optional>
#include <type_traits>
#include <concepts>
#include <compare>

#define DEPRECATED // [[deprecated]]
#define CONSTEVAL constexpr
#define CONSTINIT inline constexpr

#if !defined(OPTIONAL_THREE_WAY) && defined(__cpp_impl_three_way_comparison)
#define OPTIONAL_THREE_WAY
#endif

#ifndef NS_OPTIONAL
#  define NS_OPTIONAL boost
#endif

namespace boost {
class in_place_factory_base;
class typed_in_place_factory_base;

template<class T>
struct optional_swap_should_use_default_constructor;
} // namespace boost

namespace NS_OPTIONAL {
namespace optional_ns {
struct in_place_init_t : std::in_place_t {
	constexpr explicit in_place_init_t(std::in_place_t) {}
};
inline constexpr in_place_init_t in_place_init{ std::in_place };

struct in_place_init_if_t : std::in_place_t {
	constexpr explicit in_place_init_if_t(std::in_place_t) {}
};
inline constexpr in_place_init_if_t in_place_init_if{ std::in_place };
} // namespace optional_ns

using optional_ns::in_place_init;
using optional_ns::in_place_init_if;
using optional_ns::in_place_init_if_t;
using optional_ns::in_place_init_t;

using bad_optional_access = std::bad_optional_access;

#ifndef OPTIONAL_NONE_T_CPP20
#define OPTIONAL_NONE_T_CPP20
struct none_t : std::nullopt_t {
	constexpr none_t(std::nullopt_t) : std::nullopt_t{std::nullopt} {}
};
#endif
#ifndef OPTIONAL_NONE_CPP20
#define OPTIONAL_NONE_CPP20
inline constexpr none_t none{ std::nullopt };
#endif

namespace {
template <typename T>
using base_optional = std::optional<T>;

template <typename T>
CONSTINIT bool dependent_false = false;

CONSTEVAL std::false_type optional_tag(...);
template <typename T>
CONSTEVAL std::true_type optional_tag(const volatile std::optional<T> *);

template <typename T>
concept optional_type = decltype(optional_tag((std::remove_reference_t<T> *)nullptr))::value;
template <typename T>
concept not_optional = !optional_type<T>;
template <typename T>
concept nullopt_type = std::is_base_of_v<std::nullopt_t, std::remove_cvref_t<T>>
template <typename T>
concept optional_related = nullopt_type<T> || optional_type<T>

template <typename T>
using unwrap_t = std::conditional_t<optional_type<T>,
	typename std::remove_reference_t<T>::value_type,
	typename std::remove_reference_t<T>>;

template <class T, class U>
concept constructible =
	std::is_same_v<T, std::remove_reference_t<U>> ||
	std::is_same_v<T, const std::remove_reference_t<U>>;

template <typename T>
concept inplace_factory_type =
	std::is_base_of_v<in_place_factory_base, std::decay_t<T>> ||
	std::is_base_of_v<typed_in_place_factory_base, std::decay_t<T>>;

template <typename T>
concept _bool_testable = std::convertible_to<T, bool>;
template <typename T>
concept bool_testable = _bool_testable<T> && requires(T && t) {
	{ !static_cast<T &&>(t) } -> _bool_testable;
};

template <typename T, typename U>
concept eq_comparable =
	requires(const std::remove_cvref_t<T> & lhs, const std::remove_cvref_t<U> & rhs) {
		{ lhs == rhs } -> bool_testable;
	};

template <typename T, typename U>
concept ne_comparable =
	requires(const std::remove_cvref_t<T> & lhs, const std::remove_cvref_t<U> & rhs) {
		{ lhs != rhs } -> bool_testable;
	};

template <typename T, typename U>
concept lt_comparable =
	requires(const std::remove_cvref_t<T> & lhs, const std::remove_cvref_t<U> & rhs) {
		{ lhs < rhs } -> bool_testable;
	};

template <typename T, typename U>
concept gt_comparable =
	requires(const std::remove_cvref_t<T> & lhs, const std::remove_cvref_t<U> & rhs) {
		{ lhs > rhs } -> bool_testable;
	};

template <typename T, typename U>
concept le_comparable =
	requires(const std::remove_cvref_t<T> & lhs, const std::remove_cvref_t<U> & rhs) {
		{ lhs <= rhs } -> bool_testable;
	};

template <typename T, typename U>
concept ge_comparable =
	requires(const std::remove_cvref_t<T> & lhs, const std::remove_cvref_t<U> & rhs) {
		{ lhs >= rhs } -> bool_testable;
	};

template <typename T, typename U>
concept tw_comparable =
	requires(const std::remove_cvref_t<T> & lhs, const std::remove_cvref_t<U> & rhs) {
		{ (lhs <=> rhs) == 0 } -> bool_testable;
	};

} // anonymous namespace

template <typename T>
class optional : public base_optional<T> {
	using base = base_optional<T>;

	template <typename Factory>
	void construct_at(void * storage, Factory && f) {
		if constexpr (std::is_convertible_v<Factory *, ::boost::in_place_factory_base *>)
			f.template apply<T>(storage);
		else
			f.apply(storage);
	}

	template <typename Factory>
	T make_from(Factory && f) {
		alignas(T) char storage[sizeof(T)];
		construct_at(storage, static_cast<Factory &&>(f));
		return static_cast<T &&>(*std::launder(reinterpret_cast<T*>(storage)));
	}

	template <typename Factory>
	void replace_from(Factory && f) {
		T * pstorage = this->operator->();
		pstorage->~T();
		construct_at(pstorage, static_cast<Factory &&>(f));
	}

public:
	// construction
//	using base::optional;
	[[nodiscard]] constexpr optional() noexcept = default;
//	[[nodiscard]] constexpr optional(boost::none_t) noexcept {};
	[[nodiscard]] constexpr optional(std::nullopt_t) noexcept {};
	[[nodiscard]] constexpr optional(const T & other) : base(other) {}
	[[nodiscard]] constexpr optional(T && other) : base(static_cast<T &&>(other)) {}
	template <not_optional_related U>
		requires (!std::is_same_v<T, std::decay_t<U>> && !inplace_factory_type<U>)
	[[nodiscard]] constexpr optional(U && rhs) : base(static_cast<U &&>(rhs)) {}
    template <typename U>
	[[nodiscard]] constexpr optional(const optional<U &> & rhs) : base(rhs ? base{*rhs} : base{}) {}

	// assignment
//	optional & operator=(boost::none_t) noexcept {
//		return static_cast<optional &>(base::operator=(std::nullopt));
//	}
	optional & operator=(std::nullopt_t) noexcept {
		return static_cast<optional &>(base::operator=(std::nullopt));
	}

	template <typename U = T>
		requires (!std::is_same_v<optional, std::remove_cvref_t<U>> && 
		          !nullopt_type<U> && 
		          !(std::is_scalar_v<T> && std::is_same_v<T, std::decay_t<U>>) && 
		          std::is_constructible_v<T, U> && std::is_assignable_v<T&, U>)
	optional & operator=(U && rhs) {
		return static_cast<optional &>(base::operator=(static_cast<U &&>(rhs)));
	}

	template <typename U>
		requires (!std::is_reference_v<U> && std::is_assignable_v<base_optional<T>, const base_optional<U> &>)
	optional & operator=(const base_optional<U> & rhs) {
		return static_cast<optional &>(
			base::operator=(rhs));
	}

	template <typename U>
		requires (!std::is_reference_v<U> && std::is_assignable_v<base_optional<T>, base_optional<U> &&>)
	optional & operator=(base_optional<U> && rhs) {
		return static_cast<optional &>(
			base::operator=(static_cast<base_optional<U> &&>(rhs)));
	}

	template <typename U>
		requires requires(base lhs, const U & rhs) { lhs = rhs; }
	optional & operator=(const optional<U &> & rhs) {
		return static_cast<optional &>(rhs ? base::operator=(*rhs) : base::operator=(std::nullopt));
	}

	template <typename U>
		requires requires(base lhs, const U & rhs) { lhs = rhs; }
	optional & operator=(optional<U &> && rhs) {
	    return static_cast<optional &>( rhs ? base::operator=(*rhs) : base::operator=(std::nullopt));
	}

	// conversion from base
	[[nodiscard]] constexpr optional(const base & from) : base(from) {}
	[[nodiscard]] constexpr optional(base && from) noexcept : base(static_cast<base &&>(from)) {}

	// non-standard additional Boost interfaces

	using value_type           = T;
	using reference_type       = T &;
	using reference_const_type = const T &;
	using rval_reference_type  = T &&;
	using pointer_type         = T *;
	using pointer_const_type   = const T *;
	using argument_type        = T const &;

	// construction
	constexpr optional(bool condition, const T & other)
	: base{ condition ? base(other) : base{} } {}
	constexpr optional(bool condition, T && other)
	: base{ condition ? base(static_cast<T &&>(other)) : base{} } {}

	template <class... Args>
	constexpr optional(in_place_init_if_t, bool condition, Args &&... args)
	: base{ condition ? base(std::in_place, static_cast<Args &&>(args)...) : base{} } {}

	template <inplace_factory_type Factory>
		requires std::is_default_constructible_v<T>
	explicit optional(Factory && f)
	: base(std::in_place) {
		replace_from(static_cast<Factory &&>(f));
	}
	template <inplace_factory_type Factory>
		requires (!std::is_default_constructible_v<T>)
	explicit optional(Factory && f)
	: base(std::in_place, make_from(static_cast<Factory &&>(f))) {}

	// assignment
	template <inplace_factory_type Factory>
	optional<T> & operator=(Factory && f) {
		if (*this) {
			replace_from(static_cast<Factory &&>(f));
		} else {
			if constexpr (std::is_default_constructible_v<T>) {
				this->emplace();
				replace_from(static_cast<Factory &&>(f));
			} else {
				this->emplace(make_from(static_cast<Factory &&>(f)));
			}
		}
		return *this;
	}

	// observers
	[[nodiscard]] constexpr const T & get() const { return **this; }
	[[nodiscard]] constexpr T & get() { return **this; }

	[[nodiscard]] constexpr const T * get_ptr() const {
		return this->has_value() ? std::addressof(**this) : nullptr;
	}
	[[nodiscard]] constexpr T * get_ptr() {
		return this->has_value() ? std::addressof(**this) : nullptr;
	}

	[[nodiscard]] constexpr bool operator!() const noexcept {
		return !static_cast<bool>(*this);
	}

	template <typename Func>
	[[nodiscard]] constexpr optional<std::invoke_result_t<Func, T &>>
	map(Func f) & {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr optional<std::invoke_result_t<Func, const T &>>
	map(Func f) const & {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr optional<std::invoke_result_t<Func, T &&>>
	map(Func f) && {
		if (this->has_value())
			return f(static_cast<T &&>(**this));
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr
		optional<unwrap_t<std::invoke_result_t<Func, T &>>>
	flat_map(Func f) & {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr
		optional<unwrap_t<std::invoke_result_t<Func, const T &>>>
	flat_map(Func f) const & {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr
		optional<unwrap_t<std::invoke_result_t<Func, T &&>>>
	flat_map(Func f) && {
		if (this->has_value())
			return f(static_cast<T &&>(**this));
		return none;
	}

	[[nodiscard]] constexpr const T & get_value_or(const T & replacement) const {
		return this->has_value() ? **this : replacement;
	}
	[[nodiscard]] constexpr T & get_value_or(T & replacement) {
		return this->has_value() ? **this : replacement;
	}

	template <class Func>
	[[nodiscard]] constexpr T value_or_eval(Func f) const & {
		return this->has_value()? **this : f();
	}
	template <class Func>
	[[nodiscard]] constexpr T value_or_eval(Func f) && {
		return this->has_value()? static_cast<T &&>(**this) : f();
	}

	// modifiers
	constexpr void reset() { base::reset(); }

	// deprecated
	// observers
	DEPRECATED [[nodiscard]] constexpr bool is_initialized() const noexcept {
		return this->has_value();
	}
	// modifiers
	DEPRECATED constexpr void reset(T const & rhs) { *this = rhs; }
};

template <typename T>
class optional<T &> {
	template <typename U>
	friend CONSTEVAL std::true_type optional_tag(const volatile optional<U &> *);

	template <typename U>
	struct taint_rvalue {
		[[maybe_unused]] constexpr taint_rvalue() = default;
		static_assert(std::is_lvalue_reference_v<U>, "binding rvalues is ill-formed");
	};

	template <typename U>
	constexpr decltype(auto) forward_reference(U && r) {
		taint_rvalue<U>{};
		return static_cast<U &&>(r);
	}

	T * p_ = nullptr;

public:
	// construction
	[[nodiscard]] constexpr optional() noexcept = default;
//	[[nodiscard]] constexpr optional(none_t) noexcept {}
	[[nodiscard]] constexpr optional(std::nullopt_t) noexcept {}
	[[nodiscard]] constexpr optional(const optional & other) noexcept = default;
	[[nodiscard]] constexpr optional(optional && other) noexcept      = default;
	optional(T &&) { taint_rvalue<T>{}; }

	template <class U>
	[[nodiscard]] constexpr explicit optional(const optional<U &> & other) noexcept : p_(other.get_ptr()) {}

	template <typename U>
		requires constructible<T, U>
	[[nodiscard]] constexpr optional(U & other) noexcept : p_(std::addressof(other)) {}

	template <typename U>
		requires(not_optional<U> && !constructible<T, U>)
	[[nodiscard]] constexpr optional(U && other) noexcept : p_(std::addressof(other)) {
		taint_rvalue<U>{};
	}

	// assignment
	constexpr optional & operator=(const optional & rhs) noexcept = default;
	constexpr optional & operator=(optional && rhs) noexcept = default;

//	constexpr optional & operator=(boost::none_t) noexcept {
//		p_ = nullptr;
//		return *this;
//	}
	constexpr optional & operator=(std::nullopt_t) noexcept {
		p_ = nullptr;
		return *this;
	}

	template <typename U>
	constexpr optional & operator=(const optional<U &> & rhs) noexcept {
		p_ = rhs.get_ptr();
		return *this;
	}

	template <not_optional_related U>
	constexpr optional & operator=(U && rhs) noexcept {
		taint_rvalue<U>{};
		p_ = std::addressof(rhs);
		return *this;
	}

	// observers
	constexpr T * operator->() const noexcept { return p_; }
	constexpr T & operator*() const { return *p_; }
	[[nodiscard]] constexpr T & value() const {
		return p_ ? *p_ : throw bad_optional_access();
	}
	[[nodiscard]] constexpr bool has_value() const noexcept { return p_ != nullptr; }
	constexpr explicit operator bool() const noexcept { return p_ != nullptr; }

	// modifiers
	constexpr void swap(optional & rhs) noexcept { std::swap(p_, rhs.p_); }

	constexpr void reset() noexcept { p_ = nullptr; }

	template <not_optional_related U>
	constexpr void emplace(U && rhs) noexcept {
		p_ = std::addressof(rhs);
		taint_rvalue<U>{};
	}

	// non-standard additional Boost interfaces

	using value_type           = T &;
	using reference_type       = T &;
	using reference_const_type = T &;
	using rval_reference_type  = T &;
	using pointer_type         = T *;
	using pointer_const_type   = T *;

	// construction
	template <not_optional_related U>
	constexpr optional(bool condition, U && rhs) noexcept
	: p_(condition ? std::addressof(rhs) : nullptr) {
		taint_rvalue<U>{};
	}

	// observers
	[[nodiscard]] constexpr T & get() const { return *p_; }
	[[nodiscard]] constexpr T * get_ptr() const noexcept { return p_; }
	constexpr bool operator!() const noexcept { return p_ == nullptr; }

	template <typename Func>
	[[nodiscard]] optional<std::invoke_result_t<Func, T &>>
	map(Func f) const {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename Func>
	[[nodiscard]] optional<unwrap_t<std::invoke_result_t<Func, T &>>>
	flat_map(Func f) const {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename U>
		requires not_optional<U>
	[[nodiscard]] constexpr T & value_or(U && replacement) const noexcept {
		taint_rvalue<U>{};
		return p_ ? *p_ : replacement;
	}

	template <class Func>
	[[nodiscard]] T & value_or_eval(Func f) const {
		taint_rvalue<std::invoke_result_t<Func>>{};
		return p_ ? *p_ : f();
	}

	// deprecated
	// observers
	DEPRECATED [[nodiscard]] constexpr bool is_initialized() const noexcept {
		return p_ != nullptr;
	}

	template <typename U>
		requires not_optional<U>
	DEPRECATED [[nodiscard]] constexpr T &
	get_value_or(U && replacement) const noexcept {
		taint_rvalue<U>{};
		return p_ ? *p_ : replacement;
	}

	// modifiers
	template <typename U>
		requires not_optional<U>
	DEPRECATED constexpr void reset(U && rhs) noexcept {
		taint_rvalue<U>{};
		p_ = std::addressof(rhs);
	}
};

template <typename T, typename U>
constexpr bool eq_v(const T & lhs, const U & rhs) {
	if constexpr (eq_comparable<T, U>) {
		return lhs == rhs;
	} else if constexpr (ne_comparable<T, U>) {
		return !(lhs != rhs);
	} else {
	    static_assert(dependent_false<T>, "invalid expression lhs == rhs");
	}
}

template <typename T, typename U>
constexpr bool ne_v(const T & lhs, const U & rhs) {
	if constexpr (ne_comparable<T, U>) {
		return lhs != rhs;
	} else if constexpr (eq_comparable<T, U>) {
		return !(lhs == rhs);
	} else {
	    static_assert(dependent_false<T>, "invalid expression lhs != rhs");
	}
}

template <typename T, typename U>
constexpr bool lt_v(const T & lhs, const U & rhs) {
	if constexpr (lt_comparable<T, U>) {
		return lhs < rhs;
	} else {
	    static_assert(dependent_false<T>, "invalid expression lhs < rhs");
	}
}

template <typename T, typename U>
constexpr bool gt_v(const T & lhs, const U & rhs) {
	if constexpr (gt_comparable<T, U>) {
		return lhs > rhs;
	} else if constexpr (lt_comparable<U, T>) {
		return rhs < lhs;
	} else {
	    static_assert(dependent_false<T>, "invalid expression lhs > rhs");
	}
}

template <typename T, typename U>
constexpr bool le_v(const T & lhs, const U & rhs) {
	if constexpr (le_comparable<T, U>) {
		return lhs <= rhs;
	} else if constexpr (lt_comparable<U, T>) {
		return !(rhs < lhs);
	} else {
	    static_assert(dependent_false<T>, "invalid expression lhs <= rhs");
	}
}

template <typename T, typename U>
constexpr bool ge_v(const T & lhs, const U & rhs) {
	if constexpr (ge_comparable<T, U>) {
		return lhs >= rhs;
	} else if constexpr (lt_comparable<T, U>) {
		return !(lhs < rhs);
	} else {
	    static_assert(dependent_false<T>, "invalid expression lhs >= rhs");
	}
}

// [optional.relops]
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(const optional<T> & lhs, const optional<U> & rhs) {
	const bool lhv = lhs.has_value();
	return lhv == rhs.has_value() && (!lhv || eq_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(const optional<T> & lhs, const std::optional<U> & rhs) {
	const bool lhv = lhs.has_value();
	return lhv == rhs.has_value() && (!lhv || eq_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(const std::optional<T> & lhs, const optional<U> & rhs) {
	const bool lhv = lhs.has_value();
	return lhv == rhs.has_value() && (!lhv || eq_v(*lhs, *rhs));
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator!=(const optional<T> & lhs, const optional<U> & rhs) {
	const bool lhv = lhs.has_value();
	return lhv != rhs.has_value() || (lhv && ne_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator!=(const optional<T> & lhs, const std::optional<U> & rhs) {
	const bool lhv = lhs.has_value();
	return lhv != rhs.has_value() || (lhv && ne_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator!=(const std::optional<T> & lhs, const optional<U> & rhs) {
	const bool lhv = lhs.has_value();
	return lhv != rhs.has_value() || (lhv && ne_v(*lhs, *rhs));
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator<(const optional<T> & lhs, const optional<U> & rhs) {
	return rhs.has_value() && (!lhs.has_value() || lt_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator<(const optional<T> & lhs, const std::optional<U> & rhs) {
	return rhs.has_value() && (!lhs.has_value() || lt_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator<(const std::optional<T> & lhs, const optional<U> & rhs) {
	return rhs.has_value() && (!lhs.has_value() || lt_v(*lhs, *rhs));
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator>(const optional<T> & lhs, const optional<U> & rhs) {
	return lhs.has_value() && (!rhs.has_value() || gt_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator>(const optional<T> & lhs, const std::optional<U> & rhs) {
	return lhs.has_value() && (!rhs.has_value() || gt_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator>(const std::optional<T> & lhs, const optional<U> & rhs) {
	return lhs.has_value() && (!rhs.has_value() || gt_v(*lhs, *rhs));
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator<=(const optional<T> & lhs, const optional<U> & rhs) {
	return !lhs.has_value() || (rhs.has_value() && le_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator<=(const optional<T> & lhs, const std::optional<U> & rhs) {
	return !lhs.has_value() || (rhs.has_value() && le_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator<=(const std::optional<T> & lhs, const optional<U> & rhs) {
	return !lhs.has_value() || (rhs.has_value() && le_v(*lhs, *rhs));
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator>=(const optional<T> & lhs, const optional<U> & rhs) {
	return !rhs.has_value() || (lhs.has_value() && ge_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator>=(const optional<T> & lhs, const std::optional<U> & rhs) {
	return !rhs.has_value() || (lhs.has_value() && ge_v(*lhs, *rhs));
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator>=(const std::optional<T> & lhs, const optional<U> & rhs) {
	return !rhs.has_value() || (lhs.has_value() && ge_v(*lhs, *rhs));
}

#ifdef OPTIONAL_THREE_WAY
template <typename T, tw_comparable<T> U>
[[nodiscard]] constexpr std::compare_three_way_result_t<T, U> operator<=>(const optional<T> & lhs, const optional<U> & rhs) {
	const bool lhv = lhs.has_value();
	const bool rhv = rhs.has_value();
	return lhs && rhv ? *lhs <=> *rhs : lhv <=> rhv;
}
template <typename T, tw_comparable<T> U>
[[nodiscard]] constexpr std::compare_three_way_result_t<T, U> operator<=>(const optional<T> & lhs, const std::optional<U> & rhs) {
	const bool lhv = lhs.has_value();
	const bool rhv = rhs.has_value();
	return lhs && rhv ? *lhs <=> *rhs : lhv <=> rhv;
}
#endif

// [optional.nullops]
template <typename T>
[[nodiscard]] constexpr bool operator==(const optional<T> & o, std::nullopt_t) noexcept {
	return !o.has_value();
}
template <typename T>
[[nodiscard]] constexpr bool operator==(std::nullopt_t, const optional<T> & o) noexcept {
	return !o.has_value();
}

template <typename T>
[[nodiscard]] constexpr bool operator!=(const optional<T> & o, std::nullopt_t) noexcept {
	return o.has_value();
}
template <typename T>
[[nodiscard]] constexpr bool operator!=(std::nullopt_t, const optional<T> & o) noexcept {
	return o.has_value();
}

template <typename T>
[[nodiscard]] constexpr bool operator<(const optional<T> &, std::nullopt_t) noexcept {
	return false;
}
template <typename T>
[[nodiscard]] constexpr bool operator<(std::nullopt_t, const optional<T> & o) noexcept {
	return o.has_value();
}

template <typename T>
[[nodiscard]] constexpr bool operator>(const optional<T> & o, std::nullopt_t) noexcept {
	return o.has_value();
}
template <typename T>
[[nodiscard]] constexpr bool operator>(std::nullopt_t, const optional<T> &) noexcept {
	return false;
}

template <typename T>
[[nodiscard]] constexpr bool operator<=(const optional<T> & o, std::nullopt_t) noexcept {
	return !o.has_value();
}
template <typename T>
[[nodiscard]] constexpr bool operator<=(std::nullopt_t, const optional<T> &) noexcept {
	return true;
}

template <typename T>
[[nodiscard]] constexpr bool operator>=(const optional<T> &, std::nullopt_t) noexcept {
	return true;
}
template <typename T>
[[nodiscard]] constexpr bool operator>=(std::nullopt_t, const optional<T> & o) noexcept {
	return !o.has_value();
}

template <typename T>
[[nodiscard]] constexpr std::strong_ordering operator<=>(const optional<T> & o, std::nullopt_t) noexcept {
	return o.has_value() <=> false;
}

// [optional.comp_with_t]

template <typename T, not_optional_related U>
	requires eq_comparable<T, U> || ne_comparable<T, U>
[[nodiscard]] constexpr bool operator==(const optional<T> & lhs, const U & rhs) {
	return lhs.has_value() && eq_v(*lhs, rhs);
}
template <not_optional_related U, typename T>
	requires eq_comparable<U, T> || ne_comparable<U, T>
[[nodiscard]] constexpr bool operator==(const U & lhs, const optional<T> & rhs) {
	return rhs.has_value() && eq_v(lhs, *rhs);
}

template <typename T, not_optional_related U>
	requires ne_comparable<T, U> || eq_comparable<T, U>
[[nodiscard]] constexpr bool operator!=(const optional<T> & lhs, const U & rhs) {
	return !lhs.has_value() || ne_v(*lhs, rhs);
}
template <not_optional_related U, typename T>
	requires ne_comparable<U, T> || eq_comparable<U, T>
[[nodiscard]] constexpr bool operator!=(const U & lhs, const optional<T> & rhs) {
	return !rhs.has_value() || ne_v(lhs, *rhs);
}

template <typename T, not_optional_related U>
	requires lt_comparable<T, U>
[[nodiscard]] constexpr bool operator<(const optional<T> & lhs, const U & rhs) {
	return !lhs.has_value() || lt_v(*lhs, rhs);
}
template <not_optional_related U, typename T>
	requires lt_comparable<U, T>
[[nodiscard]] constexpr bool operator<(const U & lhs, const optional<T> & rhs) {
	return rhs.has_value() && lt_v(lhs, *rhs);
}

template <typename T, not_optional_related U>
	requires gt_comparable<T, U> || lt_comparable<U, T>
[[nodiscard]] constexpr bool operator>(const optional<T> & lhs, const U & rhs) {
	return lhs.has_value() && gt_v(*lhs, rhs);
}
template <not_optional_related U, typename T>
	requires gt_comparable<U, T> || lt_comparable<T, U>
[[nodiscard]] constexpr bool operator>(const U & lhs, const optional<T> & rhs) {
	return !rhs.has_value() || gt_v(lhs, *rhs);
}

template <typename T, not_optional_related U>
	requires le_comparable<T, U> || lt_comparable<U, T>
[[nodiscard]] constexpr bool operator<=(const optional<T> & lhs, const U & rhs) {
	return !lhs.has_value() || le_v(*lhs, rhs);
}
template <not_optional_related U, typename T>
	requires le_comparable<U, T> || lt_comparable<T, U>
[[nodiscard]] constexpr bool operator<=(const U & lhs, const optional<T> & rhs) {
	return rhs.has_value() && le_v(lhs, *rhs);
}

template <typename T, not_optional_related U>
	requires ge_comparable<T, U> || lt_comparable<T, U>
[[nodiscard]] constexpr bool operator>=(const optional<T> & lhs, const U & rhs) {
	return lhs.has_value() && ge_v(*lhs, rhs);
}
template <not_optional_related U, typename T>
	requires ge_comparable<U, T> || lt_comparable<U, T>
[[nodiscard]] constexpr bool operator>=(const U & lhs, const optional<T> & rhs) {
	return !rhs.has_value() || ge_v(lhs, *rhs);
}

#ifdef OPTIONAL_THREE_WAY
template <typename T, tw_comparable<T> U>
[[nodiscard]] constexpr std::compare_three_way_result_t<T, U> operator<=>(const optional<T> & lhs, const U & rhs) {
	return lhs.has_value() ? *lhs <=> rhs : std::strong_ordering::less;
}
#endif

// [optional.specalg]
template <typename T>
constexpr void swap(optional<T &> & lhs, optional<T &> & rhs) noexcept {
	lhs.swap(rhs);
}

template <not_optional T>
[[nodiscard]] constexpr optional<std::decay_t<T>> make_optional(T && value) {
	return optional<std::decay_t<T>>{ std::forward<T>(value) };
}
template <optional_type T>
[[nodiscard]] constexpr optional<std::decay_t<T>> make_optional(T && value) {
	return optional<std::decay_t<T>>{ std::forward<T>(value) };
}
template <typename T, typename... Args>
[[nodiscard]] constexpr optional<T> make_optional(Args &&... args) {
	return optional<T>{ std::in_place, std::forward<Args>(args)... };
}
template <typename T, typename E, typename... Args>
[[nodiscard]] constexpr optional<T>
make_optional(std::initializer_list<E> list, Args &&... args) {
	return optional<T>{ std::in_place, list, std::forward<Args>(args)... };
}

template <class T>
optional(T) -> optional<T>;

// non-standard additional Boost interfaces

namespace {
template <typename T>
using reference_t = typename optional<T>::reference_type;
template <typename T>
using const_reference_t = typename optional<T>::reference_const_type;
template <typename T>
using pointer_t = typename optional<T>::pointer_type;
template <typename T>
using const_pointer_t = typename optional<T>::pointer_const_type;
}

template <typename T>
[[nodiscard]] constexpr optional<std::decay_t<T>>
make_optional(bool condition, T && v) {
	return optional<std::decay_t<T>>(condition, std::forward<T>(v));
}

template <typename T>
constexpr const_reference_t<T> get(const optional<T> & o) {
	return o.get();
}

template <typename T>
constexpr reference_t<T> get(optional<T> & o) {
	return o.get();
}

template <typename T>
constexpr const_pointer_t<T> get(const optional<T> * o) {
	return o->get_ptr();
}

template <typename T>
constexpr pointer_t<T> get(optional<T> * o) {
	return o->get_ptr();
}

template <typename T>
constexpr const_reference_t<T>
get_optional_value_or(const optional<T> & o, const_reference_t<T> v) {
	return o.get_value_or(v);
}

template <typename T>
constexpr reference_t<T>
get_optional_value_or(optional<T> & o, reference_t<T> v) {
	return o.get_value_or(v);
}

template <typename T>
constexpr const_pointer_t<T> get_pointer(const optional<T> & o) {
	return o.get_ptr();
}

template <typename T>
constexpr pointer_t<T> get_pointer(optional<T> & o) {
	return o.get_ptr();
}
} // namespace NS_OPTIONAL

// [optional.hash]
namespace std {
template <typename T>
struct hash<::NS_OPTIONAL::optional<T>> : hash<::NS_OPTIONAL::base_optional<T>> {
	using argument_type = ::NS_OPTIONAL::optional<T>;
	using base          = hash<::NS_OPTIONAL::base_optional<T>>;
	using base::base;
	std::size_t operator()(const argument_type & o) const noexcept {
		return base::operator()(o);
	}
};

template <typename T>
struct hash<::NS_OPTIONAL::optional<T &>> : hash<T> {
	using argument_type = ::NS_OPTIONAL::optional<T &>;
	using base          = hash<T>;
	using base::base;
	std::size_t operator()(const argument_type & o) const noexcept {
		return o.has_value() ? base::operator()(*o) : 0;
	}
};
} // namespace std

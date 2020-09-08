#pragma once
//#if __cplusplus <= 201703L
//#  error insufficient language support!
//#endif

#include <optional>
#include <type_traits>

#define DEPRECATED

#ifndef NS_OPTIONAL
#  define NS_OPTIONAL boost
#endif

namespace boost {
class in_place_factory_base;
class typed_in_place_factory_base;
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
using none_t              = std::nullopt_t;

#ifndef OPTIONAL_NONE_CPP20
#define OPTIONAL_NONE_CPP20
inline constexpr none_t none{ std::nullopt };
#endif

template <typename T>
class optional;

template <class T>
optional(T) -> optional<T>;

template<class T>
struct optional_swap_should_use_default_constructor;

namespace detail {
template <typename T>
using base = std::optional<T>;

template <typename T>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional<optional<T>> : std::true_type {
	typedef T value_type;
};
template <typename T>
struct is_optional<base<T>> : std::true_type {
	typedef T value_type;
};

template <typename T>
using optional_value_type = typename is_optional<T>::value_type;

template <typename T>
concept not_optional = !is_optional<std::decay_t<T>>::value;

template <class T, class U>
concept constructible =
	std::is_same_v<T, std::remove_reference_t<U>> ||
	std::is_same_v<T, const std::remove_reference_t<U>>;

template <typename T>
concept is_inplace_factory =
	std::is_base_of_v<in_place_factory_base, std::decay_t<T>> ||
	std::is_base_of_v<typed_in_place_factory_base, std::decay_t<T>>;

template <typename T>
struct taint_rvalue {
	[[maybe_unused]] taint_rvalue() = default;
	static_assert(std::is_lvalue_reference_v<T>, "binding rvalues is ill-formed");
};

template <typename T>
std::remove_reference_t<T> & forward_reference(T && r) {
	taint_rvalue<T>{};
	return std::forward<T>(r);
}

template <typename T>
using reference_t = typename optional<T>::reference_type;
template <typename T>
using const_reference_t = typename optional<T>::reference_const_type;
template <typename T>
using pointer_t = typename optional<T>::pointer_type;
template <typename T>
using const_pointer_t = typename optional<T>::pointer_const_type;
} // namespace detail

template <typename T>
class optional : public detail::base<T> {
	using base = detail::base<T>;

	template <typename Factory>
	struct construct_by {
		construct_by(Factory && f) : f_{ std::addressof(f) } {}
		operator T() const noexcept {
			union {
				char _;
				std::remove_const_t<T> t;
			} storage{};
			if constexpr (std::is_convertible_v<Factory *, ::boost::in_place_factory_base *>)
				f_->template apply<T>(std::addressof(storage.t));
			else
				f_->apply(std::addressof(storage.t));
			return storage.t;
		}
		Factory * f_;
	};

public:
	// construction
//	using base::optional;
	constexpr optional() noexcept = default;
	constexpr optional(const T & other) : base(other) {}
	constexpr optional(T && other) : base(std::forward<T>(other)) {}
	template <typename U>
		requires (!std::is_same_v<T, std::decay_t<U>>)
	constexpr optional(U && rhs) : base(std::forward<U>(rhs)) {}

	// assignment
	optional & operator=(std::nullopt_t) noexcept {
		return static_cast<optional &>(base::operator=(std::nullopt));
	}

	template <typename U = T>
		requires requires(U && rhs) { base::operator=(std::forward<U>(rhs)); }
	optional & operator=(U && rhs) {
		return static_cast<optional &>(base::operator=(std::forward<U>(rhs)));
	}

	template <typename U>
		requires requires(const detail::base<U> & rhs) { base::operator=(rhs); }
	optional & operator=(const optional<U> & rhs) {
		return static_cast<optional &>(
			base::operator=(static_cast<const detail::base<U> &>(rhs)));
	}

	template <typename U>
		requires requires(detail::base<U> && rhs) { base::operator=(std::move(rhs)); }
	optional & operator=(optional<U> && rhs) {
		return static_cast<optional &>(
			base::operator=(static_cast<detail::base<U> &&>(rhs)));
	}

	// conversion from base
	constexpr optional(const base & from) : base(from) {}
	constexpr optional(base && from) noexcept : base(std::move(from)) {}

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
	: base{ condition ? base(std::forward<T>(other)) : base{} } {}

	template <class... Args>
	constexpr optional(in_place_init_if_t, bool condition, Args &&... args)
	: base{ condition ? base(std::in_place, std::forward<Args>(args)...) : base{} } {}

	template <typename Factory>
		requires detail::is_inplace_factory<Factory>
	explicit optional(Factory && f)
	: base(std::in_place, construct_by(std::forward<Factory>(f))) {}

	// assignment
	template <typename Factory>
		requires detail::is_inplace_factory<Factory>
	optional<T> & operator=(Factory && f) {
		this->emplace(construct_by(std::forward<Factory>(f)));
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
			return f(std::move(**this));
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr
		optional<detail::optional_value_type<std::invoke_result_t<Func, T &>>>
	flat_map(Func f) & {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr
		optional<detail::optional_value_type<std::invoke_result_t<Func, const T &>>>
	flat_map(Func f) const & {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr
		optional<detail::optional_value_type<std::invoke_result_t<Func, T &&>>>
	flat_map(Func f) && {
		if (this->has_value())
			return f(std::move(**this));
		return none;
	}

	[[nodiscard]] constexpr const T & get_value_or(const T & replacement) const {
		return this->has_value() ? **this : replacement;
	}
	[[nodiscard]] constexpr T & get_value_or(T & replacement) {
		return this->has_value() ? **this : replacement;
	}

	template <class Func>
	[[nodiscard]] constexpr T & value_or_eval(Func f) const & {
		return this->has_value()? **this : f();
	}
	template <class Func>
	[[nodiscard]] constexpr T & value_or_eval(Func f) && {
		return this->has_value()? std::move(**this) : f();
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
	T * p_ = nullptr;

public:
	// construction
	constexpr optional() noexcept = default;
	constexpr optional(std::nullopt_t) noexcept {}
	constexpr optional(const optional & other) noexcept = default;
	constexpr optional(optional && other) noexcept      = default;
	optional(T &&) { detail::taint_rvalue<T>{}; }

	template <class U>
	constexpr explicit optional(const optional<U &> & other) noexcept : p_(other.p_) {}

	template <typename U>
		requires detail::constructible<T, U>
	constexpr optional(U & other) noexcept : p_(std::addressof(other)) {}

	template <typename U>
		requires(detail::not_optional<U> && !detail::constructible<T, U>)
	constexpr optional(U && other) noexcept : p_(std::addressof(other)) {
		detail::taint_rvalue<U>{};
	}

	// assignment
	constexpr optional & operator=(const optional & rhs) noexcept = default;
	constexpr optional & operator=(optional && rhs) noexcept = default;

	constexpr optional & operator=(std::nullopt_t) noexcept {
		p_ = nullptr;
		return *this;
	}

	template <typename U>
	constexpr optional & operator=(const optional<U &> & rhs) noexcept {
		p_ = rhs.p_;
		return *this;
	}

	template <typename U>
		requires detail::not_optional<U>
	constexpr optional & operator=(U && rhs) noexcept {
		detail::taint_rvalue<U>{};
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

	template <typename U>
		requires detail::not_optional<U>
	constexpr void emplace(U && rhs) noexcept {
		p_ = std::addressof(rhs);
		detail::taint_rvalue<U>{};
	}

	// non-standard additional Boost interfaces

	using value_type           = T &;
	using reference_type       = T &;
	using reference_const_type = T &;
	using rval_reference_type  = T &;
	using pointer_type         = T *;
	using pointer_const_type   = T *;

	// construction
	template <typename U>
		requires detail::not_optional<U>
	constexpr optional(bool condition, U && rhs) noexcept
	: p_(condition ? std::addressof(rhs) : nullptr) {
		detail::taint_rvalue<U>{};
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
	[[nodiscard]] optional<detail::optional_value_type<std::invoke_result_t<Func, T &>>>
	flat_map(Func f) const {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename U>
		requires detail::not_optional<U>
	[[nodiscard]] constexpr T & value_or(U && replacement) const noexcept {
		detail::taint_rvalue<U>{};
		return p_ ? *p_ : replacement;
	}

	template <class Func>
	[[nodiscard]] T & value_or_eval(Func f) const {
		return p_ ? *p_ : detail::forward_reference(f());
	}

	// deprecated
	// observers
	DEPRECATED [[nodiscard]] constexpr bool is_initialized() const noexcept {
		return p_ != nullptr;
	}

	template <typename U>
		requires detail::not_optional<U>
	DEPRECATED [[nodiscard]] constexpr T &
	get_value_or(U && replacement) const noexcept {
		detail::taint_rvalue<U>{};
		return p_ ? *p_ : replacement;
	}

	// modifiers
	template <typename U>
		requires detail::not_optional<U>
	DEPRECATED constexpr void reset(U && rhs) noexcept {
		detail::taint_rvalue<U>{};
		p_ = std::addressof(rhs);
	}
};

// [optional.relops]
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(const optional<T> & lhs, const optional<U> & rhs) {
	const bool lhs_has_value = lhs.has_value();
	return lhs_has_value == rhs.has_value() && (!lhs_has_value || *lhs == *rhs);
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator!=(const optional<T> & lhs, const optional<U> & rhs) {
	const bool lhs_has_value = lhs.has_value();
	return lhs_has_value != rhs.has_value() || (lhs_has_value && *lhs != *rhs);
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator<(const optional<T> & lhs, const optional<U> & rhs) {
	return rhs.has_value() && (!lhs.has_value() || *lhs < *rhs);
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator>(const optional<T> & lhs, const optional<U> & rhs) {
	return lhs.has_value() && (!rhs.has_value() || *lhs > *rhs);
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator<=(const optional<T> & lhs, const optional<U> & rhs) {
	return !lhs.has_value() || (rhs.has_value()&& *lhs <= *rhs);
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator>=(const optional<T> & lhs, const optional<U> & rhs) {
	return !rhs.has_value() || (lhs.has_value()&& *lhs >= *rhs);
}

template <typename T, std::three_way_comparable_with<T> U>
[[nodiscard]] constexpr std::compare_three_way_result_t<T, U>
operator<=>(const optional<T> & lhs, const optional<U> & rhs) {
	const bool lhs_has_value = lhs.has_value();
	const bool rhs_has_value = rhs.has_value();
	return lhs_has_value && rhs_has_value ? *lhs <=> *rhs : lhs_has_value <=> rhs_has_value;
}

// compare with base
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(const optional<T> & lhs, const detail::base<U> & rhs) {
	return lhs == static_cast<const optional<U> &>(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(const detail::base<T> & lhs, const optional<U> & rhs) {
	return static_cast<const optional<T> &>(lhs) == rhs;
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator!=(const optional<T> & lhs, const detail::base<U> & rhs) {
	return lhs != static_cast<const optional<U> &>(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator!=(const detail::base<T> & lhs, const optional<U> & rhs) {
	return static_cast<const optional<T> &>(lhs) != rhs;
}

template <typename T, typename U>
    requires requires(const T & a, const T & b) { { a > b } -> std::convertible_to<bool>; }
[[nodiscard]] constexpr bool operator>(const optional<T> & lhs, const detail::base<U> & rhs) {
	return lhs > static_cast<const optional<U> &>(rhs);
}
template <typename T, typename U>
    requires requires(const T & a, const T & b) { { a > b } -> std::convertible_to<bool>; }
[[nodiscard]] constexpr bool operator>(const detail::base<T> & lhs, const optional<U> & rhs) {
	return static_cast<const optional<T> &>(lhs) > rhs;
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator<(const optional<T> & lhs, const detail::base<U> & rhs) {
	return lhs < static_cast<const optional<U> &>(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator<(const detail::base<T> & lhs, const optional<U> & rhs) {
	return static_cast<const optional<T> &>(lhs) < rhs;
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator>=(const optional<T> & lhs, const detail::base<U> & rhs) {
	return lhs >= static_cast<const optional<U> &>(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator>=(const detail::base<T> & lhs, const optional<U> & rhs) {
	return static_cast<const optional<T> &>(lhs) >= rhs;
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator<=(const optional<T> & lhs, const detail::base<U> & rhs) {
	return lhs <= static_cast<const optional<U> &>(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator<=(const detail::base<T> & lhs, const optional<U> & rhs) {
	return static_cast<const optional<T> &>(lhs) <= rhs;
}

template <typename T, std::three_way_comparable_with<T> U>
[[nodiscard]] constexpr std::compare_three_way_result_t<T, U>
operator<=>(const optional<T> & lhs, const detail::base<U> & rhs) {
	const bool lhs_has_value = lhs.has_value();
	const bool rhs_has_value = rhs.has_value();
	return lhs_has_value && rhs_has_value ? *lhs <=> *rhs : lhs_has_value <=> rhs_has_value;
}
template <typename T, std::three_way_comparable_with<T> U>
[[nodiscard]] constexpr std::compare_three_way_result_t<T, U>
operator<=>(const detail::base<T> & lhs, const optional<U> & rhs) {
	const bool lhs_has_value = lhs.has_value();
	const bool rhs_has_value = rhs.has_value();
	return lhs_has_value && rhs_has_value ? *lhs <=> *rhs : lhs_has_value <=> rhs_has_value;
}

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
[[nodiscard]] constexpr bool operator/=(const optional<T> & o, std::nullopt_t) noexcept {
	return o.has_value();
}
template <typename T>
[[nodiscard]] constexpr bool operator/=(std::nullopt_t, const optional<T> & o) noexcept {
	return o.has_value();
}

template <typename T>
[[nodiscard]] constexpr bool operator<(const optional<T> & o, std::nullopt_t) noexcept {
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
[[nodiscard]] constexpr bool operator>(std::nullopt_t, const optional<T> & o) noexcept {
	return false;
}

template <typename T>
[[nodiscard]] constexpr bool operator<=(const optional<T> & o, std::nullopt_t) noexcept {
	return !o.has_value();
}
template <typename T>
[[nodiscard]] constexpr bool operator<=(std::nullopt_t, const optional<T> & o) noexcept {
	return true;
}

template <typename T>
[[nodiscard]] constexpr bool operator>=(const optional<T> & o, std::nullopt_t) noexcept {
	return true;
}
template <typename T>
[[nodiscard]] constexpr bool operator>=(std::nullopt_t, const optional<T> & o) noexcept {
	return !o.has_value();
}

template <typename T>
[[nodiscard]] constexpr std::strong_ordering
operator<=>(const optional<T> & o, std::nullopt_t) noexcept {
	return o.has_value() <=> false;
}

// [optional.comp.with.t]
template <typename T, typename U>
constexpr bool operator==(const optional<T> & o, const U & value) {
	return o.has_value() ? *o == value : false;
}
template <typename T, typename U>
constexpr bool operator==(const T & value, const optional<U> & o) {
	return o.has_value() ? *o == value : false;
}

template <typename T, typename U>
constexpr bool operator!=(const optional<T> & o, const U & value) {
	return o.has_value() ? *o != value : true;
}
template <typename T, typename U>
constexpr bool operator!=(const T & value, const optional<U> & o) {
	return o.has_value() ? *o != value : true;
}

template <typename T, typename U>
constexpr bool operator<(const optional<T> & o, const U & value) {
	return o.has_value() ? *o < value : true;
}
template <typename T, typename U>
constexpr bool operator<(const T & value, const optional<U> & o) {
	return o.has_value() ? *o > value : false;
}

template <typename T, typename U>
constexpr bool operator>(const optional<T> & o, const U & value) {
	return o.has_value() ? *o > value : true;
}
template <typename T, typename U>
constexpr bool operator>(const T & value, const optional<U> & o) {
	return o.has_value() ? *o < value : false;
}

template <typename T, typename U>
constexpr bool operator<=(const optional<T> & o, const U & value) {
	return o.has_value() ? *o <= value : false;
}
template <typename T, typename U>
constexpr bool operator<=(const T & value, const optional<U> & o) {
	return o.has_value() ? *o >= value : true;
}

template <typename T, typename U>
constexpr bool operator>=(const optional<T> & o, const U & value) {
	return o.has_value() ? *o >= value : false;
}
template <typename T, typename U>
constexpr bool operator>=(const T & value, const optional<U> & o) {
	return o.has_value() ? *o <= value : true;
}

template <typename T, std::three_way_comparable_with<T> U>
constexpr std::compare_three_way_result_t<T, U>
operator<=>(const optional<T> & o, const U & value) {
	return o.has_value() ? *o <=> value : std::strong_ordering::less;
}

// [optional.specalg]
template <typename T>
constexpr void swap(optional<T &> & lhs, optional<T &> & rhs) noexcept {
	lhs.swap(rhs);
}

template <typename T>
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

// non-standard additional Boost interfaces

template <typename T>
[[nodiscard]] constexpr optional<std::decay_t<T>>
make_optional(bool condition, T && v) {
	return optional<std::decay_t<T>>(condition, std::forward<T>(v));
}

template <typename T>
constexpr detail::const_reference_t<T> get(const optional<T> & o) {
	return o.get();
}

template <typename T>
constexpr detail::reference_t<T> get(optional<T> & o) {
	return o.get();
}

template <typename T>
constexpr detail::const_pointer_t<T> get(const optional<T> * o) {
	return o->get_ptr();
}

template <typename T>
constexpr detail::pointer_t<T> get(optional<T> * o) {
	return o->get_ptr();
}

template <typename T>
constexpr detail::const_reference_t<T>
get_optional_value_or(const optional<T> & o, detail::const_reference_t<T> v) {
	return o.get_value_or(v);
}

template <typename T>
constexpr detail::reference_t<T>
get_optional_value_or(optional<T> & o, detail::reference_t<T> v) {
	return o.get_value_or(v);
}

template <typename T>
constexpr detail::const_pointer_t<T> get_pointer(const optional<T> & o) {
	return o.get_ptr();
}

template <typename T>
constexpr detail::pointer_t<T> get_pointer(optional<T> & o) {
	return o.get_ptr();
}
} // namespace NS_OPTIONAL

// [optional.hash]
namespace std {
template <typename T>
struct hash<::NS_OPTIONAL::optional<T>> : hash<::NS_OPTIONAL::detail::base<T>> {
	using argument_type = ::NS_OPTIONAL::optional<T>;
	using base          = hash<::NS_OPTIONAL::detail::base<T>>;
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

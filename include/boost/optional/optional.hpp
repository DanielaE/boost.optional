#pragma once
//#if __cplusplus <= 201703L
//#  error insufficient language support!
//#endif
#include <optional>
#include <type_traits>
#include <concepts>
#include <compare>

#define DEPRECATED
#define CONSTEVAL constexpr

#if !defined(OPTIONAL_NO_CPP17_SIGNATURES) && defined(__cpp_lib_three_way_comparison)
//#define OPTIONAL_NO_CPP17_SIGNATURES
#endif
#if !defined(OPTIONAL_NO_QUICK_NULLOPT_RELOPS)
#define OPTIONAL_NO_QUICK_NULLOPT_RELOPS
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
using none_t              = std::nullopt_t;

#ifndef OPTIONAL_NONE_CPP20
#define OPTIONAL_NONE_CPP20
inline constexpr none_t none{ std::nullopt };
#endif

namespace {
template <typename T>
using base_optional = std::optional<T>;

CONSTEVAL bool optional_tag(...) { return false; }
template <typename T>
CONSTEVAL bool optional_tag(const volatile std::optional<T> *) { return true; }
template <typename T>
CONSTEVAL bool is_optional_v = optional_tag((std::remove_reference_t<T> *)nullptr);

template <typename T>
concept optional_type = is_optional_v<T>;
template <typename T>
concept not_optional = !optional_type<T>;

template <optional_type T>
using optional_value_type = typename std::remove_cvref_t<T>::value_type;

template <class T, class U>
concept constructible =
	std::is_same_v<T, std::remove_reference_t<U>> ||
	std::is_same_v<T, const std::remove_reference_t<U>>;

template <typename T>
concept is_inplace_factory =
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
	struct construct_by {
		construct_by(Factory && f) : f_{ std::addressof(f) } {}
		operator T() const noexcept {
			union {
				char c;
				std::remove_cv_t<T> t;
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
	[[nodiscard]] constexpr optional() noexcept = default;
	[[nodiscard]] constexpr optional(const T & other) : base(other) {}
	[[nodiscard]] constexpr optional(T && other) : base(std::forward<T>(other)) {}
	template <typename U>
		requires (!std::is_same_v<T, std::decay_t<U>>)
	[[nodiscard]] constexpr optional(U && rhs) : base(std::forward<U>(rhs)) {}

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
		requires requires(const base_optional<U> & rhs) { base::operator=(rhs); }
	optional & operator=(const optional<U> & rhs) {
		return static_cast<optional &>(
			base::operator=(static_cast<const base_optional<U> &>(rhs)));
	}

	template <typename U>
		requires requires(base_optional<U> && rhs) { base::operator=(std::move(rhs)); }
	optional & operator=(optional<U> && rhs) {
		return static_cast<optional &>(
			base::operator=(static_cast<base_optional<U> &&>(rhs)));
	}

	// conversion from base
	constexpr optional(const base & from) : base(from) {}
	constexpr optional(base && from) noexcept : base(std::move(from)) {}

	template <typename U>
	[[nodiscard]] constexpr bool operator==(U && rhs) const {
		const bool lhs_has_value = this->has_value();
		if constexpr (optional_type<U>) {
			return lhs_has_value == rhs.has_value() && (!lhs_has_value || **this == *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return !lhs_has_value;
		} else {
			return lhs_has_value && **this == rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator!=(U && rhs) const {
		const bool lhs_has_value = this->has_value();
		if constexpr (optional_type<U>) {
			return lhs_has_value != rhs.has_value() || (lhs_has_value && **this != *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return lhs_has_value;
		} else {
			return !lhs_has_value || **this != rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator<(U && rhs) const {
		const bool lhs_has_value = this->has_value();
		if constexpr (optional_type<U>) {
			return rhs.has_value() && (!lhs_has_value || **this < *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return false;
		} else {
			return !lhs_has_value || **this < rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator>(U && rhs) const {
		const bool lhs_has_value = this->has_value();
		if constexpr (optional_type<U>) {
			return lhs_has_value && (!rhs.has_value() || **this > *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return lhs_has_value;
		} else {
			return lhs_has_value && **this > rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator<=(U && rhs) const {
		const bool lhs_has_value = this->has_value();
		if constexpr (optional_type<U>) {
			return !lhs_has_value || (rhs.has_value() && **this <= *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return !lhs_has_value;
		} else {
			return !lhs_has_value || **this <= rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator>=(U && rhs) const {
		const bool lhs_has_value = this->has_value();
		if constexpr (optional_type<U>) {
			return !rhs.has_value() || (lhs_has_value && **this >= *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return true;
		} else {
			return lhs_has_value && **this >= rhs;
		}
	}
	template <typename U>
	[[nodiscard]] constexpr auto operator<=>(U && rhs) const {
		const bool lhs_has_value = this->has_value();
		if constexpr (optional_type<U>) {
			const bool rhs_has_value = rhs.has_value();
			return lhs_has_value && rhs_has_value ? **this <=> *rhs : lhs_has_value <=> rhs_has_value;
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return lhs_has_value <=> false;
		} else {
			return lhs_has_value ? **this <=> rhs : std::strong_ordering::less;
		}
	}

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
		requires is_inplace_factory<Factory>
	explicit optional(Factory && f)
	: base(std::in_place, construct_by(std::forward<Factory>(f))) {}

	// assignment
	template <typename Factory>
		requires is_inplace_factory<Factory>
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
		optional<optional_value_type<std::invoke_result_t<Func, T &>>>
	flat_map(Func f) & {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr
		optional<optional_value_type<std::invoke_result_t<Func, const T &>>>
	flat_map(Func f) const & {
		if (this->has_value())
			return f(**this);
		return none;
	}

	template <typename Func>
	[[nodiscard]] constexpr
		optional<optional_value_type<std::invoke_result_t<Func, T &&>>>
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
	friend CONSTEVAL bool optional_tag(const volatile optional<T &> *) { return true; }

	template <typename U>
	struct taint_rvalue {
		[[maybe_unused]] constexpr taint_rvalue() = default;
		static_assert(std::is_lvalue_reference_v<U>, "binding rvalues is ill-formed");
	};

	template <typename U>
	constexpr std::remove_reference_t<U> & forward_reference(U && r) {
		taint_rvalue<U>{};
		return std::forward<U>(r);
	}

	T * p_ = nullptr;

public:
	// construction
	[[nodiscard]] constexpr optional() noexcept = default;
	[[nodiscard]] constexpr optional(std::nullopt_t) noexcept {}
	[[nodiscard]] constexpr optional(const optional & other) noexcept = default;
	[[nodiscard]] constexpr optional(optional && other) noexcept      = default;
	optional(T &&) { taint_rvalue<T>{}; }

	template <class U>
	[[nodiscard]] constexpr explicit optional(const optional<U &> & other) noexcept : p_(other.p_) {}

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
		requires not_optional<U>
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

	template <typename U>
		requires not_optional<U>
	constexpr void emplace(U && rhs) noexcept {
		p_ = std::addressof(rhs);
		taint_rvalue<U>{};
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator==(U && rhs) const {
		const bool lhs_has_value = p_ != nullptr;
		if constexpr (optional_type<U>) {
			return lhs_has_value == rhs.has_value() && (!lhs_has_value || *p_ == *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return !lhs_has_value;
		} else {
			return lhs_has_value && *p_ == rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator!=(U && rhs) const {
		const bool lhs_has_value = p_ != nullptr;
		if constexpr (optional_type<U>) {
			return lhs_has_value != rhs.has_value() || (lhs_has_value && *p_ != *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return lhs_has_value;
		} else {
			return !lhs_has_value || *p_ != rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator<(U && rhs) const {
		const bool lhs_has_value = p_ != nullptr;
		if constexpr (optional_type<U>) {
			return rhs.has_value() && (!lhs_has_value || *p_ < *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return false;
		} else {
			return !lhs_has_value || *p_ < rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator>(U && rhs) const {
		const bool lhs_has_value = p_ != nullptr;
		if constexpr (optional_type<U>) {
			return lhs_has_value && (!rhs.has_value() || *p_ > *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return lhs_has_value;
		} else {
			return lhs_has_value && *p_ > rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator<=(U && rhs) const {
		const bool lhs_has_value = p_ != nullptr;
		if constexpr (optional_type<U>) {
			return !lhs_has_value || (rhs.has_value() && *p_ <= *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return !lhs_has_value;
		} else {
			return !lhs_has_value || *p_ <= rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr bool operator>=(U && rhs) const {
		const bool lhs_has_value = p_ != nullptr;
		if constexpr (optional_type<U>) {
			return !rhs.has_value() || (lhs_has_value && *p_ >= *rhs);
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return true;
		} else {
			return lhs_has_value && *p_ >= rhs;
		}
	}

	template <typename U>
	[[nodiscard]] constexpr auto operator<=>(U && rhs) const {
		const bool lhs_has_value = p_ != nullptr;
		if constexpr (optional_type<U>) {
			const bool rhs_has_value = rhs.has_value();
			return lhs_has_value && rhs_has_value ? *p_ <=> *rhs : lhs_has_value <=> rhs_has_value;
		} else if constexpr (std::is_convertible_v<U, std::nullopt_t>) {
			return lhs_has_value <=> false;
		} else {
			return lhs_has_value ? *p_ <=> rhs : std::strong_ordering::less;
		}
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
		requires not_optional<U>
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
	[[nodiscard]] optional<optional_value_type<std::invoke_result_t<Func, T &>>>
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

// [optional.relops]
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(const optional<T> & lhs, const optional<U> & rhs) {
	return lhs.operator==(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator==(const std::optional<T> & lhs, const optional<U> & rhs) {
	return rhs.operator==(lhs);
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator!=(const optional<T> & lhs, const optional<U> & rhs) {
	return lhs.operator!=(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator!=(const std::optional<T> & lhs, const optional<U> & rhs) {
	return rhs.operator!=(lhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator<(const optional<T> & lhs, const optional<U> & rhs) {
	return lhs.operator<(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator<(const std::optional<T> & lhs, const optional<U> & rhs) {
	return rhs.operator>(lhs);
}
#if 0
template <typename T, typename U>
[[nodiscard]] constexpr bool operator>(const optional<T> & lhs, const optional<U> & rhs) {
	return lhs.operator>(rhs);
}
#endif
template <typename T, typename U>
[[nodiscard]] constexpr bool operator>(const std::optional<T> & lhs, const optional<U> & rhs) {
	return rhs.operator<(lhs);
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator<=(const optional<T> & lhs, const optional<U> & rhs) {
	return lhs.operator<=(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator<=(const std::optional<T> & lhs, const optional<U> & rhs) {
	return rhs.operator<=(lhs);
}

template <typename T, typename U>
[[nodiscard]] constexpr bool operator>=(const optional<T> & lhs, const optional<U> & rhs) {
	return lhs.operator>=(rhs);
}
template <typename T, typename U>
[[nodiscard]] constexpr bool operator>=(const std::optional<T> & lhs, const optional<U> & rhs) {
	return rhs.operator<=(lhs);
}

#ifndef OPTIONAL_NO_QUICK_NULLOPT_RELOPS

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
#endif


// [optional.comp_with_t]

template <typename T, typename U>
	requires eq_comparable<T, U>
[[nodiscard]] constexpr bool operator==(const optional<T> & lhs, const U & rhs) {
	return lhs.operator==(rhs);
}
template <typename T, typename U>
	requires eq_comparable<T, U>
[[nodiscard]] constexpr bool operator==(const U & lhs, const optional<T> & rhs) {
	return rhs.operator==(lhs);
}

template <typename T, not_optional U>
	requires ne_comparable<T, U>
[[nodiscard]] constexpr bool operator!=(const optional<T> & lhs, const U & rhs) {
	return lhs.operator!=(rhs);
}
template <typename T, not_optional U>
	requires ne_comparable<T, U>
[[nodiscard]] constexpr bool operator!=(const U & lhs, const optional<T> & rhs) {
	return rhs.operator!=(lhs);
}

template <typename T, not_optional U>
	requires lt_comparable<T, U>
[[nodiscard]] constexpr bool operator<(const optional<T> & lhs, const U & rhs) {
	return lhs.operator<(rhs);
}
template <typename T, not_optional U>
	requires gt_comparable<T, U>
[[nodiscard]] constexpr bool operator<(const U & lhs, const optional<T> & rhs) {
	return rhs.operator>(lhs);
}

template <typename T, not_optional U>
	requires gt_comparable<T, U>
[[nodiscard]] constexpr bool operator>(const optional<T> & lhs, const U & rhs) {
	return lhs.operator>(rhs);
}
template <typename T, not_optional U>
	requires lt_comparable<T, U>
[[nodiscard]] constexpr bool operator>(const U & lhs, const optional<T> & rhs) {
	return rhs.operator<(lhs);
}

template <typename T, not_optional U>
	requires le_comparable<T, U>
[[nodiscard]] constexpr bool operator<=(const optional<T> & lhs, const U & rhs) {
	return lhs.operator<=(rhs);
}
template <typename T, not_optional U>
	requires le_comparable<T, U>
[[nodiscard]] constexpr bool operator<=(const U & lhs, const optional<T> & rhs) {
	return rhs.operator>=(lhs);
}

template <typename T, not_optional U>
	requires le_comparable<T, U>
[[nodiscard]] constexpr bool operator>=(const optional<T> & lhs, const U & rhs) {
	return lhs.operator>=(rhs);
}
template <typename T, not_optional U>
	requires ge_comparable<T, U>
[[nodiscard]] constexpr bool operator>=(const U & lhs, const optional<T> & rhs) {
	return rhs.operator<=(lhs);
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

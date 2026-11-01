#pragma once

#include <type_traits>
#include <tuple>
#include <functional>
#include <utils/language_support/always_false.hpp>

namespace utils
{
	template <typename T>
	struct is_integral :  std::integral_constant<bool,
		std::is_same_v<T, char> ||
		std::is_same_v<T, uint8_t> ||
		std::is_same_v<T, int8_t> ||
		std::is_same_v<T, uint16_t> ||
		std::is_same_v<T, int16_t> ||
		std::is_same_v<T, uint32_t> ||
		std::is_same_v<T, int32_t> ||
		std::is_same_v<T, uint64_t> ||
		std::is_same_v<T, int64_t> ||
		std::is_same_v<T, long> ||
		std::is_same_v<T, unsigned long>>
	{
	};
	template <typename T>
	constexpr bool is_integral_v = is_integral<T>::value;

	template <typename T>
	struct is_arithmetic :  std::integral_constant<bool,
		std::is_same_v<T, float> ||
		std::is_same_v<T, double> ||
		utils::is_integral_v<T>>
	{
	};
	template <typename T>
	constexpr bool is_arithmetic_v = is_arithmetic<T>::value;

	template <class T>
	struct remove_cvref
	{
		using type = std::remove_cv_t<std::remove_reference_t<T>>;
	};
	template<class T>
	using remove_cvref_t = typename remove_cvref<T>::type;

	template<typename T1,typename T2>
	using enable_if_same = std::enable_if_t<std::is_same_v<T1, T2>>;

	namespace inner
	{
		template<bool is_const, typename T>
		struct add_const;

		template<typename T>
		struct add_const<true, T>
		{
			static_assert(!std::is_const_v<T>);
			using type = std::add_const_t<T>;
		};
		template<typename T>
		struct add_const<false, T>
		{
			static_assert(!std::is_const_v<T>);
			using type = T;
		};
	}
	template<bool is_const, typename T>
	using add_const = typename inner::add_const<is_const, T>::type;
	
	namespace inner
	{
		template<bool is_first, typename T1, typename T2>
		struct select_type;

		template<typename T1, typename T2>
		struct select_type<true, T1, T2>
		{
			using type = T1;
		};
		template<typename T1, typename T2>
		struct select_type<false, T1, T2>
		{
			using type = T2;
		};
	}
	template<bool is_first, typename T1, typename T2>
	using select_type = typename inner::select_type<is_first, T1, T2>::type;

	template <typename T>
	using enable_for_integer = std::enable_if_t<utils::is_integral_v<T>>;

	template <typename T, typename U>
	using enable_for_same = std::enable_if_t<std::is_same_v<T,U>>;
}

//不那么常见的。放在更深的命名空间里
namespace utils::type_traits
{
	namespace inner::lambda
	{
		template<typename T>
		struct s_get
		{
			static_assert(utils::language_support::always_false<T>, "not a lambda");
		};

		template<typename T_class, typename T_return, typename ...T_args>
		struct s_get<T_return(__cdecl T_class::*)(T_args...) const>
		{
			using return_t = T_return;
			using args_tuple_t = std::tuple<T_args...>;
			using std_function_t = std::function<T_return(T_args...)>;
			using function_t = T_return(T_args...);
		};
	}

	//解析一个lambda
	template<typename T>
	using lambda = inner::lambda::s_get<decltype(&T::operator())>;
}
#pragma once

#include <vector>
#include <utils/compiler/windows_header/maxmin__eliminate.inc>
#include <boost/multiprecision/cpp_int.hpp>
#include <utils/compiler/windows_header/maxmin__restore.inc>
#include <utils/throw.hpp>

namespace utils::rsa
{
	//根据p,q,e, 算出d
	inline void get_d(
		_Out_ boost::multiprecision::cpp_int & d,
		_In_ const boost::multiprecision::cpp_int p,
		_In_ const boost::multiprecision::cpp_int q,
		_In_ const boost::multiprecision::cpp_int e)
	{
		using boost::multiprecision::cpp_int;
		cpp_int m = (p-1)*(q-1);
		//循环算出是哪个		//todo: 看看是否有更快的算法, 比如 辗转相除法
		for (int i = 0; i < e;i++)
		{
			cpp_int dd = i * m + 1;
			if (dd % e != 0)
				continue;

			d = dd / e;
			return;
		}
		utils_throw();
	}
	//计算 (a^d)%n
	//result: 结果
	inline void power_and_mod(
		_Out_ boost::multiprecision::cpp_int & result,
		_In_ const boost::multiprecision::cpp_int a,
		_In_ const boost::multiprecision::cpp_int d,
		_In_ const boost::multiprecision::cpp_int n)
	{
		using boost::multiprecision::cpp_int;
		std::vector<cpp_int> all;
		{
			cpp_int _d = d;
			cpp_int _cur = a;
			while (1)
			{
				if (_d % 2 == 1)
				{
					all.push_back(_cur);
				}

				_d /= 2;
				_cur *= _cur;
				_cur %= n;

				if (_d == 0)
					break;
			}
		}

		result = 1;
		for(const auto & one : all)
		{
			result *= one;
			result %= n;
		}
	}
}
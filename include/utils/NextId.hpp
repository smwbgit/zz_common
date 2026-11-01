#pragma once

#include <utils/type_traits.hpp>
#include <utils/assert.hpp>

namespace utils
{
	//依次分配一系列id
	template<typename id_T>
	class NextId
	{
	private:
		//简单检查
		static_assert(utils::is_integral_v<id_T>);
		
	private:
		const id_T m_first_id{};
		const id_T m_max_id{};
		id_T m_next_id{};
		bool m_has_next_id{true};

	public:
		NextId(_In_ const id_T first_id = 1, _In_ const id_T max_id = (std::numeric_limits<id_T>::max)())
			: m_first_id{first_id}
			, m_max_id{max_id}
			, m_next_id{first_id}
		{
			//简单检查
			utils_assert(first_id <= max_id);
		}
		//获得下一个id
		id_T get()
		{
			id_T id{};
			get(id);
			return id;
		}
		//特化
		void get(_Out_ id_T & id)
		{
			//简单检查
			utils_assert(m_has_next_id);

			//给出
			id = m_next_id;

			//更新状态
			m_has_next_id = id < m_max_id;
			m_next_id++;
		}
	};
}
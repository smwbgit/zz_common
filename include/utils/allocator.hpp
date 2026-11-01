#pragma once

#include <vector>
#include <memory>

namespace utils::allocator
{
	//简单的内存分配器，不支持多线程和数组分配，支持delete的空间重新被new使用
	//note: 使用指针串联起被delete的空间，用m_recommend指向；用m_next指向从未被分配过的空间;
	//      用m_rest_n表示从未被分配过的空间里还能分配多少个
	//e.g.:
	//		public:
	//			static void* operator new(std::size_t size)
	//			{
	//				return utils::allocator::simple_allocator<the_class>::get_instance().on_new(size);
	//			}
	//			static void operator delete(void* p)
	//			{
	//				return utils::allocator::simple_allocator<the_class>::get_instance().on_delete(p);
	//			}
	template<typename T>
	class simple_allocator
	{
	private:
		static_assert(sizeof(T) >= sizeof(uint64_t), "");	//需要使用空间来存个指针
	private:
		std::vector<std::unique_ptr<std::vector<uint8_t>>> m_blocks;
		void* m_next{};
		uint64_t m_rest_n{};
		void* m_recommend{};
		uint64_t m_next_block_element_n{8};
	private:
		std::vector<uint8_t> & make_new_block()
		{
			std::unique_ptr<std::vector<uint8_t>> block {new std::vector<uint8_t>};
			block->resize(m_next_block_element_n * sizeof(T));
			m_rest_n = m_next_block_element_n;
			m_blocks.push_back(std::move(block));

			//更新状态
			m_next_block_element_n *= 2;

			return **m_blocks.rbegin();
		}
	public:
		static simple_allocator & get_instance()
		{
			static simple_allocator inst{};
			return inst;
		}
		void* on_new(std::size_t size)
		{
			//给出一个之前被delete的空间
			if (m_recommend != m_next)
			{
				void* re = m_recommend;
				m_recommend = *(void**)m_recommend;
				return re;
			}

			//如果没有剩余空间了，就再申请一大块空间
			if (m_rest_n == 0)
			{
				auto & block = make_new_block();
				m_recommend = m_next = &block[0];
			}

			//暂存
			void* re = m_next;

			//更新状态
			m_recommend = m_next = (uint8_t*)m_next + sizeof(T);
			m_rest_n --;

			//给出
			return re;
		}
		void on_delete(void* p)
		{
			*(void**)p = m_recommend;
			m_recommend = p;
		}
	};
}

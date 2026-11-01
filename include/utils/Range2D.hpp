#pragma once

#include <utils/Index2D.hpp>
#include <utils/Range1D.hpp>

namespace utils
{
	class Range2D
	{
	public:
		utils::Index2D position;
		utils::Index2D size;
	public:
		Range2D()
		{
		}
		Range2D(utils::Index2D position, utils::Index2D size)
			: position{position}
			, size{size}
		{
		}
	public:
		bool operator == (const Range2D &) const = default;
	public:
		//序列化支持
		template<bool is_saving, typename serializer_T>
		void serialize_support(serializer_T & file)
		{
			file & position;
			file & size;
		}
#pragma region 包含/重叠 检查
		//是否包含指定位置
		bool is_include(const utils::Index2D _position) const
		{
			return
				_position.i >= position.i &&
				_position.i < position.i + size.i &&
				_position.j >= position.j &&
				_position.j < position.j + size.j;
		}
		//是否有重叠部分
		bool is_overlap(const Range2D & other) const
		{
			if (!Range1D{ position.i, size.i }.is_overlap({ other.position.i, other.size.i }))
				return false;
			if (!Range1D{ position.j, size.j }.is_overlap({ other.position.j, other.size.j }))
				return false;
			return true;
		}
		//是否在别的区域内
		//note: 要所有的点,都在其内
		bool is_in(const Range2D & bigger) const
		{
			//自己是个空区域
			if(size.is_exist_zero())
				return true;

			//准备数据
			const auto & small = *this;
			
			//左上角不在其内
			if(!bigger.is_include(small.position))
				return false;
			
			//右下角不在其内
			if(!bigger.is_include(small.position + small.size - utils::Index2D{1,1}))
				return false;

			//全在
			return true;
		}
#pragma endregion
		//获取中心位置
		utils::Index2D get_center_position() const
		{
			//简单检查
			utils_assert(size.i > 0 && size.j > 0);
			
			//一半大小
			const auto half_size = size / 2;

			//中心位置
			const auto center_position = position + half_size;

			//给出
			return center_position;
		}
	};
}
#pragma once

#include <set>
#include <utils/image/bool_image/BoolImage.hpp>

//分割
namespace utils::image::bool_image::cut
{
	//分成单独图片
	//note: 横向扫描
	inline std::vector<BoolImage> cut_to_single_images(const BoolImage & bool_image)
	{
		/*
		算法描述：
			先检查最左边的一竖行的所有像素，如果都是false，则跳过这一竖行，重新检测。
			如果没有要检查的像素，则结束。如果有为ture的像素，则，从所有的true像素出发，
			检查所有能到达的像素（周围8个方向（包括斜对角的））。把所有能到达的像素的，所有所在列，
			里边的所有true像素，都标为，可到达。换句话说，即使图像不相连，但只要它和可到达的像素，
			处于同一个x坐标，就也算可到达。然后，从现有的所有可到达像素出发，按同样的算法，
			继续寻找新的可到达的像素，直到没有新的像素被加入，为止。然后把这一段的图片，做为一个
			单独图片给出（给出前，把图片上边和下边的空白行，去掉）。并从这个图片的下一个竖行，
			开始重新检测。
		*/

		//简单检查
		utils_assert(bool_image.get_size().i > 0);
		utils_assert(bool_image.get_size().j > 0);

		//准备数据
		std::vector<BoolImage> all_image;

		//当前的x坐标
		s8 x = 0;

		//图片大小
		const auto image_size = bool_image.get_size();

		//图片宽度
		const auto image_width = image_size.i;

		//图片高度
		const auto image_height = image_size.j;

		//循环处理
		while(1)
		{
			//所有的列都已处理完
			if(x >= image_width)
			{
				//不再循环
				break;
			}

			//当前列是否都是false像素
			const auto is_all_false_in_current_column = [&]()
			{
				//对于当前列中的每个位置
				for(s8 y = 0; y < image_height; y++)
				{
					//拼出位置
					const utils::Index2D position {x, y};

					//获取布尔像素
					const auto bool_pixel = bool_image.get_pixel(position);

					//是true像素
					if(bool_pixel)
					{
						//给出
						return false;
					}
				}

				//给出
				return true;
			}();

			//当前列都是false像素
			if(is_all_false_in_current_column)
			{
				//移动到下一列
				x ++;

				//进行下一次循环
				continue;
			}

			//获取和当前列，相连的最远的列
			const auto x_right = [&]()
			{
				/*
				算法描述:
					获取当前列中，所有的true像素，加入到当前列的{所有可达的像素集合}里。
					由当前列的{所有可达的像素集合}，推出，下一列的{所有可达的像素集合}。
					如果没有下一列，则结束。如果下一列的{所有可达的像素集合}为空，则结束。
					否则，把当前列挪到下一列，然后重新开始该算法。
				*/

				//当前列
				auto current_x = x;

				//当前列的{所有可达的像素集合}
				std::set<s8> all_reachable_y;

				//循环执行
				while(1)
				{
					//没有下一列
					if(current_x == image_width - 1)
					{
						//不再继续
						break;
					}

					//获取当前列中，所有的true像素，加入到当前列的{所有可达的像素集合}里
					{
						//对于当前列的每个位置
						for(s8 y = 0; y < image_height; y++)
						{
							//像素位置
							const utils::Index2D position{current_x, y};

							//获取像素
							const auto bool_pixel = bool_image.get_pixel(position);

							//是true像素
							if(bool_pixel)
							{
								//加入到可达集合中
								all_reachable_y.insert(y);
							}
						}
					}
					
					//简单检查
					utils_assert(!all_reachable_y.empty());

					//由当前列的{所有可达的像素集合}，推出，下一列的{所有可达的像素集合}
					std::set<s8> all_reachable_y_in_next_column = [&]()
					{
						//准备数据
						std::set<s8> all_reachable_y_in_next_column;

						//对于当前列的可达集合中的每个像素
						for(const auto y : all_reachable_y)
						{
							/*
							算法描述:
								检查它的右上角，右边，右下角像素，是否可达，也就是，是否是true像素。
								把他们当中的true像素，加入到下一列的{所有可达的像素集合}里。
								同时注意坐标不要超过图片大小。
							*/

							//对于三个中的每一个
							for (s8 y_offset = -1; y_offset < 2; y_offset++)
							{
								//拼出像素位置
								const utils::Index2D position{current_x + 1, y + y_offset};

								//位置不在图片上
								if(!position.is_in(image_size))
								{
									//不再继续，检查下一个
									continue;
								}
								
								//获取像素
								const auto bool_pixel = bool_image.get_pixel(position);

								//是true像素，可达
								if(bool_pixel)
								{
									//像素的y坐标
									const auto _y = position.j;

									//加入到下一列的可达集合
									all_reachable_y_in_next_column.insert(_y);
								}
							}
						}

						//给出
						return all_reachable_y_in_next_column;
					}();

					//下一列的{所有可达的像素集合}为空
					if(all_reachable_y_in_next_column.empty())
					{
						//不再继续
						break;
					}

					//把下一列，变为当前列
					current_x ++;

					//把下一列的可达集合，变为当前列的可达集合
					all_reachable_y = std::move(all_reachable_y_in_next_column);
				}

				//当前列，就是相连的最远列
				const auto x_right = current_x;

				//给出
				return x_right;
			}();

			//简单检查
			utils_assert(x <= x_right);
			utils_assert(x_right < image_width);

			//把这一块的图片，抠出来
			const auto sub_image = [&]()
			{
				//拼出子图片的位置
				const utils::Index2D position{x, 0};

				//拼出子图片的大小
				const utils::Index2D size{x_right - x + 1, image_height};

				//子图片的矩形
				const utils::Range2D rect{position, size};

				//获取子图片
				const auto sub_image = bool_image.get_sub_image(rect);

				//去掉图片上边和下边的空白行
				const auto sub_image_2 = [&sub_image]()
				{
					//图片大小
					const auto image_size = sub_image.get_size();

					//图片宽度
					const auto image_width = image_size.i;

					//图片高度
					const auto image_height = image_size.j;

					//准备函数：指定的一行中，是否存在true像素
					auto is_exist_true_in_one_row = [&sub_image, &image_width](s8 y)
					{
						//对于每个像素位置
						for(s8 x = 0; x < image_width; x++)
						{
							//拼出像素位置
							const utils::Index2D position{x, y};

							//获取像素
							const auto bool_pixel = sub_image.get_pixel(position);

							//是true像素
							if(bool_pixel)
							{
								//给出
								return true;
							}
						}

						//给出
						return false;
					};

					//获取上边界
					const auto y_top = [&]()
					{
						//从上到下，对于每一行
						for(s8 y = 0; y < image_height; y++)
						{
							//当前行是否存在true像素
							const auto is_exist_true_in_current_row = is_exist_true_in_one_row(y);

							//当前行存在true像素
							if(is_exist_true_in_current_row)
							{
								//当前行既是上边界
								const auto y_top = y;

								//给出
								return y_top;
							}
						}

						//没有true像素存在
						utils_throw();
					}();

					//获取下边界
					const auto y_bottom = [&]()
					{
						//从下到上，对于每一行
						for(s8 y = image_height - 1; y >= 0; y--)
						{
							//当前行是否存在true像素
							const auto is_exist_true_in_current_row = is_exist_true_in_one_row(y);

							//当前行存在true像素
							if(is_exist_true_in_current_row)
							{
								//当前行既是下边界
								const auto y_bottom = y;

								//给出
								return y_bottom;
							}
						}

						//没有true像素存在
						utils_throw();
					}();

					//简单检查
					utils_assert(y_top <= y_bottom);

					//新的高度
					const auto new_height = y_bottom - y_top + 1;

					//新的图片位置
					const utils::Index2D position{0, y_top};

					//新的图片大小
					const utils::Index2D size{image_width, new_height};

					//新的图片矩形
					const utils::Range2D rect{position, size};

					//获取子图片
					const auto sub_image_2 = sub_image.get_sub_image(rect);

					//给出
					return sub_image_2;
				}();

				//给出
				return sub_image_2;
			}();

			//保存
			all_image.push_back(std::move(sub_image));

			//移动当前列
			x = x_right + 1;
		}

		//给出
		return all_image;
	}
}
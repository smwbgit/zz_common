#pragma once

#include <optional>
#include <utils/image/Image2D.hpp>
#include <utils/data_encoding/md5.hpp>
#include <utils/Index2D.hpp>
#include <utils/image/md5.hpp>
#include <utils/file_format/bmp.hpp>
#include <utils/DataWriter.hpp>
#include <utils/DataReader.hpp>
#include <utils/cache_file/CacheFile.hpp>
#include <utils/event/Event.hpp>

namespace utils::windows::ui::auto_operation
{
	//一个小图片
	class SmallImage
	{
	public:
		//小图片的所有像素的md5
		utils::data_encoding::md5::MD5Value m_md5{};
		//矩形(在整个大图片中)
		utils::Range2D m_rect;
		//文件名
		std::u8string m_filename;
	public:
		//用户数据
		//note: 留给用户自由使用
		struct s_user_data
		{
			//比如，可以用于存储，点击位置
			utils::Index2D click_position;
			//比如，可以用于点击后，执行特别动作
			utils::event::Event<void()> click_event;
		} m_user_data;
	public:
		//载入
		void load(const std::u8string & filename, const utils::Range2D & rect)
		{
			//简单检查
			utils_assert(m_md5 == decltype(m_md5){});

			//保存: 矩形
			m_rect = rect;

			//保存: 文件名
			m_filename = filename;

			//接下来，从缓存文件中读取md5

			//获取缓存文件的文件名
			const auto filename_cache = get_filename_cache(filename, rect);

			//缓存文件
			utils::cache_file::CacheFile cache_file{filename_cache};

			//生成缓存签名
			utils::data cache_signature;
			{
				//数据序列化
				utils::DataWriter output;

				//添加各种签名参数
				output.serializer & rect;

				//给出
				cache_signature = output.get_data();
			}

			//设置缓存签名
			cache_file.set_signature(cache_signature);

			//获取缓存内容
			auto cache_payload = cache_file.get_payload();

			//获取成功
			if(cache_payload.has_value())
			{
				//从缓存内容里读取
				
				//数据反序列化
				utils::DataReader input{*cache_payload};

				//读取md5
				input.serializer & m_md5;

				//简单检查
				input.readed_all();
			}
			//获取失败
			else
			{
				//自己生成
				{
					//载入整张图片
					const auto whole_bmp = utils::file_format::bmp::load(filename);

					//获取小图片
					auto bmp = whole_bmp.get_sub_image(m_rect);

					//小图片的md5
					auto md5 = utils::image::md5::get(bmp);

					//保存
					m_md5 = std::move(md5);
				}

				//接下来，用刚刚生成的md5，生成缓存文件

				//生成缓存内容
				utils::data cache_payload;
				{
					//数据序列化
					utils::DataWriter output;

					//写入md5
					output.serializer & m_md5;

					//给出
					cache_payload = output.get_data();
				}

				//生成缓存文件
				cache_file.save(cache_payload);
			}
		}
		//特化
		void load(const std::u8string & filename, const utils::Index2D center_position,
			const utils::Index2D radius = {5,5})
		{
			//简单检查
			utils_assert(radius.i >= 0);
			utils_assert(radius.j >= 0);

			//小图片的矩形
			const utils::Range2D rect{center_position - radius, radius * 2 + 1};

			//转发
			return load(filename, rect);
		}

		//缓存目录。由用户指定，必须项
		inline static std::optional<std::filesystem::path> m_cache_directory;

		//获取缓存文件的文件名
		static std::u8string get_filename_cache(
			const std::u8string & filename,
			const utils::Range2D & rect)
		{
			//第一步，获得在缓存目录中，的缓存文件名。（不包含矩形信息）
			const auto filename_1 = [&]()
				-> std::u8string
			{
				//简单检查
				utils_assert(m_cache_directory.has_value());

				//缓存目录
				const auto & cache_directory = *m_cache_directory;

				//缓存目录的父目录
				const auto parent_directory_for_cache_directory = cache_directory.parent_path();

				//原始文件
				const std::filesystem::path file_path{ filename };

				//获取 原始文件 相对于 缓存目录的父目录 的 相对路径
				const auto relative_path = [&]()
					-> std::filesystem::path
				{
					//查找相对路径的起始点
					const auto [it_1, it_2] = std::mismatch(
						file_path.begin(), file_path.end(),
						parent_directory_for_cache_directory.begin(),
						parent_directory_for_cache_directory.end());

					//简单检查
					utils_assert(it_1 != file_path.end());
					utils_assert(it_2 == parent_directory_for_cache_directory.end());

					//获取相对路径
					const auto relative_path = [&]()
						-> std::filesystem::path
					{
						//准备数据: 相对路径
						std::filesystem::path relative_path;

						//对于每个剩余的路径节点
						for(auto current = it_1; current != file_path.end(); current++)
						{
							//添加
							relative_path /= *current;
						}

						//给出
						return relative_path;
					}();

					//给出
					return relative_path;
				}();

				//拼出缓存文件
				const auto cache_file_path = cache_directory / relative_path;

				//准备好它的所有父目录
				{
					//准备数据: 错误码
					std::error_code error_code{};

					//父目录
					const auto parent_directory_for_cache_file = cache_file_path.parent_path();

					//创建目录
					std::filesystem::create_directories(parent_directory_for_cache_file, error_code);

					//简单检查
					utils_assert(!error_code);
				}

				//获取缓存文件的u8string形式
				const auto cache_filename = cache_file_path.u8string();

				//给出
				return cache_filename;
			}();

			//第二步，添加矩形信息
			const auto filename_2 = filename_1 +
				utils::text::format::format(u8".{%lld,%lld}", rect.position.i, rect.position.j) +
				utils::text::format::format(u8".{%lld,%lld}", rect.size.i, rect.size.j);

			//给出
			return filename_2;
		}
	};
}
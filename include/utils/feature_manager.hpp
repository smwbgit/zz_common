#pragma once

#include <memory>
#include <utils/modifier/Modifier.hpp>
#include <utils/noun.hpp>
#include <utils/map.hpp>

//功能管理器
namespace utils::feature_manager
{
	namespace modifier
	{
		//运行环境
		class Context : public utils::modifier::Modifier
		{
		};
	}

	namespace noun
	{
		//一项功能
		class Feature : public utils::noun::Noun
		{
		protected:
			//功能名称
			std::u8string m_name;
		public:
			//初始化
			virtual void init(modifier::Context & context) = 0;
			//获取到了一点点执行时间
			virtual void tick(modifier::Context & context) = 0;
		public:
			//获取名字
			const std::u8string & get_name() const
			{
				return m_name;
			}
		};
	}

	//功能管理器
	class FeatureManager
	{
	private:
		//运行时环境
		std::unique_ptr<modifier::Context> m_context;
		//所有功能
		std::map<std::u8string, std::vector<std::unique_ptr<noun::Feature>>> m_all_feature;
	public:
		//设置运行环境
		void set_context(std::unique_ptr<modifier::Context> context)
		{
			//简单检查
			utils_assert(!m_context);
			utils_assert(context);

			//保存
			m_context = std::move(context);
		}
		//获取运行环境
		modifier::Context & get_context()
		{
			//简单检查
			utils_assert(m_context);

			//给出
			return *m_context;
		}
		//添加功能
		void add_feature(const std::u8string & group_name, std::unique_ptr<noun::Feature> feature)
		{
			//运行环境
			auto & context = get_context();

			//初始化
			feature->init(context);

			//保存
			m_all_feature[group_name].push_back(std::move(feature));
		}
		//获得了一点点运行时间
		//group_name: 要运行的功能组
		void tick_features(const std::u8string & group_name)
		{
			//要运行的功能组
			const auto & features = utils::map(m_all_feature).get_existed_value(group_name);

			//运行环境
			auto & context = get_context();

			//对于每个功能
			for(const auto & feature : features)
			{
				//让它获得一点点运行时间
				feature->tick(context);
			}
		}
	};
}
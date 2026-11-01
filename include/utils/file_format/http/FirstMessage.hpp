#pragma once

#include <vector>
#include "utils/string.hpp"
#include "utils/map.hpp"
#include "utils/data_encoding/base64.hpp"

namespace utils::file_format::http
{
	//构造一个http请求报文
	//example:
	//	a):
	//		utils::file_format::http::FirstMessage message;
	//		message.set_method("POST");
	//		message.set("Host", "www.123.org");
	//		message.set_authorization("the_user", "the_password");
	//		message.set("Cookie", "PHPSESSID=sess_id");
	//		message.set("Content-Type", "application/x-www-form-urlencoded");
	//		message.set_body("name=123");
	//		message.build_message(data_send);
	//	b):
	//		utils::file_format::http::FirstMessage message;
	//		message.set_head(R"(POST /translate_o?smartresult=dict&smartresult=rule HTTP/1.1
	//		Content-Length: 253
	//		Host: fanyi.youdao.com
	//		...)");
	//		message.set_body("name=123");
	//		message.build_message(data_send);
	class FirstMessage
	{
	private:
		std::string m_method;
		std::string m_page;
		utils::map<std::string, std::string> m_header_fields;
		std::vector<uint8_t> m_body;
	public:
		//method: "GET" or "POST"
		void set_method(const std::string & method)
		{
			m_method = method;
		}
		//example: "/index.php"
		void set_page(const std::string & page)
		{
			m_page = page;
		}
		//note: "content-length"字段会在调用build_message()时自动调整为合适值
		//example:
		//	set("Host", "www.123.org");
		void set(const std::string & name, const std::string & value)
		{
			//字段名转为小写
			std::string _name = name;
			utils::string_function::to_lower(_name);

			//保存
			m_header_fields[_name] = value;
		}
		void set_body(const std::string & body)
		{
			m_body.assign(body.begin(), body.end());
		}
		//填写authorization字段
		void set_authorization(const std::string & user, const std::string & password)
		{
			std::string tmp;
			utils::data_encoding::base64::encode(tmp, user + ":" + password);	//todo: look if need check ':' existing in user and password
			set("authorization", "Basic " + tmp);
		}
		//设置报文头
		//text: e.g.:	R"(POST /translate_o?smartresult=dict&smartresult=rule HTTP/1.1
		//				Content-Length: 253
		//				...
		//				Host: fanyi.youdao.com)"
		void set_head(_In_ std::string_view text)
		{
			std::vector<std::string_view> lines;
			utils::string_function::split(lines, text, "\n", false);

			//简单检查
			utils_assert(lines.empty() == false);

			//提取第一行
			{
				//提取method
				std::string_view line = lines[0];
				if (line.starts_with("GET "))
				{
					set_method("GET");
					line.remove_prefix(4);
				}
				else if (line.starts_with("POST "))
				{
					set_method("POST");
					line.remove_prefix(5);
				}
				else
					utils_throw();

				//提取协议版本
				{
					//简单检查
					utils_assert(line.ends_with(" HTTP/1.1"));

					line.remove_suffix(9);
				}

				//提取page
				{
					//简单检查
					utils_assert(line.find(' ') == line.npos);
					utils_assert(line.empty() == false);

					set_page((std::string)line);
				}
			}

			//提取其余行
			for (const auto i : utils::iterator_index(lines))
			{
				//跳过第一行
				if (i == 0)
					continue;

				//提取
				std::string_view line = lines[i];
				auto pos = line.find(": ");

				//简单检查
				utils_assert(pos != line.npos);

				//提取 cont.
				const auto name = line.substr(0, pos);
				const auto value = line.substr(pos + 2);

				//简单检查
				utils_assert(name.empty() == false);
				utils_assert(value.empty() == false);
	
				//保存
				set(
					(std::string)name,
					(std::string)value
				);
			}
		}
		//生成报文
		void build_message(_Out_ std::vector<uint8_t> & data)
		{
			std::string head;
			{
				auto add_one_header_field = [&](const std::string & line)
				{
					head += line + "\r\n";
				};

				//method
				{
					//简单检查
					utils_assert(m_method == "GET" || m_method == "POST");
					utils_assert(m_page.empty() == false);

					add_one_header_field(m_method + " " + m_page + " HTTP/1.1");
				}

				//fields
				{
					//消息体大小
					{
						if (m_body.size() > 0)	//todo 看下这里是和POST相关还是和m_body的大小相关
							set("content-length", std::to_string(m_body.size()));
						else
							m_header_fields.erase("content-length");
					}

					for (const auto & e : m_header_fields)
						add_one_header_field(e.first + ": " + e.second);
				}

				//head finish
				add_one_header_field("");
			}

			//给出
			data.clear();
			data.reserve(head.length() + m_body.size());
			data.assign(head.begin(), head.end());
			data.insert(data.end(), m_body.begin(), m_body.end());
		}
	};
}

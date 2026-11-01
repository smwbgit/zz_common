#pragma once

#include <string>
#include <vector>
#pragma comment(lib, "ws2_32.lib")
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <utils/map.hpp>
#include <utils/string.hpp>
#include <utils/data_encoding/base64.hpp>
#include <utils/assert.hpp>
#include <utils/real_path.hpp>
#include <utils/FileReader.hpp>
#include <utils/windows/command.hpp>
#include <utils/on_leave_scope.hpp>

namespace utils::network::http
{
#if 0
	//从指定位置下载文件
	//url: e.g. http://www.123.com/demo.txt
	//filename: e.g. c:/a/demo.txt
	inline void download(const std::string & url, const std::string & filename)
	{
		utils_assert(S_OK == URLDownloadToFileA(NULL, url.c_str(), filename.c_str(), 0, NULL));
	}
	//从指定位置下载，并以字符串形式返回
	//url: e.g. http://www.123.com/demo.txt
	//filename: e.g. c:/a/demo.txt
	inline std::string download(const std::string & url)
	{
		auto filename = utils::real_path::get_temp_filename();
		download(url, filename);

		utils::FileReader input{filename};
		std::string all;
		input.read_whole_file(all);
		return all;
	}
#endif
	//下载文件
	//url: e.g. https://www.123.com/demo.txt
	inline void download(const std::string & url, const std::string & filename)
	{
#ifdef utils_on_windows
		//简单检查
		utils_assert(url.starts_with("'@") == false);
		utils_assert(url.find('\n') == url.npos);

		auto re = utils::windows::command::run(R"(powershell -Command try{$no_show=Invoke-WebRequest -Uri @'
)" + url + R"(
'@ -OutFile ')" + filename + R"(';$StatusCode=200}catch{$StatusCode=201}; exit $StatusCode)");
		utils_assert(re == 200);
#else
#	error
#endif
	}
	//获取body
	//url: e.g. https://www.123.com/demo.txt
	inline std::string get_content(const std::string & url)
	{
		auto filename = utils::real_path::get_temp_filename();
		download(url, filename);

		utils::FileReader input{filename};
		std::string all;
		input.read_whole_file(all);
		return all;
	}
	//特化：快速版
	//host:  www.baidu.com
	//path: /a/b.txt
	inline std::string get_content_fast_https(_In_ const std::string & host, _In_ const std::string & path, _In_ bool is_safe_mode)
	{
#ifndef utils_on_windows
#error
#endif
		utils_assert(is_safe_mode == false);	//todo 添加安全模式， 主要是修改HttpOpenRequestA的flag参数

		HINTERNET hSession = InternetOpenA("", INTERNET_OPEN_TYPE_DIRECT, 0, 0, 0);
		utils_assert(hSession);
		utils_on_leave_scope(utils_assert(InternetCloseHandle(hSession)));

		HINTERNET hConnect = InternetConnectA(hSession, host.c_str(), 443, 0, 0, INTERNET_SERVICE_HTTP, 0, 0);
		utils_assert(hConnect);
		utils_on_leave_scope(utils_assert(InternetCloseHandle(hConnect)));

		auto hRequest = HttpOpenRequestA(hConnect, "GET", path.c_str(), HTTP_VERSIONA, 0, 0, 
			INTERNET_FLAG_RELOAD |
			INTERNET_FLAG_SECURE |
			INTERNET_FLAG_NO_COOKIES |
	//		INTERNET_FLAG_NO_AUTH |
	//		INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
	//		INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
	//		INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
	//		INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
			0, 0);
		utils_assert(hRequest);
		utils_on_leave_scope(utils_assert(InternetCloseHandle(hRequest)));

		{
			DWORD dwFlags = 0;
			DWORD dwBuffLen = sizeof(dwFlags);
			utils_assert(InternetQueryOptionA(hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwBuffLen));

			dwFlags |= SECURITY_FLAG_IGNORE_REVOCATION;

			utils_assert(InternetSetOptionA(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags)));
		}

		utils_assert(HttpSendRequestA(hRequest, 0, 0, 0, 0));

		int status_code{};
		DWORD status_code_size{sizeof(status_code)};
		utils_assert(HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &status_code_size, 0));
		utils_assert(status_code == 200);

		utils::data data;
		utils::data buffer;
		buffer.resize(1000);
		while(1)
		{
			DWORD readed_size{};
			utils_assert(InternetReadFile(hRequest, &buffer[0], buffer.size(), &readed_size));
			if(readed_size == 0)
				break;
			data.append(&buffer[0], readed_size);
		}
		return std::string{(const char*)&data[0], data.size()};
	}
}
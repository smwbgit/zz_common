#pragma once

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#include <memory>
#include <filesystem>
#include <utils/function.hpp>
#include <utils/iterator.hpp>
#include <optional>
#include <utils/Index2D.hpp>
#include <utils/algorithm/iterator.hpp>
#include <utils/set.hpp>
#include <utils/type.hpp>
#include <utils/vector.hpp>
#include <utils/windows/ui/window.hpp>
#include <utils/real_path.hpp>
#include <utils/CallWhenProgramStart.hpp>
#include <utils/map.hpp>

namespace utils::windows::ui::gdiplus
{
	namespace helper
	{
		/*
		//refer: https://learn.microsoft.com/zh-cn/windows/win32/gdiplus/-gdiplus-retrieving-the-class-identifier-for-an-encoder-use
		支持的name:
		image/bmp
		image/jpeg
		image/gif
		image/tiff
		image/png
		*/
		inline CLSID* get_image_encoder_clsid(const std::wstring & name)
		{
			static std::map<std::wstring, CLSID> table = []()
			{
				std::map<std::wstring, CLSID> table;

				UINT num{}, size{};
				utils_assert(Gdiplus::Status::Ok == Gdiplus::GetImageEncodersSize(&num, &size));
				utils_assert(size != 0);

				std::vector<uint8_t> buffer;
				buffer.resize(size);
				auto pImageCodecInfo = (Gdiplus::ImageCodecInfo*)&buffer[0];

				utils_assert(Gdiplus::Status::Ok == Gdiplus::GetImageEncoders(num, size, pImageCodecInfo));

				for (UINT j = 0; j < num; j++)
					utils::map(table).insert_not_exist(pImageCodecInfo[j].MimeType, pImageCodecInfo[j].Clsid);

				return table;
			}();

			return &utils::map(table).get_existed_value(name);
		}
	}

	//初始化和释放
	inline void init()
	{
		class Init
		{
		private:
			::Gdiplus::GdiplusStartupInput m_startup_input;
			ULONG_PTR m_token;
		public:
			Init()
			{
				utils_assert(
					::Gdiplus::Status::Ok ==
					::Gdiplus::GdiplusStartup(&m_token, &m_startup_input, NULL));
			}
			~Init()
			{
				::Gdiplus::GdiplusShutdown(m_token);
			}
		};
		static Init _init;
	}

	//自动初始化
	CallWhenProgramStart([]()
	{
		utils::windows::ui::gdiplus::init();
	});

	namespace inner
	{
		//加快GetPixel操作的速度
		class LockedBitmap
		{
		private:
			::Gdiplus::Bitmap & m_bmp;
			::Gdiplus::BitmapData m_data{};
		public:
			LockedBitmap(::Gdiplus::Bitmap & bmp)
				: m_bmp{bmp}
			{
				//简单检查
				//auto re = m_bmp.GetPixelFormat();
				utils_assert(PixelFormat24bppRGB == m_bmp.GetPixelFormat() || PixelFormat32bppRGB == m_bmp.GetPixelFormat() || PixelFormat1bppIndexed == m_bmp.GetPixelFormat());

				utils_assert(::Gdiplus::Status::Ok == m_bmp.LockBits(0, ::Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat24bppRGB, &m_data));

				//简单检查
				utils_assert(m_data.Width == m_bmp.GetWidth() && m_data.Height == m_bmp.GetHeight());
				utils_assert(m_data.Stride > 0);	//If the pixel data is stored bottom-up, the Stride data member is negative. If the pixel data is stored top-down, the Stride data member is positive.
			}
			~LockedBitmap()
			{
				auto re = m_bmp.UnlockBits(&m_data);
				utils_assert_abort(::Gdiplus::Status::Ok == re);

				//utils_assert_abort(::Gdiplus::Status::Ok == m_bmp.UnlockBits(&m_data));
			}
			COLORREF GetPixel(int x, int y)
			{
				if (m_data.Stride > 0)
				{
					auto p = (BYTE*)m_data.Scan0 + y * m_data.Stride + x * 3;
					return RGB(*(p + 2), *(p + 1), *p);
				}
				else
				{
					//todo 验证
					utils_throw();
					auto p = (BYTE*)m_data.Scan0 + (m_data.Height - y - 1) * (-m_data.Stride) + x * 3;
					return RGB(*(p + 2), *(p + 1), *p);
				}
			}
			uint32_t GetWidth()
			{
				return m_data.Width;
			}
			uint32_t GetHeight()
			{
				return m_data.Height;
			}
		};
	}

	//因为::Gdiplus::Bitmap需要创建它的HBITMAP一直存在，所以写了这个类，来在析构时，把HBITMAP删除掉
	class Bitmap
	{
	public:
		class s_modify
		{
		private:
			Bitmap* m_this;
		public:
			s_modify(Bitmap* _this) : m_this{_this} {}

			//把指定颜色外的地方，都变为另一种指定的颜色
			void keep_these_colors(const std::vector<COLORREF> & keep, COLORREF change_to)
			{
				const auto & _keep = utils::vector(keep);
				for(const auto pos : m_this->size())
				{
					auto color = m_this->get_pixel(pos);
					
					if(!_keep.is_exist(color))
						m_this->set_pixel(pos, change_to);
				}
			}
		} modify{this};
	private:
		struct s_quick_find_info
		{
			utils::Index2D::Range block;
			std::set<COLORREF> colors;
		};
	private:
		std::unique_ptr<::Gdiplus::Bitmap> m_bmp;
		std::function<void()> m_clean;
		std::unique_ptr<utils::windows::ui::gdiplus::inner::LockedBitmap> m_locked_bmp;	//这个要在m_bmp的下面，这个需要m_bmp里的资源
		std::optional<COLORREF> m_background_color;	//如果更改，需要取消掉m_quick_find_info，因为它里面保存了依照背景色找出的最大方块
		mutable std::optional<s_quick_find_info> m_quick_find_info;
	public:
		Bitmap(const std::filesystem::path & filename, std::optional<COLORREF> background_color = {})
		{
			m_bmp = std::make_unique<::Gdiplus::Bitmap>(filename.wstring().c_str());
			utils_assert(::Gdiplus::Status::Ok == m_bmp->GetLastStatus());
			m_background_color = background_color;
			make_locked();
		}
		Bitmap(utils::Index2D size, ::Gdiplus::PixelFormat format = PixelFormat24bppRGB)
		{
			m_bmp = std::make_unique<::Gdiplus::Bitmap>(size.i, size.j, format);
		}
		/* example:
			Bitmap bitmap{std::make_unique<::Gdiplus::Bitmap>(membit,(HPALETTE)NULL),[memdc, membit]()
			{
				DeleteObject(membit);
				DeleteObject(memdc);
			}};
		*/
		Bitmap(std::unique_ptr<::Gdiplus::Bitmap> bmp, std::function<void()> clean)
			: m_bmp{std::move(bmp)}, m_clean{clean}
		{
		}
		Bitmap(const Bitmap &) = delete;
		Bitmap(Bitmap &&) = default;
		Bitmap & operator =(const Bitmap &) = delete;
		Bitmap & operator =(Bitmap &&) = default;
		~Bitmap()
		{
			utils::function(m_clean).call_if_exist();
		}
		::Gdiplus::Bitmap & bmp()
		{
			return *m_bmp.get();
		}
		//创建locked数据
		void make_locked()
		{
			if (!m_locked_bmp)
				m_locked_bmp = std::make_unique<utils::windows::ui::gdiplus::inner::LockedBitmap>(bmp());
		}
		void unlock()
		{
			//简单检查
			utils_assert(m_locked_bmp);

			m_locked_bmp.reset();
		}
		void set_background_color(std::optional<COLORREF> color = {})
		{
			m_background_color = color;
			m_quick_find_info.reset();
		}
		//note: 会自动使用some中的背景色（如果存在的话）
		//背景色: 检查时会忽略这种颜色
		//example:
		//	if (screen->is_exist(money, _pos, RGB(255, 0, 0)))
		//		...
		bool is_exist(utils::windows::ui::gdiplus::Bitmap & some, utils::Index2D pos) const
		{
			//简单检查
			utils_assert(some.width() >= 1);
			utils_assert(some.height() >= 1);

			//坐标是否都在本图片里
			utils::Index2D _pos{pos.i + some.width() - 1, pos.j + some.height() - 1};	//右下角坐标
			if(!is_in(pos))		return false;
			if(!is_in(_pos))	return false;

			for(auto x = 0; x < (int64_t)some.width(); x++)
			{
				for(auto y = 0; y < (int64_t)some.height(); y++)
				{
					COLORREF c1 = this->get_pixel(pos.i+x, pos.j+y);
					COLORREF c2 = some.get_pixel(x, y);

					//忽略some中的背景色
					if(some.m_background_color.has_value())
					{
						if(c2 == some.m_background_color.value())
							continue;
					}

					if(c1 != c2)
						return false;
				}
			}
			return true;
		}
		//特化
		bool is_exist(utils::windows::ui::gdiplus::Bitmap & some) const
		{
			auto pos_iterator = utils::iterator::index_2D::iterator(width(), height());
			return utils::algorithm::iterator::is_exist(pos_iterator, [this, &some](const auto & pos)
			{
				return is_exist(some, pos);
			});
		}
		COLORREF get_pixel(int x, int y) const
		{
			if(m_locked_bmp)
				return m_locked_bmp->GetPixel(x,y);
			else
			{
				Gdiplus::Color col;
				{
					auto re = m_bmp->GetPixel(x,y, &col);
					re = re;
				}
				utils_assert(Gdiplus::Status::Ok == m_bmp->GetPixel(x,y, &col));
				return RGB(col.GetR(),col.GetG(),col.GetB());
			}
		}
		//特化
		COLORREF get_pixel(utils::Index2D pos) const
		{
			return get_pixel(pos.i, pos.j);
		}
		void set_pixel(utils::Index2D pos, COLORREF col)
		{
			//简单检查
			utils_assert(m_locked_bmp == nullptr);

			Gdiplus::Color _col{GetRValue(col), GetGValue(col), GetBValue(col)};
			utils_assert(Gdiplus::Status::Ok == m_bmp->SetPixel(pos.i, pos.j, _col));
		}
		uint32_t width() const
		{
			if(m_locked_bmp)
				return m_locked_bmp->GetWidth();
			else
				return m_bmp->GetWidth();
		}
		uint32_t height() const
		{
			if(m_locked_bmp)
				return m_locked_bmp->GetHeight();
			else
				return m_bmp->GetHeight();
		}
		//返回宽和高
		utils::Index2D size() const
		{
			return {width(), height()};
		}
		bool is_in(utils::Index2D pos) const
		{
			//简单检查
			utils_assert(width() >= 1);
			utils_assert(height() >= 1);
		
			if(pos.i < 0 || pos.i >= width())	return false;
			if(pos.j < 0 || pos.j >= height())	return false;
			return true;
		}
		//获得图片中所有的颜色
		std::set<COLORREF> get_all_colors(std::optional<utils::Index2D::Range> range) const
		{
			//准备数据
			if (!range.has_value())
				range = utils::Index2D::Range{{},size()};

			std::set<COLORREF> colors;
			for (const auto pos : range.value())
				colors.insert(get_pixel(pos));
			return colors;
		}
		//算法说明:
		//	1.找出一个不包括背景色的最大的小方块
		//	2.检查小方块的右下角，如果那个位置的颜色没有出现在some中，则说明这个小方块的所有坐标都不用检查
		std::vector<utils::Index2D> find_quick(utils::windows::ui::gdiplus::Bitmap & some) const
		{
			const auto & info = some.get_quick_find_info();

			//简单检查
			utils_assert(info.block.width() >= 2);
			utils_assert(info.block.height() >= 2);

			std::vector<utils::Index2D> positions;
			for(int64_t x = 0; x < width(); x+= info.block.width())
			{
				for(int64_t y = 0; y < height(); y+= info.block.height())
				{
					utils::Index2D pos_右下角{x + info.block.width() - 1,y + info.block.height() - 1};
					if(!is_in(pos_右下角) || !utils::set(info.colors).is_exist(get_pixel(pos_右下角)))
						continue;	//快速跳过这个小方块
					
					for(int64_t i = 0; i < info.block.width(); i++)
					{
						for(int64_t j = 0; j < info.block.height(); j++)
						{
							utils::Index2D pos{x+i,y+j}; pos -= info.block.pos();
							if (is_exist(some, pos))
								positions.push_back(pos);
						}
					}
				}
			}
			return positions;
		}
		Bitmap copy(std::optional<utils::Index2D::Range> range = {}) const
		{
			//todo 增加一个参数，表明想使用原来的像素格式，或者指定像素格式
			//todo 似乎调用之前，需要调用下this->unlock()，不然this析构时，unlock会出错

			//准备数据
			if (!range.has_value())
				range = utils::Index2D::Range{{},size()};

			std::unique_ptr<::Gdiplus::Bitmap> bmp{m_bmp->Clone(
				(INT)range.value().pos().i,
				(INT)range.value().pos().j,
				(INT)range.value().size().i,
				(INT)range.value().size().j,
				PixelFormat24bppRGB)};
			utils_assert(bmp);

			return {std::move(bmp), {}};
		}
		//保存图片
		//note: 文件名需要以bmp/jpg/png结尾
		void save(const std::filesystem::path & filename)
		{
			//准备数据
			auto clsid = utils::windows::ui::gdiplus::helper::get_image_encoder_clsid([&filename]()
			{
				auto _filename = filename.wstring();
				auto view = std::wstring_view(_filename);
				if (view.ends_with(L".bmp")) return L"image/bmp";
				if (view.ends_with(L".png")) return L"image/png";
				if (view.ends_with(L".jpg")) return L"image/jpeg";
				utils_throw();
			}());

			//保存
			utils::real_path::create_its_directory_if_not_exist(filename);
			utils_assert(Gdiplus::Status::Ok == bmp().Save(filename.wstring().c_str(), clsid));
		}
#if 0
		//特化
		void save(const std::string & filename)
		{
			std::wstring unicode;
			utils::character_encoding::convert_cp936_to_unicode(unicode, filename);

			save(unicode.c_str());
		}
#endif
		HBITMAP get_hbitmap()
		{
			HBITMAP hbitmap{};
			utils_assert(Gdiplus::Status::Ok == bmp().GetHBITMAP({}, &hbitmap));	//todo 看看GetHBITMAP() 的第一个参数该怎么给 //todo 看看GetHBITMAP()的返回值用不用释放
			return hbitmap;
		}
	private:
		const s_quick_find_info & get_quick_find_info() const
		{
			if (!m_quick_find_info.has_value())
			{
				auto get_max_block = [this]() -> std::optional<utils::Index2D::Range>
				{
					if (!m_background_color.has_value())
						return utils::Index2D::Range{{}, size()};

					//todo 看看有没有更好的办法

					std::optional<utils::Index2D::Range> result;

					//最小2*2
					utils::Index2D::Range whole{{}, size()};
					for (const auto & a : whole)
					{
						for (const auto & b : whole)
						{
							if (b.i <= a.i || b.j <= a.j) continue;

							utils::Index2D::Range cur{a, b - a + utils::Index2D{1,1}};
							//存在背景色
							if ([&cur, this]()
							{
								for (const auto & pos : cur)
								{
									if(get_pixel(pos) == m_background_color.value())
										return true;
								}
								return false;
							}())
							{
								continue;
							}

							if (!result.has_value() ||
								result.value().size().i * result.value().size().j <
								cur.size().i * cur.size().j)
							{
								result = cur;
							}
						}
					}
					return result;
				};

				std::optional<utils::Index2D::Range> block = get_max_block();
				
				//简单检查
				utils_assert(block.has_value());
				
				std::set<COLORREF> colors = get_all_colors(block.value());

				m_quick_find_info = s_quick_find_info{block.value(), colors};
			}
			return m_quick_find_info.value();
		}
	};

	//截屏
	//hwnd: 要截的窗口, 默认为整个屏幕
	inline Bitmap screenshot(HWND hwnd = 0)
	{
		HDC srcdc, memdc;
		HBITMAP membit;
		srcdc = ::GetDC(hwnd);
		int width{}, height{};
		if (hwnd == 0)
		{
			width = GetSystemMetrics(SM_CXSCREEN);
			height = GetSystemMetrics(SM_CYSCREEN);
		}
		else
		{
			//简单检查
			//utils_assert(false == utils::windows::window::state::is_minimized(hwnd));

			RECT rect{};
			utils_assert(GetClientRect(hwnd, &rect));
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;
		}
		memdc = CreateCompatibleDC(srcdc);
		membit = CreateCompatibleBitmap(srcdc, width, height);
		HBITMAP hOldBitmap = (HBITMAP) SelectObject(memdc, membit);
		BitBlt(memdc, 0,0,width,height,srcdc,0,0,SRCCOPY);
		SelectObject(memdc,hOldBitmap);
		Bitmap bitmap{std::make_unique<::Gdiplus::Bitmap>(membit,(HPALETTE)NULL),[memdc, membit]()
		{
			DeleteObject(membit);
			DeleteObject(memdc);
		}};
		::ReleaseDC(hwnd, srcdc);
		return std::move(bitmap);
	}
}

//对图片的操作
namespace utils::windows::gdiplus::operate
{
#if 0
	//坐标是否在图片里
	bool is_in(::Gdiplus::Bitmap & whole, utils::Index2D pos)
	{
		//简单检查
		utils_assert(whole.GetWidth() >= 1);
		utils_assert(whole.GetHeight() >= 1);
		
		if(pos.i < 0 || pos.i >= whole.GetWidth())	return false;
		if(pos.j < 0 || pos.j >= whole.GetHeight())	return false;
		return true;
	}
	//特化
	bool is_in(utils::windows::gdiplus::LockedBitmap & whole, utils::Index2D pos)
	{
		//简单检查
		utils_assert(whole.GetWidth() >= 1);
		utils_assert(whole.GetHeight() >= 1);
		
		if(pos.i < 0 || pos.i >= whole.GetWidth())	return false;
		if(pos.j < 0 || pos.j >= whole.GetHeight())	return false;
		return true;
	}
	//颜色是否相同
	bool is_same(::Gdiplus::Color & c1, ::Gdiplus::Color & c2)
	{
		return 
			c1.GetR() == c2.GetR() &&
			c1.GetG() == c2.GetG() && 
			c1.GetB() == c2.GetB();
	}
	//特化
	bool is_same(COLORREF & c1, COLORREF & c2)
	{
		return c1 == c2;
	}
	//检查一个图片(some)是否在另一个图片(whole)里
	//background_color: some中的背景色，检查时会忽略some中的这种颜色
	bool is_same(::Gdiplus::Bitmap & whole, ::Gdiplus::Bitmap & some, utils::Index2D pos, std::optional<::Gdiplus::Color> background_color = {})
	{
		//简单检查
		utils_assert(some.GetWidth() >= 1);
		utils_assert(some.GetHeight() >= 1);

		//坐标是否都在whole里
		utils::Index2D _pos{pos.i + some.GetWidth() - 1, pos.j + some.GetHeight() - 1};	//右下角坐标
		if(!is_in(whole, pos))	return false;
		if(!is_in(whole, _pos))	return false;

		for(auto x = 0; x < (int64_t)some.GetWidth(); x++)
		{
			for(auto y = 0; y < (int64_t)some.GetHeight(); y++)
			{
				::Gdiplus::Color c1{};
				utils_assert(::Gdiplus::Status::Ok == whole.GetPixel(pos.i+x, pos.j+y, &c1));
				::Gdiplus::Color c2{};
				utils_assert(::Gdiplus::Status::Ok == some.GetPixel(x,y, &c2));

				//忽略some中的背景色
				if(background_color.has_value())
				{
					if(is_same(c2,background_color.value()))
						continue;
				}

				if(!is_same(c1,c2))
				{
					return false;
				}
			}
		}
		return true;
	}
	//特化: 快速版
	bool is_same(utils::windows::gdiplus::LockedBitmap & whole, utils::windows::gdiplus::LockedBitmap & some, utils::Index2D pos, std::optional<COLORREF> background_color = {})
	{
		//简单检查
		utils_assert(some.GetWidth() >= 1);
		utils_assert(some.GetHeight() >= 1);

		//坐标是否都在whole里
		utils::Index2D _pos{pos.i + some.GetWidth() - 1, pos.j + some.GetHeight() - 1};	//右下角坐标
		if(!is_in(whole, pos))	return false;
		if(!is_in(whole, _pos))	return false;

		for(auto x = 0; x < (int64_t)some.GetWidth(); x++)
		{
			for(auto y = 0; y < (int64_t)some.GetHeight(); y++)
			{
				COLORREF c1 = whole.GetPixel(pos.i+x, pos.j+y);
				COLORREF c2 = some.GetPixel(x, y);

				//忽略some中的背景色
				if(background_color.has_value())
				{
					if(is_same(c2,background_color.value()))
						continue;
				}

				if(!is_same(c1,c2))
				{
					return false;
				}
			}
		}
		return true;
	}

#if 0
	//查找图片位置
	template<typename T_iterator>
	std::optional<utils::Index2D> find(::Gdiplus::Bitmap & whole, ::Gdiplus::Bitmap & some, T_iterator pos_iterator, const ::Gdiplus::Color & background_color)
	{
		for(auto pos : pos_iterator)
		{
			if(is_same(whole, some, pos, background_color))
				return pos;
		}
		return {};
	}
#endif
#endif
}
#pragma once

#include <utils/u8string.hpp>
#include <utils/text/hex.hpp>
#include <utils/map.hpp>

namespace utils::opcode::dalvik::common::instruction
{
	enum class Format
	{
		_10x,
		_10t,
		_11n,
		_11x,
		_12x,
		_20t,
		_21c,
		_21h,
		_21s,
		_21t,
		_22b,
		_22c,
		_22s,
		_22t,
		_22x,
		_23x,
		_30t,
		_31i,
		_31t,
		_35c,
		_3rc,
		_51l,
		todo,
	};
	struct s_format
	{
		u8 size;
		Format format;
		std::u8string text;
	};
	struct s_flags
	{
		bool is_continue;
		bool is_throw;
		bool is_return;
		bool is_switch;
		bool is_invoke;
		bool is_branch;
	};
	enum class PoolType
	{
		None,
		String,
		Type,
		Field,
		Method,
	};
	struct s_opcode_desc
	{
		u1 opcode;
		std::u8string name;
		s_format format;
		s_flags flags;
		PoolType pool_type;
	};
	namespace inner
	{
		inline const std::u8string & get_table_text()
		{
			//refer: https://github.com/aosp-mirror/platform_dalvik/blob/main/opcode-gen/bytecode.txt

			static const std::u8string table_text = u8R"(
op   00 nop                         10x  n none          continue
op   01 move                        12x  y none          continue
op   02 move/from16                 22x  y none          continue
op   03 move/16                     32x  y none          continue
op   04 move-wide                   12x  y none          continue
op   05 move-wide/from16            22x  y none          continue
op   06 move-wide/16                32x  y none          continue
op   07 move-object                 12x  y none          continue
op   08 move-object/from16          22x  y none          continue
op   09 move-object/16              32x  y none          continue
op   0a move-result                 11x  y none          continue
op   0b move-result-wide            11x  y none          continue
op   0c move-result-object          11x  y none          continue
op   0d move-exception              11x  y none          continue
op   0e return-void                 10x  n none          return
op   0f return                      11x  n none          return
op   10 return-wide                 11x  n none          return
op   11 return-object               11x  n none          return
op   12 const/4                     11n  y none          continue
op   13 const/16                    21s  y none          continue
op   14 const                       31i  y none          continue
op   15 const/high16                21h  y none          continue
op   16 const-wide/16               21s  y none          continue
op   17 const-wide/32               31i  y none          continue
op   18 const-wide                  51l  y none          continue
op   19 const-wide/high16           21h  y none          continue
op   1a const-string                21c  y string-ref    continue|throw
op   1b const-string/jumbo          31c  y string-ref    continue|throw
op   1c const-class                 21c  y type-ref      continue|throw
op   1d monitor-enter               11x  n none          continue|throw
op   1e monitor-exit                11x  n none          continue|throw
op   1f check-cast                  21c  y type-ref      continue|throw
op   20 instance-of                 22c  y type-ref      continue|throw
op   21 array-length                12x  y none          continue|throw
op   22 new-instance                21c  y type-ref      continue|throw
op   23 new-array                   22c  y type-ref      continue|throw
op   24 filled-new-array            35c  n type-ref      continue|throw
op   25 filled-new-array/range      3rc  n type-ref      continue|throw
op   26 fill-array-data             31t  n none          continue
op   27 throw                       11x  n none          throw
op   28 goto                        10t  n none          branch
op   29 goto/16                     20t  n none          branch
op   2a goto/32                     30t  n none          branch
op   2b packed-switch               31t  n none          continue|switch
op   2c sparse-switch               31t  n none          continue|switch
op   2d cmpl-float                  23x  y none          continue
op   2e cmpg-float                  23x  y none          continue
op   2f cmpl-double                 23x  y none          continue
op   30 cmpg-double                 23x  y none          continue
op   31 cmp-long                    23x  y none          continue
op   32 if-eq                       22t  n none          continue|branch
op   33 if-ne                       22t  n none          continue|branch
op   34 if-lt                       22t  n none          continue|branch
op   35 if-ge                       22t  n none          continue|branch
op   36 if-gt                       22t  n none          continue|branch
op   37 if-le                       22t  n none          continue|branch
op   38 if-eqz                      21t  n none          continue|branch
op   39 if-nez                      21t  n none          continue|branch
op   3a if-ltz                      21t  n none          continue|branch
op   3b if-gez                      21t  n none          continue|branch
op   3c if-gtz                      21t  n none          continue|branch
op   3d if-lez                      21t  n none          continue|branch
op   44 aget                        23x  y none          continue|throw
op   45 aget-wide                   23x  y none          continue|throw
op   46 aget-object                 23x  y none          continue|throw
op   47 aget-boolean                23x  y none          continue|throw
op   48 aget-byte                   23x  y none          continue|throw
op   49 aget-char                   23x  y none          continue|throw
op   4a aget-short                  23x  y none          continue|throw
op   4b aput                        23x  n none          continue|throw
op   4c aput-wide                   23x  n none          continue|throw
op   4d aput-object                 23x  n none          continue|throw
op   4e aput-boolean                23x  n none          continue|throw
op   4f aput-byte                   23x  n none          continue|throw
op   50 aput-char                   23x  n none          continue|throw
op   51 aput-short                  23x  n none          continue|throw
op   52 iget                        22c  y field-ref     continue|throw
op   53 iget-wide                   22c  y field-ref     continue|throw
op   54 iget-object                 22c  y field-ref     continue|throw
op   55 iget-boolean                22c  y field-ref     continue|throw
op   56 iget-byte                   22c  y field-ref     continue|throw
op   57 iget-char                   22c  y field-ref     continue|throw
op   58 iget-short                  22c  y field-ref     continue|throw
op   59 iput                        22c  n field-ref     continue|throw
op   5a iput-wide                   22c  n field-ref     continue|throw
op   5b iput-object                 22c  n field-ref     continue|throw
op   5c iput-boolean                22c  n field-ref     continue|throw
op   5d iput-byte                   22c  n field-ref     continue|throw
op   5e iput-char                   22c  n field-ref     continue|throw
op   5f iput-short                  22c  n field-ref     continue|throw
op   60 sget                        21c  y field-ref     continue|throw
op   61 sget-wide                   21c  y field-ref     continue|throw
op   62 sget-object                 21c  y field-ref     continue|throw
op   63 sget-boolean                21c  y field-ref     continue|throw
op   64 sget-byte                   21c  y field-ref     continue|throw
op   65 sget-char                   21c  y field-ref     continue|throw
op   66 sget-short                  21c  y field-ref     continue|throw
op   67 sput                        21c  n field-ref     continue|throw
op   68 sput-wide                   21c  n field-ref     continue|throw
op   69 sput-object                 21c  n field-ref     continue|throw
op   6a sput-boolean                21c  n field-ref     continue|throw
op   6b sput-byte                   21c  n field-ref     continue|throw
op   6c sput-char                   21c  n field-ref     continue|throw
op   6d sput-short                  21c  n field-ref     continue|throw
op   6e invoke-virtual              35c  n method-ref    continue|throw|invoke
op   6f invoke-super                35c  n method-ref    continue|throw|invoke
op   70 invoke-direct               35c  n method-ref    continue|throw|invoke
op   71 invoke-static               35c  n method-ref    continue|throw|invoke
op   72 invoke-interface            35c  n method-ref    continue|throw|invoke
op   74 invoke-virtual/range        3rc  n method-ref    continue|throw|invoke
op   75 invoke-super/range          3rc  n method-ref    continue|throw|invoke
op   76 invoke-direct/range         3rc  n method-ref    continue|throw|invoke
op   77 invoke-static/range         3rc  n method-ref    continue|throw|invoke
op   78 invoke-interface/range      3rc  n method-ref    continue|throw|invoke
op   7b neg-int                     12x  y none          continue
op   7c not-int                     12x  y none          continue
op   7d neg-long                    12x  y none          continue
op   7e not-long                    12x  y none          continue
op   7f neg-float                   12x  y none          continue
op   80 neg-double                  12x  y none          continue
op   81 int-to-long                 12x  y none          continue
op   82 int-to-float                12x  y none          continue
op   83 int-to-double               12x  y none          continue
op   84 long-to-int                 12x  y none          continue
op   85 long-to-float               12x  y none          continue
op   86 long-to-double              12x  y none          continue
op   87 float-to-int                12x  y none          continue
op   88 float-to-long               12x  y none          continue
op   89 float-to-double             12x  y none          continue
op   8a double-to-int               12x  y none          continue
op   8b double-to-long              12x  y none          continue
op   8c double-to-float             12x  y none          continue
op   8d int-to-byte                 12x  y none          continue
op   8e int-to-char                 12x  y none          continue
op   8f int-to-short                12x  y none          continue
op   90 add-int                     23x  y none          continue
op   91 sub-int                     23x  y none          continue
op   92 mul-int                     23x  y none          continue
op   93 div-int                     23x  y none          continue|throw
op   94 rem-int                     23x  y none          continue|throw
op   95 and-int                     23x  y none          continue
op   96 or-int                      23x  y none          continue
op   97 xor-int                     23x  y none          continue
op   98 shl-int                     23x  y none          continue
op   99 shr-int                     23x  y none          continue
op   9a ushr-int                    23x  y none          continue
op   9b add-long                    23x  y none          continue
op   9c sub-long                    23x  y none          continue
op   9d mul-long                    23x  y none          continue
op   9e div-long                    23x  y none          continue|throw
op   9f rem-long                    23x  y none          continue|throw
op   a0 and-long                    23x  y none          continue
op   a1 or-long                     23x  y none          continue
op   a2 xor-long                    23x  y none          continue
op   a3 shl-long                    23x  y none          continue
op   a4 shr-long                    23x  y none          continue
op   a5 ushr-long                   23x  y none          continue
op   a6 add-float                   23x  y none          continue
op   a7 sub-float                   23x  y none          continue
op   a8 mul-float                   23x  y none          continue
op   a9 div-float                   23x  y none          continue
op   aa rem-float                   23x  y none          continue
op   ab add-double                  23x  y none          continue
op   ac sub-double                  23x  y none          continue
op   ad mul-double                  23x  y none          continue
op   ae div-double                  23x  y none          continue
op   af rem-double                  23x  y none          continue
op   b0 add-int/2addr               12x  y none          continue
op   b1 sub-int/2addr               12x  y none          continue
op   b2 mul-int/2addr               12x  y none          continue
op   b3 div-int/2addr               12x  y none          continue|throw
op   b4 rem-int/2addr               12x  y none          continue|throw
op   b5 and-int/2addr               12x  y none          continue
op   b6 or-int/2addr                12x  y none          continue
op   b7 xor-int/2addr               12x  y none          continue
op   b8 shl-int/2addr               12x  y none          continue
op   b9 shr-int/2addr               12x  y none          continue
op   ba ushr-int/2addr              12x  y none          continue
op   bb add-long/2addr              12x  y none          continue
op   bc sub-long/2addr              12x  y none          continue
op   bd mul-long/2addr              12x  y none          continue
op   be div-long/2addr              12x  y none          continue|throw
op   bf rem-long/2addr              12x  y none          continue|throw
op   c0 and-long/2addr              12x  y none          continue
op   c1 or-long/2addr               12x  y none          continue
op   c2 xor-long/2addr              12x  y none          continue
op   c3 shl-long/2addr              12x  y none          continue
op   c4 shr-long/2addr              12x  y none          continue
op   c5 ushr-long/2addr             12x  y none          continue
op   c6 add-float/2addr             12x  y none          continue
op   c7 sub-float/2addr             12x  y none          continue
op   c8 mul-float/2addr             12x  y none          continue
op   c9 div-float/2addr             12x  y none          continue
op   ca rem-float/2addr             12x  y none          continue
op   cb add-double/2addr            12x  y none          continue
op   cc sub-double/2addr            12x  y none          continue
op   cd mul-double/2addr            12x  y none          continue
op   ce div-double/2addr            12x  y none          continue
op   cf rem-double/2addr            12x  y none          continue
op   d0 add-int/lit16               22s  y none          continue
op   d1 rsub-int                    22s  y none          continue
op   d2 mul-int/lit16               22s  y none          continue
op   d3 div-int/lit16               22s  y none          continue|throw
op   d4 rem-int/lit16               22s  y none          continue|throw
op   d5 and-int/lit16               22s  y none          continue
op   d6 or-int/lit16                22s  y none          continue
op   d7 xor-int/lit16               22s  y none          continue
op   d8 add-int/lit8                22b  y none          continue
op   d9 rsub-int/lit8               22b  y none          continue
op   da mul-int/lit8                22b  y none          continue
op   db div-int/lit8                22b  y none          continue|throw
op   dc rem-int/lit8                22b  y none          continue|throw
op   dd and-int/lit8                22b  y none          continue
op   de or-int/lit8                 22b  y none          continue
op   df xor-int/lit8                22b  y none          continue
op   e0 shl-int/lit8                22b  y none          continue
op   e1 shr-int/lit8                22b  y none          continue
op   e2 ushr-int/lit8               22b  y none          continue
)";
			return table_text;
		}
		inline Format decode_Format(const std::u8string & text)
		{
			if(text == u8"10x") return Format::_10x;
			if(text == u8"10t") return Format::_10t;
			if(text == u8"11n") return Format::_11n;
			if(text == u8"11x") return Format::_11x;
			if(text == u8"12x") return Format::_12x;
			if(text == u8"20t") return Format::_20t;
			if(text == u8"21c") return Format::_21c;
			if(text == u8"21h") return Format::_21h;
			if(text == u8"21s") return Format::_21s;
			if(text == u8"21t") return Format::_21t;
			if(text == u8"22b") return Format::_22b;
			if(text == u8"22c") return Format::_22c;
			if(text == u8"22s") return Format::_22s;
			if(text == u8"22t") return Format::_22t;
			if(text == u8"22x") return Format::_22x;
			if(text == u8"23x") return Format::_23x;
			if(text == u8"30t") return Format::_30t;
			if(text == u8"31i") return Format::_31i;
			if(text == u8"31t") return Format::_31t;
			if(text == u8"35c") return Format::_35c;
			if(text == u8"3rc") return Format::_3rc;
			if(text == u8"51l") return Format::_51l;

			
			if(text == u8"31c") return Format::todo;
			if(text == u8"32x") return Format::todo;
			utils_throw();
		}
	};
	inline const std::map<u1, s_opcode_desc> & get_opcode_desc_table()
	{
		static std::map<u1, s_opcode_desc> opcode_desc_table = []()
		{
			const auto & table_text = inner::get_table_text();

			auto decode_format = [](const std::u8string & text)
			{
				//简单检查
				utils_assert(text.size() == 3);
				utils_assert(text[0] >= u8'0' && text[0] <= u8'9');

				s_format format{};
				format.size = text[0] - u8'0';
				format.text = text;
				format.format = inner::decode_Format(text);
				return format;
			};
			auto decode_pool_type = [](const std::u8string & text)
			{
				if(text == u8"none") return PoolType::None;
				if(text == u8"string-ref") return PoolType::String;
				if(text == u8"type-ref") return PoolType::Type;
				if(text == u8"field-ref") return PoolType::Field;
				if(text == u8"method-ref") return PoolType::Method;
				utils_throw();
			};
			auto decode_flags = [](const std::u8string & text)
			{
				auto fields = utils::u8string(text).split(u8"|");

				//简单检查
				utils_assert(!fields.empty());
				
				s_flags flags{};
				for(const auto & one : fields)
				{
					if (one == u8"continue") flags.is_continue = true;
					else if (one == u8"return") flags.is_return = true;
					else if (one == u8"throw") flags.is_throw = true;
					else if (one == u8"invoke") flags.is_invoke = true;
					else if (one == u8"switch") flags.is_switch = true;
					else if (one == u8"branch") flags.is_branch = true;
					else utils_throw();
				}
				return flags;
			};

			const auto lines = utils::u8string(table_text).split(u8"\n", false);
			std::map<u1, s_opcode_desc> opcode_desc_table;
			for(const auto & line : lines)
			{
				auto fields = utils::u8string(line).split(u8" ", false);

				//简单检查
				utils_assert(fields.size() == 7);
				utils_assert(fields[0] == u8"op");
				utils_assert(fields[1].size() == 2);

				s_opcode_desc one{};
				{
					one.name = fields[2];
					utils::text::hex::from_text(one.opcode, fields[1], true);
					one.format = decode_format(fields[3]);
					one.pool_type = decode_pool_type(fields[5]);
					one.flags = decode_flags(fields[6]);
				}
				utils::map(opcode_desc_table).insert_not_exist(one.opcode, std::move(one));
			}
			return opcode_desc_table;
		}();

		return opcode_desc_table;
	}
	inline const s_opcode_desc & get_opcode_desc(u1 opcode)
	{
		return utils::map(get_opcode_desc_table()).get_existed_value(opcode);
	}
}
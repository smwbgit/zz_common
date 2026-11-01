#pragma once


#if 0

dex_code.debug_info_off
dex_code.tries
dex_code.catch_handlers

code
dex_code.catch_handlers.code

class_def.annotations_off
class_def.static_values_off

#endif


#include <utils/data_encoding/uleb128.hpp>
#include <utils/FileReader.hpp>
#include <utils/FileWriter.hpp>
#include <utils/basic/bits.hpp>
#include <utils/text/combine.hpp>
#include <utils/integer_type.hpp>
#include <utils/opcode/dalvik/decode.hpp>
#include <utils/vector.hpp>

namespace utils::file_format::dex
{
	namespace raw
	{
		#pragma pack(push, 1)
		struct FileHeader
		{
			u1 magic[8];
			u4 checksum;
			u1 signature[20];
			u4 file_size;
			u4 header_size;
			u4 endian_tag;
			u4 link_size, link_off;
			u4 map_off;
			u4 string_ids_size, string_ids_off;
			u4 type_ids_size,  type_ids_off;
			u4 proto_ids_size, proto_ids_off;
			u4 field_ids_size, field_ids_off;
			u4 method_ids_size,method_ids_off;
			u4 class_defs_size,class_defs_off;
			u4 data_size, data_off;
		};
		struct ProtoId
		{
			u4 shortly_idx;         // 字符串池索引：完整签名
			u4 return_type_idx;     // type_ids 索引：返回类型
			u4 parameters_off;      // 0 表示无参数；否则指向 DexTypeList
		};
#if 0
		struct DexTypeList
		{
			u4  size;               // 参数个数
			u2  list[size];         // 每个元素是 type_ids 索引
		};
#endif
		struct MethodId
		{
			u2 class_idx;
			u2 proto_idx;
			u4 name_idx;
		};
		struct FieldId
		{
			u2 class_idx;
			u2 type_idx;
			u4 name_idx;
		};
		struct ClassDef
		{
			u4 class_idx;
			u4 access_flags;
			u4 superclass_idx;
			u4 interfaces_off;
			u4 source_file_idx;
			u4 annotations_off;
			u4 class_data_off;
			u4 static_values_off;
		};
		struct Try
		{
			u4 start_addr;
			u2 insn_count;
			u2 handler_off;
		};
		#pragma pack(pop)
		
		struct CatchHandlerGroup
		{
			int64_t size;
			std::vector<u4> type_idxs;	// 异常类型索引
			std::vector<u4> addr_idxs;	// 对应 catch 地址
			u4 catch_all_addr;
		};
		struct EncodedField
		{
			u4 field_idx;
			u4 access_flags;
		};
		struct EncodedMethod
		{
			u4 method_idx;
			u4 access_flags;
			u4 code_off;
		};
		struct ClassData
		{
			u4 static_field_size;
			u4 instance_field_size;
			u4 direct_method_size;
			u4 virtual_method_size;
			std::vector<EncodedField>  static_fields;
			std::vector<EncodedField>  instance_fields;
			std::vector<EncodedMethod> direct_methods;
			std::vector<EncodedMethod> virtual_methods;
		};
	}

	//一个dex字符串
	struct DexString
	{
		u8 length;
		std::u8string text;
	};
	//一个dex原型
	struct DexProto
	{
		DexString shortly;
		DexString return_type;
		std::vector<DexString> params;
	};
	//一个dex字段
	struct DexField
	{
		DexString name;
		DexString type;
		DexString of_class;
	};
	//一个dex方法
	struct DexMethod
	{
		DexString name;
		DexProto proto;
		DexString of_class;
	};
	struct DexAccessFlags
	{
		bool is_public                ;
		bool is_private               ;
		bool is_protected             ;
		bool is_static                ;
		bool is_final                 ;
		bool is_synchronized          ;
		bool is_volatile              ;
		bool is_bridge                ;
		bool is_transient             ;
		bool is_varargs               ;
		bool is_native                ;
		bool is_interface             ;
		bool is_abstract              ;
		bool is_strict                ;
		bool is_synthetic             ;
		bool is_annotation            ;
		bool is_enum                  ;
		bool is_constructor           ;
		bool is_declared_synchronized ;
	};
	struct DexHandlerGroup
	{
		u4 start_offset;
		std::vector<DexString> types;
		std::vector<u4> addr_idxs;
		std::optional<u4> catch_all_addr;
	};
	struct DexCode
	{
		u2 registers_size;
		u2 ins_size;
		u2 outs_size;
		u4 debug_info_off;
		std::vector<u2> insns;
		std::vector<raw::Try> tries;
		std::vector<DexHandlerGroup> catch_handlers;
		std::optional<utils::opcode::dalvik::decode::s_function_code> decoded_code;
	};
	//一个dex字段
	struct DexField2
	{
		DexString name;
		DexString type;
		DexAccessFlags access_flags;
	};
	//一个dex方法
	struct DexMethod2
	{
		DexString name;
		DexProto proto;
		DexAccessFlags access_flags;
		std::optional<DexCode> code;
	};
	//一个dex类
	struct DexClass
	{
		DexString name;
		DexAccessFlags access_flags;
		DexString superclass;
		DexString source_file;
		std::vector<DexString> interfaces;
		std::vector<DexField2> static_fields;
		std::vector<DexField2> instance_fields;
		std::vector<DexMethod2> direct_methods;
		std::vector<DexMethod2> virtual_methods;
	};

	//读取dex文件
	class DexReader
	{
	private:
		utils::FileReader m_input;
		std::optional<raw::FileHeader> m_file_header;
		std::optional<std::vector<DexString>> m_string_table;
		std::optional<std::vector<DexString>> m_type_table;
		std::optional<std::vector<DexMethod>> m_method_table;
		std::optional<std::vector<DexProto>> m_proto_table;
		std::optional<std::vector<DexField>> m_field_table;
		std::optional<std::vector<DexClass>> m_class_table;

	public:
		DexReader(const std::u8string & filename)
			: m_input{filename}
		{
		}
	public:
		//获取文件头
		const raw::FileHeader & get_file_header()
		{
			//读取
			if (!m_file_header)
			{
				m_file_header = m_input.read<raw::FileHeader>();

				//简单检查
				utils_assert(m_file_header->magic[0] == 'd');
				utils_assert(m_file_header->magic[1] == 'e');
				utils_assert(m_file_header->magic[2] == 'x');
				utils_assert(m_file_header->magic[3] == '\n');
				utils_assert(m_file_header->magic[4] == '0');
				utils_assert(m_file_header->magic[5] == '3');
				utils_assert(m_file_header->magic[6] == '5');
				utils_assert(m_file_header->magic[7] == 0);
				utils_assert(m_file_header->header_size == sizeof(raw::FileHeader));
			}

			//给出
			return m_file_header.value();
		}
		//获取所有字符串
		const std::vector<DexString> & get_string_table()
		{
			//读取
			if(!m_string_table)
			{
				const auto & file_header = get_file_header();
				m_input.seekg(file_header.string_ids_off);
				std::vector<uint32_t> string_id_table;
				m_input.read(string_id_table, file_header.string_ids_size);
	
				std::vector<DexString> string_table;
				for (const auto & string_id : string_id_table)
				{
					auto string = read_dex_string(string_id);
					string_table.push_back(std::move(string));
				}
				m_string_table = std::move(string_table);
			}

			//给出
			return m_string_table.value();
		}
		//输出所有字符串
		std::u8string dump_string_table()
		{
			std::u8string dump_text;
			const auto & string_table = get_string_table();
			for(const auto & one : string_table)
				dump_text += dump_string(one) + u8"\n";
			return dump_text;
		}
		//获取所有type
		const std::vector<DexString> & get_type_table()
		{
			//读取
			if(!m_type_table)
			{
				//type_id表
				const auto & file_header = get_file_header();
				m_input.seekg(file_header.type_ids_off);
				std::vector<uint32_t> type_id_table;
				m_input.read(type_id_table, file_header.type_ids_size);
			
				//type表
				std::vector<DexString> type_table;
				for (const auto & type_id : type_id_table)
				{
					type_table.push_back(get_string_from_string_table(type_id));
				}

				//简单检查: 类的父类，用0表示Object，所以猜测第一个type不能是某个类的类型
				utils_assert(!type_table.empty());
				utils_assert(!type_table[0].text.empty());
				utils_assert(type_table[0].text[0] != u8'L');

				m_type_table = type_table;
			}

			//给出
			return m_type_table.value();
		}
		//输出所有type
		std::u8string dump_type_table()
		{
			std::u8string dump_text;
			const auto & table = get_type_table();
			for(const auto & one : table)
				dump_text += dump_string(one) + u8"\n";
			return dump_text;
		}
		//获取所有proto
		const std::vector<DexProto> & get_proto_table()
		{
			//读取
			if(!m_proto_table)
			{
				//proto_id表
				const auto & file_header = get_file_header();
				std::vector<raw::ProtoId> proto_id_table;
				m_input.seekg(file_header.proto_ids_off);
				m_input.read(proto_id_table, file_header.proto_ids_size);
			
				//proto表
				std::vector<DexProto> proto_table;
				for (const auto & proto_id : proto_id_table)
				{
					DexProto one{};
					{
						one.shortly = get_string_from_string_table(proto_id.shortly_idx);
						one.return_type = get_string_from_type_table(proto_id.return_type_idx);

						if(proto_id.parameters_off)
							one.params = read_type_list(proto_id.parameters_off);
					}
					proto_table.push_back(std::move(one));
				}
				m_proto_table = proto_table;
			}

			//给出
			return m_proto_table.value();
		}
		//输出所有proto
		std::u8string dump_proto_table()
		{
			std::u8string dump_text;
			const auto & table = get_proto_table();
			for(const auto & one : table)
				dump_text += dump_proto(one) + u8"\n";
			return dump_text;
		}
		//获取所有method
		const std::vector<DexMethod> & get_method_table()
		{
			//读取
			if(!m_method_table)
			{
				//method_id
				const auto & file_header = get_file_header();
				std::vector<raw::MethodId> method_id_table;
				m_input.seekg(file_header.method_ids_off);
				m_input.read(method_id_table, file_header.method_ids_size);
			
				//method
				std::vector<DexMethod> method_table;
				for(const auto & method_id : method_id_table)
				{
					DexMethod one{};
					{
						one.name = get_string_from_string_table(method_id.name_idx);
						one.proto = get_element_from_proto_table(method_id.proto_idx);
						one.of_class = get_string_from_type_table(method_id.class_idx);
					}
					method_table.push_back(one);
				}
				m_method_table = method_table;
			}

			//给出
			return m_method_table.value();
		}
		//输出所有method
		std::u8string dump_method_table()
		{
			std::u8string dump_text;
			const auto & table = get_method_table();
			for(const auto & one : table)
			{
				dump_text += dump_string(one.name) + u8" ";
				dump_text += dump_proto(one.proto) + u8" in ";
				dump_text += dump_string(one.of_class) + u8"\n";
			}
			return dump_text;
		}
		//获取所有field
		const std::vector<DexField> & get_field_table()
		{
			//读取
			if(!m_field_table)
			{
				//field_id表
				const auto & file_header = get_file_header();
				std::vector<raw::FieldId> field_id_table;
				m_input.seekg(file_header.field_ids_off);
				m_input.read(field_id_table, file_header.field_ids_size);
			
				//field表
				std::vector<DexField> field_table;
				for (const auto & field_id : field_id_table)
				{
					DexField one{};
					{
						one.name = get_string_from_string_table(field_id.name_idx);
						one.type = get_string_from_type_table(field_id.type_idx);
						one.of_class = get_string_from_type_table(field_id.class_idx);
					}
					field_table.push_back(std::move(one));
				}
				m_field_table = field_table;
			}

			//给出
			return m_field_table.value();
		}
		//输出所有field
		std::u8string dump_field_table()
		{
			std::u8string dump_text;
			const auto & table = get_field_table();
			for(const auto & one : table)
			{
				dump_text += dump_string(one.name) + u8" ";
				dump_text += dump_string(one.type) + u8" ";
				dump_text += u8"in " + dump_string(one.of_class) + u8"\n";
			}
			return dump_text;
		}
		//获取所有class
		const std::vector<DexClass> & get_class_table()
		{
			//读取
			if(!m_class_table)
			{
				//class_def表
				const auto & file_header = get_file_header();
				std::vector<raw::ClassDef> class_def_table;
				m_input.seekg(file_header.class_defs_off);
				m_input.read(class_def_table, file_header.class_defs_size);
			
				//class表
				std::vector<DexClass> class_table;
				for (const auto & class_def : class_def_table)
				{
					DexClass one{};
					{
						one.name = get_string_from_type_table(class_def.class_idx);
						one.access_flags = get_access_flags_for_class(class_def.access_flags);
						if(class_def.superclass_idx == 0)
						{
							one.superclass = get_dex_string(u8"Ljava/lang/Object;");
						}
						else
						{
							one.superclass = get_string_from_type_table(class_def.superclass_idx);

							//即使可以用superclass_idx==0表示Object，似乎也有superclass_idx!=0,superclass_idx直接指向Object字符串的
						}
						if (class_def.interfaces_off)
							one.interfaces = read_type_list(class_def.interfaces_off);
						if(class_def.source_file_idx == (u4)(-1))
							one.source_file = get_dex_string(u8"unknown");
						else
							one.source_file = get_string_from_string_table(class_def.source_file_idx);
						if (class_def.annotations_off)
						{
							//todo: annotations_off	uint32	类注解偏移（0=无注解）
						}
						if(class_def.class_data_off)
						{
							//读取
							auto class_data = read_raw_class_data(class_def.class_data_off);
							
							//根据field_table和class_data，填充
							const auto & field_table = get_field_table();
							auto fill_fields = [&field_table, &one, this](std::vector<DexField2> & fields, const std::vector<raw::EncodedField> & encoded_fields)
							{
								for(const auto & encoded_field : encoded_fields)
								{
									//简单检查
									utils_assert(encoded_field.field_idx < field_table.size());

									const auto & field_item = field_table[encoded_field.field_idx];

									//简单检查
									utils_assert(field_item.of_class.text == one.name.text);

									DexField2 field{};
									field.name = field_item.name;
									field.type = field_item.type;
									field.access_flags = get_access_flags_for_field(encoded_field.access_flags);
									fields.push_back(field);
								}
							};
							fill_fields(one.static_fields, class_data.static_fields);
							fill_fields(one.instance_fields, class_data.instance_fields);
							
							//根据method_table和class_data，填充
							const auto & method_table = get_method_table();
							auto fill_methods = [&method_table, &one, this](std::vector<DexMethod2> & methods, const std::vector<raw::EncodedMethod> & encoded_methods)
							{
								for(const auto & encoded_method : encoded_methods)
								{
									//简单检查
									utils_assert(encoded_method.method_idx < method_table.size());

									const auto & method_item = method_table[encoded_method.method_idx];

									//简单检查
									utils_assert(method_item.of_class.text == one.name.text);

									DexMethod2 method{};
									method.name = method_item.name;
									method.proto= method_item.proto;
									method.code = read_dex_code(encoded_method.code_off);
									method.access_flags = get_access_flags_for_method(encoded_method.access_flags);
									methods.push_back(method);
								}
							};
							fill_methods(one.direct_methods, class_data.direct_methods);
							fill_methods(one.virtual_methods, class_data.virtual_methods);
						}
						if (class_def.static_values_off)
						{
							//todo: static_values_off	uint32	静态字段初始值偏移（0=无初始值）
						}
					}
					class_table.push_back(std::move(one));
				}
				m_class_table = class_table;
			}

			//给出
			return m_class_table.value();
		}
		//输出所有class
		std::u8string dump_class_table()
		{
			std::u8string dump_text;
			const auto & table = get_class_table();
			for(const auto & one : table)
			{
				dump_text += dump_string(one.name) + u8"\n";
				dump_text += u8"\taccess_flags: " + dump_access_flags(one.access_flags, false) + u8"\n";
				dump_text += u8"\tsuperclass: " + dump_string(one.superclass) + u8"\n";
				dump_text += u8"\tsource_file: " + dump_string(one.source_file) + u8"\n";
				dump_text += u8"\tinterfaces:\n";
				for(const auto & one_interface : one.interfaces)
					dump_text += u8"\t\t" + dump_string(one_interface) + u8"\n";
				dump_text += u8"\tclass_data:\n";

				auto dump_field = [&dump_text, this, &one](const DexField2 & field)
				{
					dump_text += u8"\t\t\t";
					dump_text += dump_string(field.name) + u8" ";
					dump_text += dump_string(field.type) + u8" ";
					dump_text += dump_access_flags(field.access_flags, true);
					dump_text += u8"\n";
				};
				dump_text += u8"\t\tstatic_fields:\n";
				for(const auto & field : one.static_fields)
					dump_field(field);
				dump_text += u8"\t\tinstance_fields:\n";
				for(const auto & field : one.instance_fields)
					dump_field(field);

				auto dump_method = [&dump_text, this, &one](const DexMethod2 & method)
				{
					dump_text += u8"\t\t\t";
					dump_text += dump_string(method.name) + u8" ";
					dump_text += dump_proto(method.proto) + u8" ";
					dump_text += dump_access_flags(method.access_flags, true);
					dump_text += u8"\n";
					dump_text += dump_dex_code(method.code);
				};
				dump_text += u8"\t\tdirect_methods:\n";
				for(const auto & method : one.direct_methods)
					dump_method(method);
				dump_text += u8"\t\tvirtual_methods:\n";
				for(const auto & method : one.virtual_methods)
					dump_method(method);
			}
			return dump_text;
		}
	private:
		//读取string_id指向的字符串
		DexString read_dex_string(uint32_t string_id)
		{
			//读取length
			m_input.seekg(string_id);
			uint64_t length = read_uleb128();
			
			//读取text
			std::u8string text;
			m_input.text_u8.read_until(text, {0}, false);

			//保存
			DexString re{};
			re.length = length;
			re.text = text;

			//给出
			return re;
		}
		std::u8string dump_string(const DexString & dex_string)
		{
			return dex_string.text;
		}
		std::u8string dump_proto(const DexProto & dex_proto)
		{
			std::u8string dump_text;
			dump_text += dump_string(dex_proto.shortly) + u8" ";
			dump_text += dump_string(dex_proto.return_type) + u8"(";
			bool is_first{true};
			for(const auto & one_param : dex_proto.params)
			{
				if(is_first) is_first = false;
				else dump_text += u8", ";
				dump_text += dump_string(one_param);
			}
			dump_text += u8")";
			return dump_text;
		}
		const DexString & get_string_from_string_table(uint32_t index)
		{
			//准备数据
			const auto & string_table = get_string_table();
			
			//简单检查
			utils_assert(index < string_table.size());

			//给出
			return string_table[index];
		}
		const DexString & get_string_from_type_table(uint32_t index)
		{
			//准备数据
			const auto & type_table = get_type_table();
			
			//简单检查
			utils_assert(index < type_table.size());

			//给出
			return type_table[index];
		}
		const DexProto & get_element_from_proto_table(uint32_t index)
		{
			//准备数据
			const auto & proto_table = get_proto_table();
			
			//简单检查
			utils_assert(index < proto_table.size());

			//给出
			return proto_table[index];
		}
		DexString get_dex_string(const std::u8string & text)
		{
			DexString dex_string;

			//todo 看看需不需要把DexString.data也赋上值
			dex_string.text = text;
			return dex_string;
		}
		std::vector<DexString> read_type_list(u4 offset)
		{
			m_input.seekg(offset);
			auto type_id_n = m_input.read<u4>();
			std::vector<u2> type_ids;
			m_input.read(type_ids, type_id_n);
			std::vector<DexString> types;
			for(const auto type_id : type_ids)
			{
				types.push_back(
					get_string_from_type_table(type_id));
			}
			return types;
		}
		raw::ClassData read_raw_class_data(u4 class_data_off)
		{
			raw::ClassData class_data{};
			m_input.seekg(class_data_off);
			class_data.static_field_size = read_uleb128();
			class_data.instance_field_size = read_uleb128();
			class_data.direct_method_size = read_uleb128();
			class_data.virtual_method_size = read_uleb128();

			auto read_fields = [&](std::vector<raw::EncodedField> & vec, u4 count)
			{
				u4 idx{};
				for (u4 i = 0; i < count; i++)
				{
					u4 diff = read_uleb128();
					u4 access = read_uleb128();
					idx += diff;
					vec.push_back({idx, access});
				}
			};
			auto read_methods = [&](std::vector<raw::EncodedMethod> & vec, u4 count)
			{
				u4 idx{};
				for (u4 i = 0; i < count; i++)
				{
					u4 diff = read_uleb128();
					u4 access = read_uleb128();
					u4 code_off = read_uleb128();
					idx += diff;
					vec.push_back({idx, access, code_off});
				}
			};

			read_fields(class_data.static_fields, class_data.static_field_size);
			read_fields(class_data.instance_fields, class_data.instance_field_size);
			read_methods(class_data.direct_methods, class_data.direct_method_size);
			read_methods(class_data.virtual_methods, class_data.virtual_method_size);

			return class_data;
		}
		u4 read_uleb128()
		{
			u8 value{};
			u8 used_size{};
			utils::data_encoding::uleb128::decode(value, used_size, [this]()
			{
				return m_input.read<u1>();
			});
			return utils::basic::store_in<u4>(value);
		}
		int32_t read_sleb128()
		{
			int64_t value{};
			u8 used_size{};
			utils::data_encoding::sleb128::decode(value, used_size, [this]()
			{
				return m_input.read<u1>();
			});
			return utils::basic::store_in<int32_t>(value);
		}
		DexAccessFlags get_access_flags(u4 access_flags, u8 type)
		{
			enum
			{
				ACC_PUBLIC       = 0x0001,
				ACC_PRIVATE      = 0x0002,
				ACC_PROTECTED    = 0x0004,
				ACC_STATIC       = 0x0008,
				ACC_FINAL        = 0x0010,
				ACC_SYNCHRONIZED = 0x0020,
				ACC_VOLATILE     = 0x0040,
				ACC_BRIDGE       = 0x0040,
				ACC_TRANSIENT    = 0x0080,
				ACC_VARARGS      = 0x0080,
				ACC_NATIVE       = 0x0100,
				ACC_INTERFACE    = 0x0200,
				ACC_ABSTRACT     = 0x0400,
				ACC_STRICT       = 0x0800,
				ACC_SYNTHETIC    = 0x1000,
				ACC_ANNOTATION   = 0x2000,
				ACC_ENUM         = 0x4000,
				ACC_CONSTRUCTOR  = 0x00010000,
				ACC_DECLARED_SYNCHRONIZED = 0x00020000,
			};
			DexAccessFlags re{};
			if(access_flags & ACC_PUBLIC	            ) re.is_public                = true;
			if(access_flags & ACC_PRIVATE               ) re.is_private               = true;
			if(access_flags & ACC_PROTECTED             ) re.is_protected             = true;
			if(access_flags & ACC_STATIC                ) re.is_static                = true;
			if(access_flags & ACC_FINAL                 ) re.is_final                 = true;
			if(access_flags & ACC_SYNCHRONIZED          ) re.is_synchronized          = true;
			if(access_flags & ACC_VOLATILE              ) re.is_volatile              = true;
			if(access_flags & ACC_BRIDGE                ) re.is_bridge                = true;
			if(access_flags & ACC_TRANSIENT             ) re.is_transient             = true;
			if(access_flags & ACC_VARARGS               ) re.is_varargs               = true;
			if(access_flags & ACC_NATIVE                ) re.is_native                = true;
			if(access_flags & ACC_INTERFACE             ) re.is_interface             = true;
			if(access_flags & ACC_ABSTRACT              ) re.is_abstract              = true;
			if(access_flags & ACC_STRICT                ) re.is_strict                = true;
			if(access_flags & ACC_SYNTHETIC             ) re.is_synthetic             = true;
			if(access_flags & ACC_ANNOTATION            ) re.is_annotation            = true;
			if(access_flags & ACC_ENUM                  ) re.is_enum                  = true;
			if(access_flags & ACC_CONSTRUCTOR           ) re.is_constructor           = true;
			if(access_flags & ACC_DECLARED_SYNCHRONIZED ) re.is_declared_synchronized = true;

			//处理相同的标志
			if(re.is_volatile)
			{
				if(type == 1) re.is_bridge = false;
				if(type == 2) re.is_volatile = false;
				utils_assert(type == 1 || type == 2);
			}
			if(re.is_transient)
			{
				if(type == 1) re.is_varargs = false;
				if(type == 2) re.is_transient = false;
				utils_assert(type == 1 || type == 2);
			}

			//简单检查
			switch(type)
			{
			case 0: //class
				utils_assert(0 == utils::basic::bits::clear(access_flags,
					ACC_PUBLIC | ACC_FINAL | ACC_INTERFACE | ACC_ABSTRACT |
					ACC_SYNTHETIC | ACC_ANNOTATION | ACC_ENUM));
				break;
			case 1: //field
				utils_assert(0 == utils::basic::bits::clear(access_flags,
					ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL |
					ACC_VOLATILE | ACC_TRANSIENT | ACC_SYNTHETIC | ACC_ENUM));
				break;
			case 2: //method
				utils_assert(0 == utils::basic::bits::clear(access_flags,
					ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL |
					ACC_SYNCHRONIZED | ACC_BRIDGE | ACC_VARARGS | ACC_NATIVE | ACC_ABSTRACT |
					ACC_STRICT | ACC_SYNTHETIC | ACC_CONSTRUCTOR | ACC_DECLARED_SYNCHRONIZED));
				break;
			default:
				utils_throw();
			}

			return re;
		}
		DexAccessFlags get_access_flags_for_class(u4 access_flags)
		{
			//转发
			return get_access_flags(access_flags, 0);
		}
		DexAccessFlags get_access_flags_for_field(u4 access_flags)
		{
			//转发
			return get_access_flags(access_flags, 1);
		}
		DexAccessFlags get_access_flags_for_method(u4 access_flags)
		{
			//转发
			return get_access_flags(access_flags, 2);
		}
		std::u8string dump_access_flags(DexAccessFlags access_flags, bool is_add_bracket)
		{
			std::vector<std::u8string> texts;
			if(access_flags.is_public               )texts.push_back(u8"public");
			if(access_flags.is_private              )texts.push_back(u8"private");
			if(access_flags.is_protected            )texts.push_back(u8"protected");
			if(access_flags.is_static               )texts.push_back(u8"static");
			if(access_flags.is_final                )texts.push_back(u8"final");
			if(access_flags.is_synchronized         )texts.push_back(u8"synchronized");
			if(access_flags.is_volatile             )texts.push_back(u8"volatile");
			if(access_flags.is_bridge               )texts.push_back(u8"bridge");
			if(access_flags.is_transient            )texts.push_back(u8"transient");
			if(access_flags.is_varargs              )texts.push_back(u8"varargs");
			if(access_flags.is_native               )texts.push_back(u8"native");
			if(access_flags.is_interface            )texts.push_back(u8"interface");
			if(access_flags.is_abstract             )texts.push_back(u8"abstract");
			if(access_flags.is_strict               )texts.push_back(u8"strict");
			if(access_flags.is_synthetic            )texts.push_back(u8"synthetic");
			if(access_flags.is_annotation           )texts.push_back(u8"annotation");
			if(access_flags.is_enum                 )texts.push_back(u8"enum");
			if(access_flags.is_constructor          )texts.push_back(u8"constructor");
			if(access_flags.is_declared_synchronized)texts.push_back(u8"declared_synchronized");

			const auto text = utils::text::combine::combine(texts, u8" ");
			if(!is_add_bracket) return text;
			if(text.empty()) return {};
			return u8"[" + text + u8"]";
		}
		std::optional<DexCode> read_dex_code(u4 code_off)
		{
			if(0 == code_off) return {};

			DexCode dex_code{};
			m_input.seekg(code_off);
			dex_code.registers_size = m_input.read<u2>();
			dex_code.ins_size = m_input.read<u2>();
			dex_code.outs_size = m_input.read<u2>();
			const auto tries_size = m_input.read<u2>();
			dex_code.debug_info_off = m_input.read<u4>();
			const auto insns_size = m_input.read<u4>();
			m_input.read(dex_code.insns, insns_size);

			if(tries_size > 0)
			{
				m_input.seekg_to_align(4);
				m_input.read(dex_code.tries, tries_size);

				uint64_t handlers_start_addr{};
				m_input.tellg(handlers_start_addr);
				const auto handlers_size = read_uleb128();
				for(u8 i = 0; i < handlers_size; i++)
				{
					uint64_t handler_start_addr{};
					m_input.tellg(handler_start_addr);

					raw::CatchHandlerGroup handler{};
					{
						handler.size = read_sleb128();
						const u8 abs_size = abs(handler.size);
						handler.type_idxs.resize(abs_size);
						handler.addr_idxs.resize(abs_size);
						for (u8 i = 0; i < abs_size; i++)
						{
							handler.type_idxs[i] = read_uleb128();
							handler.addr_idxs[i] = read_uleb128();
						}
						if (handler.size <= 0)
							handler.catch_all_addr = read_uleb128();
					}

					DexHandlerGroup handler_2{};
					{
						for(const auto & one : handler.type_idxs)
							handler_2.types.push_back(get_string_from_type_table(one));
						handler_2.addr_idxs = handler.addr_idxs;
						if (handler.size <= 0)
							handler_2.catch_all_addr = handler.catch_all_addr;
						handler_2.start_offset = utils::basic::store_in<u4>(handler_start_addr-handlers_start_addr);
					}
					dex_code.catch_handlers.push_back(std::move(handler_2));
				}
			}

			dex_code.decoded_code = utils::opcode::dalvik::decode::decode_function_code(
				dex_code.insns,
				[&dex_code]()
				{
					std::vector<u8> targets;
					for(const auto & one : dex_code.catch_handlers)
					{
						utils::vector(targets).push_back_many(one.addr_idxs);
						if(one.catch_all_addr.has_value())
							utils::vector(targets).push_back(one.catch_all_addr.value());
					}
					return targets;
				}
			);
			return dex_code;
		}
		std::u8string dump_dex_code(const std::optional<DexCode> & dex_code)
		{
			if(!dex_code) return {};
			
			std::u8string text;
			text += u8"\t\t\t\t";
			text += utils::text::format::format(u8"reg:%d in:%d out:%d 0x%x 0x%x\n",
				dex_code->registers_size,
				dex_code->ins_size,
				dex_code->outs_size,
				dex_code->debug_info_off,
				dex_code->insns.size());

			if (!dex_code->tries.empty())
			{
				text += u8"\t\t\t\t";
				text += u8"tries:\n";
				for(const auto & one : dex_code->tries)
				{
					text += u8"\t\t\t\t\t";
					text += utils::text::format::format(u8"start:0x%x size:0x%x catch:0x%x\n",
						one.start_addr,
						(u4)one.insn_count,
						(u4)one.handler_off);
				}
				
				//简单检查: 猜测有try就必有catch块
				utils_assert(!dex_code->catch_handlers.empty());
			}
			if (!dex_code->catch_handlers.empty())
			{
				text += u8"\t\t\t\t";
				text += u8"catch_handlers:\n";
				for(const auto & catch_handler : dex_code->catch_handlers)
				{
					text += u8"\t\t\t\t\t";
					text += utils::text::format::format(u8"group 0x%x\n",
						catch_handler.start_offset);

					//简单检查
					utils_assert(catch_handler.addr_idxs.size() == catch_handler.types.size());
					
					for(uint64_t i = 0; i < catch_handler.addr_idxs.size(); i++)
					{
						text += u8"\t\t\t\t\t\t";
						text += dump_string(catch_handler.types[i]);
						text += utils::text::format::format(u8" code:0x%x\n",
							catch_handler.addr_idxs[i]);
					}
					if(catch_handler.catch_all_addr)
					{
						text += u8"\t\t\t\t\t\t";
						text += utils::text::format::format(u8"<catch_all> code:0x%x\n",
							catch_handler.catch_all_addr.value());
					}
				}
			}
			text += u8"\t\t\t\t";
			text += u8"code:\n";

			//准备数据
			using PoolType = utils::opcode::dalvik::common::instruction::PoolType;
			auto get_text_for_const_pool = [this](PoolType pool_type, u8 index) -> std::u8string
			{
				switch(pool_type)
				{
				case PoolType::String:
					return u8"\"" + m_string_table.value().at(index).text + u8"\"";
				case PoolType::Type:
					return m_type_table.value().at(index).text;
				case PoolType::Field:
					return 
						u8"{" +
						m_field_table.value().at(index).name.text +
						u8" type:" +
						m_field_table.value().at(index).type.text +
						u8" of_class:" +
						m_field_table.value().at(index).of_class.text +
						u8"}";
				case PoolType::Method:
					return m_method_table.value().at(index).name.text;
				default:
					utils_throw();
				}
			};

			text += utils::opcode::dalvik::decode::dump_function_code(dex_code->decoded_code.value(),
				5, dex_code->insns, get_text_for_const_pool);
			return text;
		}
	};
}
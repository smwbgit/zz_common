#pragma once

#include <utils/binary/code/basic_block.hpp>
#include <utils/IdGiver.hpp>

namespace utils::binary::code::basic_block
{
	//给出基本块流程图中，所有变量的作用范围
	class VariableManager
	{
	public:
		struct s_range_in_block
		{
			uint64_t block_addr{};
			uint64_t begin{};
			uint64_t end{};
		};
		struct s_used_position
		{
			uint64_t addr{};	//block_addr+pos
			uint64_t block_addr{};
			uint64_t pos{};
			bool is_write{};
			bool is_read{};
		};
		struct s_variable
		{
			uint64_t id{};	//变量id
			uint64_t register_id{};
			std::vector<s_range_in_block> scope;	//作用域
			std::vector<s_used_position> used_positions;	//被使用的位置
		};
	private:
		//一个块的上半部分或下半部分
		struct s_half
		{
			uint64_t m_addr{};
			bool m_is_come_from_upstream{};/*用来计算作用域是在节点的上边还是下边
				（因为作用域是到节点中的赋值语句的，所以分上边和下边）*/

			bool operator< (const s_half & other) const
			{
				if (m_addr != other.m_addr) return m_addr < other.m_addr;
				if (m_is_come_from_upstream != other.m_is_come_from_upstream)
					return (int)m_is_come_from_upstream < (int)other.m_is_come_from_upstream;
				return false;
			}
		};
	private:
		const basic_block::Graph & m_graph;
		std::map<uint64_t, s_variable> m_variables;
		utils::IdGiver<uint64_t> m_id_giver;
	public:
		VariableManager(const basic_block::Graph & graph)
			: m_graph{graph}
		{
			make();
		}
	private:
		//识别出所有变量
		void make(uint64_t register_id)
		{
			/*
				核心思想：
					利用赋值行为来分离出各个变量的作用域

				具体算法：
					当出现赋值行为时，当前块的向上或向下的链视为断开状态
					尽可能沿着上下游的链，多遍历节点
					最后，所有遍历到的节点，就是新变量的作用域
					再依次按上面的办法，把被隔离的节点，也走一遍，直到所有节点都被遍历

				补充细节：
					对于上面的算法得出的作用域，还要做一次细分：
					当一个作用域中节点，沿着自然路径，不能再碰到读或写这个变量的语句时，这个节点被视为
					无关节点，对任何方向，都不再有传导性（也就是说，它上下游连接的节点，都视为其他变量的作用域）
					这样，就可以把一个变量的作用域，分成几个部分：
					1.若干个正常的作用域
					2.若干个无关节点
			*/
			
			//逐渐识别出每一个作用域
			{
				std::queue<s_half> wait; wait.push({m_graph.get_entry(), true});
				std::set<s_half> visited;
				while(!wait.empty())
				{
					//取出一个节点描述
					const auto start = utils::queue(wait).pop();

					//跳过访问过的
					if(utils::set(visited).is_exist(start)) continue;
				
					//计算与它相关的 若干个正常作用域 和 若干个无关节点，以及 若干个相邻作用域的节点
					std::vector<std::set<s_half>> scopes;
					std::set<s_half> unrelated_nodes;
					std::set<s_half> other_scope_nodes;
					get_scope(register_id, start, scopes, unrelated_nodes, other_scope_nodes);

					//保存正常作用域
					for(const auto & scope : scopes)
					{
						//简单检查: 此作用域 未和 已访问区域 重叠
						utils_assert(utils::set(visited).get_intersection(scope).empty());
				
						//保存
						add_variable(register_id, scope);

						//此作用域 视为已访问过
						utils::set(visited).insert_many_not_exist(scope);
					}

					//保存无关节点
					{
						//简单检查: 这些节点 未和 已访问区域 重叠
						utils_assert(utils::set(visited).get_intersection(unrelated_nodes).empty());
				
						//这些节点 视为已访问过
						utils::set(visited).insert_many_not_exist(unrelated_nodes);
					}

					//比邻的其他作用域节点 加入到等待访问中
					utils::queue(wait).push_many(other_scope_nodes);
				}

#if 0	//for debug
				if (visited.size() != m_blocks.size()*2)
				{
					std::set<s_block> all;
					for(const auto & block : m_blocks)
					{
						utils::set(all).insert_not_exist({block.first, true});
						utils::set(all).insert_not_exist({block.first, false});
					}
					std::set<s_block> not_visit = all;
					utils::set(not_visit).remove_intersection(visited);
					while(0);
				}
#endif

				//简单检查: 所有块均已遍历
				utils_assert(visited.size() == m_graph.get_basic_blocks().size() * 2);
			}

			//补充上在基本块的夹层中出现的变量
			for (const auto & block : utils::map(m_graph.get_basic_blocks()).iterator_seconds())
			{
				//查找节点
				const auto & reg = utils::map(block->m_register_use_positions).get_existed_value(register_id);

				//没有夹层
				if(reg.m_assign_positions.size() <= 1) continue;

				//保存
				for(uint64_t i = 1; i < reg.m_assign_positions.size(); i++)
					add_interlayer_variable(register_id, block.m_addr, i);
			}

			//已识别出全部的寄存器变量，识别每个变量的使用位置，包括读取和写入
			for(auto & variable : utils::map(m_variables).iterator_seconds())
			{
				//只处理当前寄存器的变量
				if(variable.register_id != register_id) continue;

				//对于作用域中的每个节点，查找使用位置
				for(const auto & scope_block : variable.scope)
				{
					//查找节点
					const auto & block = m_graph.look_up_basic_block(scope_block.block_addr);
					const auto & reg = utils::map(block->m_register_use_positions).get_existed_value(register_id);
					
					//对于节点中的每个汇编语句，检查是否使用了此变量
					for(const auto & instruction : block.m_instructions)
					{
						//没在作用域内
						const auto pos = instruction.first;
						if (!(pos >= scope_block.begin && pos < scope_block.end)) continue;

						bool is_read = instruction.second.is_read_64bit_general_purpose_register(register_id);
						bool is_write = instruction.second.is_write_64bit_general_purpose_register(register_id);

						//没有被使用
						if(!is_read && !is_write) continue;

						//保存
						s_used_position used_position;
						used_position.block_addr = block.m_addr;
						used_position.pos = instruction.first;
						used_position.addr = used_position.block_addr + used_position.pos;
						used_position.is_read = is_read;
						used_position.is_write = is_write;
						variable.used_positions.push_back(used_position);
					}
				}
			}
		}
		//识别出所有变量
		void make()
		{
			//每个寄存器的算法是一样的
			for (const auto register_id : utils::binary::code::capstone::register_id::
				get_all_64bit_general_purpose_register())
			{
				//转发
				make(register_id);
			}

#if 0	//debug
			std::u8string text_debug;
			for(const auto register_id : utils::binary::code::capstone::register_id::get_all_64bit_general_purpose_register())
			{
				for (const auto & variable : m_variables)
				{
					if(variable.second.register_id != register_id) continue;

					text_debug += utils::text::format::format(u8"%llu ", variable.second.id);
					text_debug += utils::text::format::format(u8"%s:\n",
						utils::binary::code::capstone::get_register_name(variable.second.register_id).c_str());
					for(const auto & one : variable.second.scope)
					{
						text_debug += utils::text::format::format(u8"%llx ", one.block_addr);
						//text_debug += utils::text::format::format(u8"%llx ", one.start);
						//text_debug += utils::text::format::format(u8"%llx\n", one.end);
					}
					text_debug += u8"\n";
					for(const auto & one : variable.second.used_positions)
					{
						//if (!one.is_write) continue;

						//查找节点
						const auto & block = m_graph.look_up_basic_block(one.block_addr);
						const auto & instruction = utils::map(block.m_instructions).get_existed_value(one.pos);
					
						text_debug += utils::text::format::format(u8"%llx ", one.addr);
						text_debug += utils::text::format::format(u8"%s %s\n",
							instruction.m_insn.mnemonic,
							instruction.m_insn.op_str);
					}
					text_debug += u8"\n";
				}
			}

			{	//delete me
				const auto & block = m_graph.look_up_basic_block(0x180030eb0);
				const auto & reg = utils::map(block->m_register_use_positions).get_existed_value(X86_REG_R15);

				while(0);
			}
			while(0);
#endif
		}
		void get_scope(
			const uint64_t register_id,
			const s_half & start,
			std::set<s_half> & scope,
			std::set<s_half> & unrelated_nodes,
			std::set<s_half> & other_scope_nodes)
		{
			//沿着上下游的链，尽可能多遍历节点, 来形成作用域
			get_scope_1(register_id, start, {}, scope, other_scope_nodes);

			//找出所有无关节点
			unrelated_nodes = get_unrelated_nodes(register_id, start, scope);

			//再扫描一遍
			get_scope_1(register_id, start, unrelated_nodes, scope, other_scope_nodes);
		}
		void get_scope_1(
			const uint64_t register_id,
			const s_half & start,
			const std::set<s_half> & unrelated_nodes,
			std::set<s_half> & scope,
			std::set<s_half> & other_scope_nodes)
		{
			utils::algorithm::graph::BreadthFirstSearch<s_half> search;
			search.m_entry = {start};
			search.m_meet_one_node = [this, register_id, &scope_group, &other_scope_nodes](const s_half current)->std::set<s_half>
			{
				//查找节点
				const auto & block = m_graph.look_up_basic_block(current.m_addr);
				const auto & reg = utils::map(block->m_register_use_positions).get_existed_value(register_id);

				//收集此作用域
				utils::set(scope_group).insert_not_exist(current);

				//按节点是否被赋值语句截断，分两种情况，得出节点
				std::set<s_half> next;
				if (reg.m_assign_positions.empty())	//没被截断，此节点的两个方向，都要遍历
				{
					for (const auto block_addr : block->m_immediate_successor)
						next.insert({block_addr, true});
					for (const auto block_addr : block->m_immediate_predecessor)
						next.insert({block_addr, false});
						
					//补充上当前节点的前半和后半部分
					next.insert({current.m_addr, true});
					next.insert({current.m_addr, false});
				}
				else	//被截断时, 只向一个方向遍历，另一个方向视为其他作用域
				{
					if(current.m_is_come_from_upstream)
					{
						for (const auto block_addr : block->m_immediate_predecessor)
							next.insert({block_addr, false});
						next.insert({current.m_addr, true});
					}
					else
					{
						for (const auto block_addr : block->m_immediate_successor)
							next.insert({block_addr, true});
						next.insert({current.m_addr, false});
					}

					//非此作用域的节点
					other_scope_nodes.insert({current.m_addr, !current.m_is_come_from_upstream});
				}

				//给出
				return next;
			};
			search.start();

			//对other_scope_nodes来说，排除掉此作用域中的。（因为有时一个节点虽然被隔开了，但有可能从另一个路径能到达这个节点）
			utils::set(other_scope_nodes).remove_intersection(scope_group);
		}
		//找出所有无关节点
		std::set<s_half> get_unrelated_nodes(
			const uint64_t register_id,
			const s_half & start,
			const std::set<s_half> & scope_group)
		{
			std::set<s_half> unrelated_nodes;
			for (const auto& node : scope_group)
			{
				if (is_unrelated_node(register_id, node, scope_group))
					utils::set(unrelated_nodes).insert_not_exist(node);
			}
			return unrelated_nodes;
		}
		//是否为无关节点
		bool is_unrelated_node(const uint64_t register_id, const s_half & node,
			const std::set<s_half> & scope_group)
		{
			//沿着自然路径，检查是否能碰到读或写这个变量的语句
			bool is_unrelated_node = true;
			utils::algorithm::graph::BreadthFirstSearch<s_half> search;
			search.m_entry = {node};
			search.m_meet_one_node = [this, register_id, &is_unrelated_node, &search, &scope_group]
				(const s_half current) -> std::set<s_half>
			{
				//查找节点
				const auto & block = m_graph.look_up_basic_block(current.m_addr);
				const auto & reg = utils::map(block->m_register_use_positions).get_existed_value(register_id);
				
				//if (!reg.m_assign_positions.empty() || !reg.m_read_positions.empty())
				//{
				//	is_unrelated_node = false;
				//	search.stop();
				//}

				//沿着自然路径前进
				std::set<s_half> next;
				if(current.m_is_come_from_upstream)
				{
					next.insert({current.m_addr, false});
				}
				else
				{
					for (const auto & block_addr : block->m_immediate_successor)
						next.insert({block_addr, true});
				}
				//限制在当前的作用域组中
				next = utils::set(next).get_intersection(scope_group);

				return next;
			};
			search.start();
			return is_unrelated_node;
		}
		//找出所有正常作用域
		std::vector<std::set<s_half>> get_scopes(
			const uint64_t register_id,
			const std::set<s_half> & scope_group,
			const std::set<s_half> & unrelated_nodes)
		{
			//使用无关节点，再重新扫描一遍
			utils::algorithm::graph::BreadthFirstSearch<s_half> search;
			search.m_entry = {start};
			search.m_meet_one_node = [this, register_id, &scope_group, &other_scope_nodes](const s_half current)->std::set<s_half>
			{
				//查找节点
				const auto & block = m_graph.look_up_basic_block(current.m_addr);
				const auto & reg = utils::map(block->m_register_use_positions).get_existed_value(register_id);

				//收集此作用域
				utils::set(scope_group).insert_not_exist(current);

				//按节点是否被赋值语句截断，分两种情况，得出节点
				std::set<s_half> next;
				if (reg.m_assign_positions.empty())	//没被截断，此节点的两个方向，都要遍历
				{
					for (const auto block_addr : block->m_immediate_successor)
						next.insert({block_addr, true});
					for (const auto block_addr : block->m_immediate_predecessor)
						next.insert({block_addr, false});
						
					//补充上当前节点的前半和后半部分
					next.insert({current.m_addr, true});
					next.insert({current.m_addr, false});
				}
				else	//被截断时, 只向一个方向遍历，另一个方向视为其他作用域
				{
					if(current.m_is_come_from_upstream)
					{
						for (const auto block_addr : block->m_immediate_predecessor)
							next.insert({block_addr, false});
						next.insert({current.m_addr, true});
					}
					else
					{
						for (const auto block_addr : block->m_immediate_successor)
							next.insert({block_addr, true});
						next.insert({current.m_addr, false});
					}

					//非此作用域的节点
					other_scope_nodes.insert({current.m_addr, !current.m_is_come_from_upstream});
				}

				//给出
				return next;
			};
			search.start();

			//对other_scope_nodes来说，排除掉此作用域中的。（因为有时一个节点虽然被隔开了，但有可能从另一个路径能到达这个节点）
			utils::set(other_scope_nodes).remove_intersection(scope_group);

		}
#if 0
			get_scope_3(register_id, node, scope, unrelated_nodes);
			{
				std::set<s_half> scope;
				std::set<s_half> other_scope_nodes;
				scopes.push_back(scope);
			}
			const auto 

			//对于每个节点，沿着自然路径，检查是否能碰到读或写这个变量的语句
			for(const auto & node : scope_1)
			{
				bool is_unrelated_node = is_unrelated_node();
				utils::algorithm::graph::BreadthFirstSearch<s_half> search;
				search.m_entry = {node};
				search.m_meet_one_node = [this, register_id, &unrelated_nodes](const s_half current)->std::set<s_half>
				{
					//查找节点
					const auto& block = m_graph.look_up_basic_block(current.m_addr);
					const auto& reg = utils::map(block->m_register_use_positions).get_existed_value(register_id);
					//对于节点中的每个汇编语句，检查是否使用了此变量
					for (const auto& instruction : block.m_instructions)
					{
						//没在作用域内
						const auto pos = instruction.first;
						if (!(pos >= 0 && pos < block.m_size)) continue;
						bool is_read = instruction.second.is_read_64bit_general_purpose_register(register_id);
						bool is_write = instruction.second.is_write_64bit_general_purpose_register(register_id);
						//没有被使用
						if (!is_read && !is_write) continue;
						//保存
						s_used_position used_position;
						used_position.block_addr = block.m_addr;
						used_position.pos = instruction.first;
						used_position.addr = used_position.block_addr + used_position.pos;
						used_position.is_read = is_read;
						used_position.is_write = is_write;
						utils::set(unrelated_nodes).insert_not_exist(current);
					}
					//给出
					return {};
				};
			}
#endif
		}
#if 0
				//查找节点
				const auto & block = m_graph.look_up_basic_block(current.m_addr);
				const auto & reg = utils::map(block->m_register_use_positions).get_existed_value(register_id);

				//空作用域: 如果此节点是从上游过来的，并且，第一个语句是赋值语句，则此节点为空作用域
				const bool is_empty_scope = current.is_come_from_upstream && !reg.m_assign_positions.empty() && reg.m_assign_positions[0] == 0;

				//如果不为空作用域， 则收集此作用域
				if (!is_empty_scope)
					utils::set(scope).insert_not_exist(current);

				//按节点是否被赋值语句截断，分两种情况，得出节点
				std::set<s_block> next;
				if (reg.m_assign_positions.empty())	//没被截断，此节点的两个方向，都要遍历
				{
					//简单检查
					utils_assert(!is_empty_scope);

					for (const auto block_addr : block->m_immediate_successor)
						next.insert({block_addr, true});
					for (const auto block_addr : block->m_immediate_predecessor)
						next.insert({block_addr, false});
						
					//补充上当前节点的前半和后半部分
					next.insert({current.addr, true});
					next.insert({current.addr, false});
				}
				else	//被截断时
				{
					if(is_empty_scope)   //不向任何方向遍历. 把相连的节点，当做另一个作用域里的节点
					{
						//简单检查
						utils_assert(current.is_come_from_upstream);

						for (const auto block_addr : block->m_immediate_predecessor)
							other_scopes.insert({block_addr, false});
						other_scopes.insert({current.addr, false});
					}
					else   //只向一个方向遍历，另一个方向视为其他作用域
					{
						if(current.is_come_from_upstream)
						{
							for (const auto block_addr : block->m_immediate_predecessor)
								next.insert({block_addr, false});
							next.insert({current.addr, true});
						}
						else
						{
							for (const auto block_addr : block->m_immediate_successor)
								next.insert({block_addr, true});
							next.insert({current.addr, false});
						}

						//非此作用域的节点
						other_scopes.insert({current.addr, !current.is_come_from_upstream});
					}
				}

				//给出
				return next;
			};
#endif
		void add_variable(const uint64_t register_id, const std::set<s_half> & scope)
		{
			//获得id
			s_variable variable;
			variable.id = m_id_giver.get_one_id();

			//填写信息
			variable.register_id = register_id;

			//简单检查
			utils_assert(!scope.empty());

			//把基本块分成两种：全部覆盖和只覆盖上边或下边
			std::set<s_half> half_blocks;
			std::set<uint64_t> whole_blocks;
			{
				//统计每个块的个数
				std::map<uint64_t, uint64_t> counts;
				for(const auto & block : scope)
					counts[block.m_addr] ++;
					
				//按个数来分类
				std::map<uint64_t, uint64_t> insert_counts;
				for(const auto & block : scope)
				{
					if(counts[block.m_addr] == 1)
						utils::set(half_blocks).insert_not_exist(block);
					else if(counts[block.m_addr] == 2)
					{
						if (insert_counts[block.m_addr] == 0)
							utils::set(whole_blocks).insert_not_exist(block.m_addr);
						else
							utils_assert(insert_counts[block.m_addr] == 1);
					}
					else
						utils_throw();
						
					insert_counts[block.m_addr] ++;
				}
			}

			//全部覆盖的基本块
			for(const auto block_addr : whole_blocks)
			{
				//查找节点
				const auto & block = m_graph.look_up_basic_block(block_addr);

				//添加
				variable.scope.push_back({block_addr, 0, block.m_size});
			}

			//只覆盖上边或下边的块
			for(const auto & half_block : half_blocks)
			{
				//查找节点
				const auto & block = m_graph.look_up_basic_block(half_block.m_addr);
				const auto & reg = utils::map(block->m_register_use_positions).get_existed_value(register_id);

				//简单检查
				utils_assert(!reg.m_assign_positions.empty());

				//添加
				if (half_block.m_is_come_from_upstream)
					variable.scope.push_back({block.m_addr, 0, reg.m_assign_positions[0]});
				else
					variable.scope.push_back({block.m_addr, utils::vector(reg.m_assign_positions).last_one(), block.m_size});
			}

			//保存
			utils::map(m_variables).insert_not_exist(variable.id, std::move(variable));
		}
		void add_interlayer_variable(const uint64_t register_id, const uint64_t block_addr, uint64_t i)
		{
			//获得id
			s_variable variable;
			variable.id = m_id_giver.get_one_id();

			//填写信息
			variable.register_id = register_id;
				
			//查找节点
			const auto & block = m_graph.look_up_basic_block(block_addr);
			const auto & reg = utils::map(block->m_register_use_positions).get_existed_value(register_id);
				
			//简单检查
			utils_assert(reg.m_assign_positions.size() >= 2);
			utils_assert(i >= 1);
			utils_assert(i < reg.m_assign_positions.size());
				
			//填写信息
			variable.scope.push_back({block.m_addr, reg.m_assign_positions[i-1], reg.m_assign_positions[i]});

			//保存
			utils::map(m_variables).insert_not_exist(variable.id, std::move(variable));
		}
	};
}
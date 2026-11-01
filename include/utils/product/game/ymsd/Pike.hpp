#pragma once

#include <stdint.h>
#include <utils/data.hpp>

namespace utils::product::game::ymsd
{
    class Pike
    {
    private:
        struct s_pike_key
        {
            uint64_t carry;
	        uint32_t buffer[64];
            uint64_t index;
	        uint64_t dis1;
	        uint64_t dis2;
	        uint32_t sd;
        };
        struct s_pike
        {
            uint64_t index;
	        uint8_t buffer[4096];
            s_pike_key keys[3];
	        uint32_t sd;
        };
    private:
        s_pike m_pike;
    public:
        Pike(uint32_t seed)
        {
            init_pike(seed, m_pike);
        }
	    void encode_or_decode(utils::data & data)
	    {
		    use_pike(data, m_pike);
	    }
    private:
        static void init_pike(const uint32_t seed, s_pike & pike)
        {
            auto new_bit = [](uint32_t & v)
            {
                uint64_t _v = 
                    ((uint64_t)(v ^ (v >> 6) ^ (v >> 31) ^ (v >> 4) ^ (v >> 2) ^ (v >> 1)) << 32)
                    + v;
                v = _v >> 1;
            };

	        const uint32_t GENIUS_NUMBER = 0x5027919;
            pike.sd = GENIUS_NUMBER^seed;
            pike.keys[0].sd = pike.sd;
            new_bit(pike.keys[0].sd);
            pike.keys[0].dis1=55;
            pike.keys[0].dis2=24;
            pike.keys[1].sd = (pike.sd >> 1) & 0x55555555 | 2 * pike.sd & 0xAAAAAAAA;
            new_bit(pike.keys[1].sd);
            pike.keys[1].dis1 = 57;
            pike.keys[1].dis2 = 7;
            pike.keys[2].sd = ~((pike.sd >> 4) & 0xF0F0F0F | 16 * pike.sd & 0xF0F0F0F0);
            new_bit(pike.keys[2].sd);
            pike.keys[2].dis1=58;
            pike.keys[2].dis2=19;

            for(uint64_t key_index = 0; key_index < 3; key_index ++)
            {
                uint32_t _sd = pike.keys[key_index].sd;
                for(uint64_t buffer_index = 0; buffer_index < 64; buffer_index ++)
                {
                    for(uint64_t j = 0; j < 32; j++)
                        new_bit(_sd);

                    pike.keys[key_index].buffer[buffer_index]=_sd;
                }

                pike.keys[key_index].carry = 0;
                pike.keys[key_index].index=63;
            }
            pike.index = 4096;
        }
        static void next_key(s_pike_key & key)
        {
	        key.index++;
	        key.index &= 0x3Fu;
	        uint64_t v6 = (key.index - key.dis1) & 0x3F;
	        uint64_t v7 = (key.index - key.dis2) & 0x3F;
	        key.buffer[key.index] = key.buffer[v6] + key.buffer[v7];
	        key.carry = key.buffer[key.index] < key.buffer[v6] || key.buffer[key.index] < key.buffer[v7];
        }
        static void make_pike(s_pike & pike)
        {
            for (uint64_t i = 0; i < 1024; i++)
            {
                int v9 = pike.keys[0].carry + pike.keys[1].carry + pike.keys[2].carry;
                if(v9 == 1 || v9 == 2)
                {
                    uint64_t v12 = v9 == 1 ? 0 : 1;
            
                    for (uint64_t key_index = 0; key_index < 3; key_index++)
                    {
                        auto & key = pike.keys[key_index];
                        if (key.carry == v12)
                            next_key(key);
                    }
                }
                else if(v9 == 0 || v9 == 3)
                {
                    next_key(pike.keys[0]);
                    next_key(pike.keys[1]);
                    next_key(pike.keys[2]);
                }
                else
                {
                    utils_throw();
                }
        
		        *(uint32_t*)&pike.buffer[4 * i] = 
                    pike.keys[0].buffer[pike.keys[0].index] ^
                    pike.keys[1].buffer[pike.keys[1].index] ^
                    pike.keys[2].buffer[pike.keys[2].index];
            }
            pike.index = 0;
        }
        static void use_pike(utils::data & data, s_pike & pike)
        {
	        uint64_t data_offset = 0;
	        uint64_t rest_size = data.size();

	        while (1)
	        {
		        if (rest_size <= 0)
			        return;

		        if (pike.index >= 4096)
		        {
			        make_pike(pike);
			        continue;
		        }

		        uint64_t pike_size = 4096 - pike.index;
		        if (pike_size > rest_size) pike_size = rest_size;
			
		        for(uint64_t i = 0; i < pike_size; i++)
			        data[data_offset+i] ^= pike.buffer[pike.index+i];

		        data_offset += pike_size;
		        rest_size -= pike_size;
		        pike.index += pike_size;
	        }
        }
    };
}

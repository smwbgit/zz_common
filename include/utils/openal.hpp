#pragma once

#include <utils/libs/openal/openal-1.1/all.hpp>
#include <utils/math/geo.hpp>
#include <thread>

namespace utils::openal
{
	class Context
	{
	private:
		ALCdevice* m_device{};
		ALCcontext* m_context{};
		ALuint m_source{};
	public:
		Context()
		{
			m_device = alcOpenDevice(0);
			utils_assert(m_device);
			m_context = alcCreateContext(m_device,0);
			utils_assert(m_context);
			utils_assert(ALC_TRUE == alcMakeContextCurrent(m_context));

			alGenSources(1,&m_source);
			alSourcef(m_source,AL_GAIN, 1);
			alSource3f(m_source,AL_POSITION,0,0,0);
		}
		~Context()
		{
			alDeleteSources(1, &m_source);

			alcDestroyContext(m_context);
			utils_assert_abort(ALC_TRUE == alcCloseDevice(m_device));
		}
		//播放一个余弦波。播放结束后返回
		//音量: [0,1]
		//example: play_cos(1.3, {{470, 0.2}});
		void play_cos(double 秒数, const std::vector<std::pair<double /*频率*/, double /*音量*/>> & rates)
		{
			auto load_sound = [](ALuint buffer, double 秒数, const std::vector<std::pair<double /*频率*/, double /*音量*/>> & rates)
			{
				std::vector<double> data{};
				data.resize(1400*10*秒数*2);
				double _max = 0;
				for(const auto & rate : rates)
				{
					_max += rate.second  /*音量*/;
					double rad=0;
					for(auto & one : data)
					{
						one += cos(rad) * rate.second /*音量*/;
						rad+=utils::math::geo::pi*2 / 10  * (rate.first /*频率*/ / 1400);
					}
				}
	
				std::vector<short> data_2{};
				data_2.reserve(data.size());
				for(const auto & one : data)
				{
					data_2.push_back( (std::numeric_limits<short>::max)() * one / _max);
				}
				alBufferData(buffer, AL_FORMAT_MONO16, data_2.data(), (ALsizei)data_2.size(), 1400*10);
			};

			ALuint buffer{};
			alGenBuffers(1, &buffer);
			load_sound(buffer, 秒数, rates);

			alSourcei(m_source, AL_BUFFER, buffer);
			alSourcePlay(m_source);
	
			while(1)
			{
				ALint iState{};
				alGetSourcei(m_source, AL_SOURCE_STATE, &iState);
				if(iState != AL_PLAYING) break;
				std::this_thread::yield();
			}

			alSourceStop(m_source);
			alSourcei(m_source, AL_BUFFER, 0);
			
			alDeleteBuffers(1, &buffer);
		}
	};
}

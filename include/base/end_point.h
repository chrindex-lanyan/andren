#pragma once



#include <stdint.h>
#include <string>

struct sockaddr_in;
struct sockaddr;

namespace chrindex::andren::base
{
	class EndPointIPV4
	{
	public:
		EndPointIPV4();
		EndPointIPV4(const std::string& ip, int32_t port);
		EndPointIPV4(const EndPointIPV4& ep) noexcept;
		EndPointIPV4(EndPointIPV4&& ep) noexcept;
		EndPointIPV4(::sockaddr_in const * another) noexcept;
		void operator=(const EndPointIPV4& ep);
		void operator=(EndPointIPV4&& ep) noexcept;
		~EndPointIPV4();

		/// 设置ip和端口
		bool setSockAddrIn(const std::string& ip, int32_t port);

		/// 设置ip和端口
		bool getSockAddrIn(std::string& ip, int32_t& port) const;

		uint16_t port();

		std::string ip();

		::sockaddr_in* raw() const;

		::sockaddr * toAddr();

		static size_t addrSize() ;

	private:
		::sockaddr_in* handle;
	};

	using end_point_v4 = EndPointIPV4;

}
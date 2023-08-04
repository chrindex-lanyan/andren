
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <assert.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "EndPoint.h"

namespace chrindex::andren::base{

EndPointIPV4::EndPointIPV4(const std::string& ip, int32_t port)
{
	handle = new sockaddr_in;
	memset(handle, 0, sizeof(sockaddr_in));
	setSockAddrIn(ip, port);
}
EndPointIPV4::EndPointIPV4()
{
	handle = new sockaddr_in;
	memset(handle, 0, sizeof(sockaddr_in));
}

EndPointIPV4::EndPointIPV4(sockaddr_in const * another)
{
    handle = new sockaddr_in;
	memcpy(handle, another, sizeof(sockaddr_in));
}

EndPointIPV4::~EndPointIPV4()
{
	delete handle;
}

sockaddr_in* EndPointIPV4::raw() const
{
	return handle;
}

bool EndPointIPV4::setSockAddrIn(const std::string& ip, int32_t port)
{
	if (ip == "any" || ip == "")
	{
		handle->sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		int ret = ::inet_pton(AF_INET, ip.c_str(), &handle->sin_addr.s_addr);
		assert(ret);
	}
	handle->sin_family = AF_INET;
	handle->sin_port = htons((uint16_t)port);
	return true;
}

bool EndPointIPV4::getSockAddrIn(std::string& ip, int32_t& port) const
{
	if (handle->sin_addr.s_addr == INADDR_ANY)
	{
		ip = "any";
	}
	else
	{
		ip.resize(64);
		char _ip[64] = { 0 };
		::inet_ntop(AF_INET, &handle->sin_addr, &_ip[0], sizeof(_ip));
		ip = _ip;
	}
	port = ntohs(handle->sin_port);
	return true;
}

EndPointIPV4::EndPointIPV4(const EndPointIPV4& ep) noexcept
{
	handle = new sockaddr_in;
	memcpy(handle, ep.handle, sizeof(sockaddr_in));
}

EndPointIPV4::EndPointIPV4(EndPointIPV4&& ep) noexcept
{
	EndPointIPV4 _ep;
	std::swap(_ep.handle, ep.handle);
	std::swap(_ep.handle, handle);
	_ep.handle = 0;
}

void EndPointIPV4::operator=(const EndPointIPV4& ep)
{
	memcpy(handle, ep.handle, sizeof(sockaddr_in));
}

void EndPointIPV4::operator=(EndPointIPV4&& ep) noexcept
{
	EndPointIPV4 _ep;
	std::swap(_ep.handle, ep.handle);
	std::swap(_ep.handle, handle);
}

uint16_t EndPointIPV4::port()
{
	std::string ip;
	int32_t port;
	getSockAddrIn(ip, port);
	return (uint16_t)port;
}

std::string EndPointIPV4::ip()
{
	std::string ip;
	int32_t port;
	getSockAddrIn(ip, port);
	return ip;
}

    sockaddr * EndPointIPV4::toAddr()
    {
        return reinterpret_cast<sockaddr *>(raw());
    }

    size_t EndPointIPV4::addrSize() const
    {
        return sizeof(sockaddr_in);
    }


}
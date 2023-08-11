
#include "udpstream.hh"

namespace chrindex::andren::network
{
    UdpStream::UdpStream() {}
    UdpStream::UdpStream(UdpStream &&_) {}
    UdpStream::UdpStream(base::Socket &&sock) {}
    UdpStream::~UdpStream() {}

    void UdpStream::operator=(UdpStream &&_) {}

    void UdpStream::setEventLoop(std::weak_ptr<EventLoop> ev) {}

    void UdpStream::setEpoller(std::weak_ptr<base::Epoll> ep) {}

    bool UdpStream::bind() { return true; }

    bool UdpStream::start() { return true; }

    bool UdpStream::valid() const { return true; }

    base::Socket *UdpStream::handle() { return 0; }

    bool UdpStream::requestRead() { return true; }

    bool UdpStream::requestWrite(std::string const &data, base::EndPointIPV4 const &epv4) { return true; }

    bool UdpStream::requestWrite(std::string &&data, base::EndPointIPV4 &&epv4) { return true; }

    void UdpStream::processEvents() {}
}

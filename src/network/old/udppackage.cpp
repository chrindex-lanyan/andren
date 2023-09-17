
#include "udppackage.hh"

namespace chrindex::andren::network
{
    UdpPackage::UdpPackage() {}
    UdpPackage::UdpPackage(UdpPackage &&_) {}
    UdpPackage::UdpPackage(base::Socket &&sock) {}
    UdpPackage::~UdpPackage() {}

    void UdpPackage::operator=(UdpPackage &&_) {}

    void UdpPackage::setEventLoop(std::weak_ptr<TaskDistributor> ev) {}

    void UdpPackage::setEpoller(std::weak_ptr<base::Epoll> ep) {}

    bool UdpPackage::bind() { return true; }

    bool UdpPackage::start() { return true; }

    bool UdpPackage::valid() const { return true; }

    base::Socket *UdpPackage::handle() { return 0; }

    bool UdpPackage::requestRead() { return true; }

    bool UdpPackage::requestWrite(std::string const &data, base::EndPointIPV4 const &epv4) { return true; }

    bool UdpPackage::requestWrite(std::string &&data, base::EndPointIPV4 &&epv4) { return true; }

    void UdpPackage::processEvents() {}
}

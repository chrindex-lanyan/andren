
#include "afile.hh"
#include "eventloop.hh"
#include <cassert>
#include <fcntl.h>
#include <memory>


namespace chrindex::andren::network
{

        AFile::AFile(){}


        AFile::~AFile()
        {
            /// 为什么不调用aclose....
            /// 都开始析构了，怎么shared_from_this。
            m_file.close();
        }

        void AFile::setEventLoop(std::weak_ptr<EventLoop> wev)
        {
            m_wev = wev;
        }

        bool AFile::open(std::string const & path, int flags, int createMode)
        {
            bool bret = m_file.open(path, flags , createMode);

            if(!bret)
            {
                return false;
            }

            return bret;
        }


        bool AFile::aclose(OnControl const& onControl)
        {
            auto onC = onControl;
            return aclose(std::move(onC));
        }

        /// close文件。
        /// 该操作是异步的，在IO线程里完成。
        bool AFile::aclose(OnControl && onControl)
        {
            auto ev = m_wev.lock();
            if (!ev || !onControl)
            {
                return false;
            }
            return ev->addTask([self = shared_from_this(), onControl = std::move(onControl)]()
            {
                self->m_file.close();
                onControl(false);
            }, EventLoopTaskType::IO_TASK);
        }

        /// open文件.
        /// 该操作是异步的，在IO线程里完成。
        bool AFile::areopen(OnControl const & onControl, std::string const & path, int flags, int createMode )
        {
            auto onC = onControl;
            auto _path = path;
            return areopen(std::move(onC),std::move(_path),flags,createMode);
        }

        /// open文件.
        /// 该操作是异步的，在IO线程里完成。
        bool AFile::areopen(OnControl && onControl, std::string && path, int flags, int createMode )
        {
            auto ev = m_wev.lock();
            if(!ev || !onControl || path.empty())
            {
                return false;
            }
            return ev->addTask([self = shared_from_this()
                , onControl= std::move(onControl), path = std::move(path) , flags,createMode]()
            {
                if (self->m_file.handle() >0 ){self->m_file.close();}
                onControl(self->open(path,  flags,createMode));
            },EventLoopTaskType::IO_TASK);
        }

        /// 发送一个写请求。
        /// 写操作将在请求执行时在IO线程中进行,
        /// 并在完成后回调。
        bool AFile::asend(std::string const & data , OnWriteDone const & onWriteDone)
        {
            auto d= data;
            auto ow = onWriteDone;
            return asend(std::move(d),std::move(ow));
        }

        /// 发送一个写请求。
        /// 写操作将在请求执行时在IO线程中进行,
        /// 并在完成后回调。
        bool AFile::asend(std::string && data , OnWriteDone && onWriteDone)
        {
            auto ev = m_wev.lock();
            if(!ev || data.empty() || !onWriteDone)
            {
                return false;
            }
            return ev->addTask([self = shared_from_this(), data = std::move(data) , ow = std::move(onWriteDone)]()
            mutable{
                self->switchNonblock(false);
                ssize_t ret = self->m_file.write(data.c_str(), data.size());
                ow(ret , std::move(data));
            },EventLoopTaskType::IO_TASK);
        }

        /// 发送一个读请求。
        /// 读操作将在请求执行时在IO线程中进行,
        /// 并在完成后回调。
        /// 请注意，read必须是非阻塞的。
        bool AFile::aread(OnReadDone const & onReadDone)
        {
            auto oread = onReadDone;
            return aread(std::move(oread));
        }

        /// 发送一个读请求。
        /// 读操作将在请求执行时在IO线程中进行,
        /// 并在完成后回调。
        /// 请注意，read必须是非阻塞的。
        bool AFile::aread(OnReadDone && onReadDone)
        {
            auto ev = m_wev.lock();
            if(!ev || !onReadDone)
            {
                return false;
            }
            ev->addTask([self = shared_from_this(), oread = std::move(onReadDone)]()
            {
                self->switchNonblock(true);
                std::string data;
                std::string tmp(64 * 1024 ,0); // 64KB
                ssize_t ret = 0, count = 0;

                while(1)
                {
                    ret = self->m_file.readToBuf(tmp);
                    if(ret == 0 || (ret == -1 && errno == (EAGAIN | EWOULDBLOCK))) // OK
                    {
                        break;
                    }
                    if (ret >0){
                        count += ret;
                        data.insert(data.end(),tmp.begin(),tmp.begin()+ret);
                    }
                }
                oread(count == 0 ? ret : count,std::move(data));
            },EventLoopTaskType::IO_TASK);
            return true;
        }

        base::File & AFile::handle()
        {
            return m_file;
        }

        bool AFile::switchNonblock(bool enable)
        {
            int _flags = base::File::fcntl(m_file.handle(), F_GETFL, 0L); 

            if (!enable)
            { 
                _flags &= ~O_NONBLOCK;
            }
            else { 
                _flags |= O_NONBLOCK;
            }
            bool bret = (0 == m_file.fcntl(m_file.handle(),F_SETFL, 0L | _flags ));
            assert(bret);
            return bret;
        }

};


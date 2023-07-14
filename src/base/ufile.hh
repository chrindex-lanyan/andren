#pragma once

#include <string>

#include "noncopyable.hpp"

namespace chrindex ::andren::base
{

/**
     * @brief 基本文件操作类
     * 包装了fd，提供了一部分接口的包装。
     */
    class File : public noncopyable
    {
    public:
        File();

        File(int fd);

        File(File &&);

        ~File();

        File &operator=(File &&);

        explicit operator bool() const;

    public:
        /// @brief 打开指定路径的文件
        /// @param path 路径
        /// @param flags 文件选项
        /// @param createMode 文件选项（仅创建新文件时有效）
        /// @return 创建结果
        bool open(const std::string &path, int flags, int createMode = -1);

        /// @brief 关闭文件
        void close();

        /// @brief 从缓冲区里拿数据写
        /// @param data 缓冲区
        /// @param size 要写入的数据量
        /// @return 返回写入字节数，但返回-1为出错，并设置相应的errno值。
        ssize_t write(const char *data, size_t size);

        /// @brief 从缓冲区里拿数据写
        /// @param buf 缓冲区
        /// @return 返回写入字节数，但返回-1为出错，并设置相应的errno值。
        ssize_t writeByBuf(const std::string &buf);

        /// @brief 读取到缓冲区里
        /// @param buf 缓冲区
        /// @param buf_size 缓冲区大小
        /// @return 返回读入字节数，但返回-1为出错，并设置相应的errno值。
        ssize_t read(char *buf, size_t buf_size);

        /// @brief 读取到指定缓冲区里
        /// @param buf 缓冲区
        /// @return 返回读入字节数，但返回-1为出错，并设置相应的errno值。
        ssize_t readToBuf(std::string &buf);

        /// @brief 交换文件对象
        /// @param
        /// @return
        File &swap(File &);

        /// @brief 返回文件描述符
        /// @return 文件描述符
        int handle() const;

        /// @brief fd控制
        /// @param fd 文件描述符
        /// @param cmd 控制命令
        /// @return 错误码
        static int fcntl(int fd, int cmd);

        /// @brief fd控制
        /// @param fd 文件描述符
        /// @param cmd 控制命令
        /// @param arg 参数
        /// @return 错误码
        static int fcntl(int fd, int cmd, long arg);

        /// @brief fd控制
        /// @param fd 文件描述符
        /// @param cmd 控制命令
        /// @param lock 文件锁
        /// @return 错误码
        static int fcntl(int fd, int cmd, struct flock *lock);

        /// @brief  触发IO同步。
        static void sync();

        /// @brief 打开一个最小未使用FD，作为oldfd的复制。
        /// @param oldfd 要复制的fd
        /// @return 新fd
        static int dup(int oldfd);

        /// @brief 指定FD作为oldfd的复制，如果newfd已存在则会先关闭。
        /// @param oldfd 要复制的fd
        /// @param newfd 指定的fd
        /// @return 即newfd
        static int dup2(int oldfd, int newfd);

        /// @brief 递归创建路径中的文件夹。
        /// @param path 
        /// @return 成功为0
        static int createDirectories(std::string const &path, int mode, bool ignoreErr = false);

    private:
        int m_fd;
    };

}
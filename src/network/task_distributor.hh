
#pragma once


#include "../base/andren_base.hh"

namespace chrindex::andren::network
{
    enum TaskDistributorTaskType:int
    {
        SHCEDULE_TASK,  // 定时器任务（线程1~n）
        IO_TASK,        // io任务（线程1）
        FURTURE_TASK,   // 常规任务（线程1~n）
    };
    
    class TaskDistributor : public std::enable_shared_from_this<TaskDistributor> , base::noncopyable
    {
    public :

        using Task = std::function<void()>;

        TaskDistributor(uint32_t size = std::thread::hardware_concurrency());
        ~TaskDistributor();

        /// @brief 添加任务
        /// @param task 任务函数
        /// @param type 任务类型
        /// @return 添加成功与否
        bool addTask(Task task , TaskDistributorTaskType type);

        /// @brief 添加任务
        /// @param task 任务函数
        /// @param index 指派的线程。线程索引从0开始，不超过N。
        /// @return 添加成功与否
        bool addTask(Task task , uint32_t index);

        /// @brief 添加任务，并尽可能快地运行：
        /// 如果调用者和目标工作线程的线程ID一致，则 addTask_ASAP 被调用时，
        /// 立即执行task。
        /// 请注意，为了避免栈溢出，对于非一次性的任务，请尽量不要使用此接口。
        /// 此处所指的一次性任务是指，task不会在内部再次进行addTask_ASAP操作，
        /// 因为这种操作实际上使得task被直接地递归调用了。
        /// @param task 任务函数
        /// @param index 指派的线程。线程索引从0开始，不超过N。
        /// @return 添加成功与否
        bool addTask_ASAP(Task task , uint32_t index);

        /// @brief 启动事件循环
        /// 注意，该函数是非阻塞的。
        /// 不包含本线程在内的线程，数量共N个。
        /// @return 返回true为成功
        bool start();

        /// @brief 关闭。
        void shutdown();

        /// @brief 是否停止或者即将停止了
        /// @return 
        bool isShutdown()const ;

        std::thread::id threadId(uint32_t index)const;

    private :

        /// @brief 开始下一次的事件循环
        void startNextLoop(uint32_t index);

        /// @brief 工作线程函数
        /// @param index 线程号
        void work(uint32_t index);

    private :
        uint32_t m_size;
        std::mutex *m_cvmut;
        std::condition_variable *m_cv;
        std::thread * m_tpool;
        base::DBuffer<Task>* m_bqueForTask;
        std::atomic<bool> m_exit;
    };


}





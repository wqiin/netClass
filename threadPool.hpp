#ifndef _CThreadPool_HPP
#define _CThreadPool_HPP

/************************************************************************/
/* source code url: https://zhuanlan.zhihu.com/p/61464921                  */
/************************************************************************/

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <future>
#include <utility>


namespace UT{
class CThreadPool
{
public:
    CThreadPool(const CThreadPool & ) = delete;
    CThreadPool(const CThreadPool && ) = delete;
    CThreadPool & operator=(const CThreadPool &) = delete;
    CThreadPool & operator=(const CThreadPool &&) = delete;
    void * operator new(size_t) = delete;
    void * operator new[](size_t) = delete;

public:
    explicit CThreadPool(size_t nThreadNum) : m_bIsStop(false)
    {
        //when thread count being zero, the std::future.get() would be blocked
        if(0 == nThreadNum)
            nThreadNum += 1;

        for (size_t ii = 0; ii < nThreadNum; ++ii){
            m_vecWorkerThreads.emplace_back([this](){
                while(true){
                    std::function<void()> funcTask;
                    {
                        std::unique_lock<std::mutex> lockGuard(m_mtx);
                        m_cv.wait(lockGuard, [this]() { return m_bIsStop.load() || !m_queTasks.empty(); });
                        if (m_bIsStop.load() && m_queTasks.empty())
                            return;

                        funcTask = std::move(m_queTasks.front());
                        m_queTasks.pop();
                    }
                    funcTask();
                }
          });
        }
    }

    ~CThreadPool()
    {
        m_bIsStop.store(true);
        m_cv.notify_all();

        for (auto & workerThread : m_vecWorkerThreads){
            workerThread.join();
        }
    }

    template<typename Func, typename... Args>
    auto addTask(Func && func, Args&&... args) -> std::future<decltype(func(args...))>
    {
        if (m_bIsStop.load())
            throw std::runtime_error("Such the object had been deleted");

        auto pTask = std::make_shared<std::packaged_task<decltype(func(args...))()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

        {
            std::unique_lock<std::mutex> ul(m_mtx);
            m_queTasks.emplace([pTask]() { (*pTask)(); });
        }

        m_cv.notify_one();
        return pTask->get_future();
    }

private:
    std::atomic<bool> m_bIsStop;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    std::vector<std::thread> m_vecWorkerThreads;
    std::queue<std::function<void()> > m_queTasks;
};
}

#endif // CThreadPool_HPP

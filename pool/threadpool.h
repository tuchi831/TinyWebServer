#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <assert.h>


class ThreadPool {
public:
    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;
    // ������make_shared����new�����ͨ��new�ٴ��ݸ�shared_ptr���ڴ��ǲ������ģ�������ڴ���Ƭ��
    explicit ThreadPool(int threadCount = 8) : pool_(std::make_shared<Pool>()) { // make_shared:������ֵ���������ڶ�̬�ڴ��з���һ�����󲢳�ʼ����������ָ��˶����shared_ptr
        assert(threadCount > 0);
        for(int i = 0; i < threadCount; i++) {
            std::thread([this]() {
                std::unique_lock<std::mutex> locker(pool_->mtx_);
                while(true) {
                    if(!pool_->tasks.empty()) {
                        auto task = std::move(pool_->tasks.front());    // ��ֵ����ֵ,�ʲ�ת��
                        pool_->tasks.pop();
                        locker.unlock();    // ��Ϊ�Ѿ�������ȡ�����ˣ����Կ�����ǰ������
                        task();
                        locker.lock();      // ������Ҫȡ�����ˣ�����
                    } else if(pool_->isClosed) {
                        break;
                    } else {
                        pool_->cond_.wait(locker);    // �ȴ�,����������˾�notify��
                    }
                    
                }
            }).detach();
        }
    }

    ~ThreadPool() {
        if(pool_) {
            std::unique_lock<std::mutex> locker(pool_->mtx_);
            pool_->isClosed = true;
        }
        pool_->cond_.notify_all();  // �������е��߳�
    }

    template<typename T>
    void AddTask(T&& task) {
        std::unique_lock<std::mutex> locker(pool_->mtx_);
        pool_->tasks.emplace(std::forward<T>(task));
        pool_->cond_.notify_one();
    }

private:
    // ��һ���ṹ���װ�������������
    struct Pool {
        std::mutex mtx_;
        std::condition_variable cond_;
        bool isClosed;
        std::queue<std::function<void()>> tasks; // ������У���������Ϊvoid()
    };
    std::shared_ptr<Pool> pool_;
};

#endif

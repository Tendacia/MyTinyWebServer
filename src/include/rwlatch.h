//
// Created by tendacia on 24-4-1.
//

#ifndef MYTINYWEBSERVER_RWLATCH_H
#define MYTINYWEBSERVER_RWLATCH_H


#include <condition_variable>
#include <mutex>

class ReaderWriterlatch{


public:
    ReaderWriterlatch() = default;
    ~ReaderWriterlatch(){std::lock_guard<std::mutex> guard(mutex_);}


    void WLock()
    {
        std::unique_lock<std::mutex> latch(mutex_);

        reader_cond.wait(latch, [&](){return !writer_entered_; });

        writer_entered_ = true;

        writer_cond.wait(latch, [&](){return reader_count_ == 0;});

    }

    void WUnlock()
    {
        std::lock_guard<std::mutex> latch(mutex_);

        writer_entered_ = false;
        reader_cond.notify_all();
    }

    void RLock()
    {
        std::unique_lock<std::mutex> latch(mutex_);
        reader_cond.wait(latch, [&](){return !writer_entered_;});

        reader_count_++;

    }

    void RUnlock()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        reader_count_--;
        if(writer_entered_)
        {
            if(reader_count_ == 0)
            {
                writer_cond.notify_one();
            }
        }
        else
        {
            reader_cond.notify_one();
        }
    }



private:
    std::mutex mutex_;
    std::condition_variable writer_cond;
    std::condition_variable reader_cond;

    uint32_t reader_count_{0};
    bool writer_entered_{false};
};









#endif //MYTINYWEBSERVER_RWLATCH_H

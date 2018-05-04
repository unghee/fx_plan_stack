#ifndef __CIRCULAR_BUFFER_H
#define __CIRCULAR_BUFFER_H

#include <cstdio>
#include <memory>
#include <mutex>

/* 	Credit where credit is due:
    This circular buffer implementation was taken almost exactly as is from
    https://embeddedartistry.com/blog/2017/4/6/circular-buffers-in-cc

    Changes made were:
         - get is not a const member since it moves the tail
         - added a count()
         - added a peak()
*/

/*
* Important Usage Note: This library reserves one spare entry for queue-full detection
* Otherwise, corner cases and detecting difference between full/empty is hard.
* You are not seeing an accidental off-by-one.
*/

/// \brief Simple and robust circular buffer template class.
/// This container is used for storing incoming data. It is fixed size, thus isn't slowed down by memory
/// allocation. Additionally, old data isn't useful data for us, so we don't worry too much about
/// overwriting old values.
template <class T>
class circular_buffer {
public:
    explicit circular_buffer(size_t size=10) :
        buf_(std::unique_ptr<T[]>(new T[size])),
        size_(size), head_(0), tail_(0), count_(0)
    {
        //Empty constructor
    }

    /// \brief Places an item into the buffer, potentially overwriting
    /// head_ stores the next position to place an item. put places the item,
    /// checks for overflow, and updates members accordingly.
    void put(T item)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        buf_[head_] = item;
        // check if we just overwrote
        if(count_ == size_)
        {
            tail_ = (tail_ + 1) % size_;
        }
        else
        {
            count_++;
        }
        head_ = (head_ + 1) % size_;
    }
    /// \brief Takes an item from the buffer
    /// _tail holds the position of the next item to take. If there are no items
    /// to take, get returns a default item.
    T get(void)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if(empty())
        {
            return T();
        }

        //Read data and advance the tail (we now have a free space)
        auto val = buf_[tail_];
        tail_ = (tail_ + 1) % size_;
        count_--;

        return val;
    }

    /// \brief Returns an item by const ref without removing it from the buffer
    const T& peek(size_t idx) const
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if(empty() || idx >= count_)
        {
            return defaultT_;
        }

        //Read data without advancing tail
        return buf_[(tail_+idx) % size_];
    }

    /// \brief Returns the last item in the buffer by const ref without removing it from the buffer
    const T& peekBack() const
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if(empty())
        {
            return defaultT_;
        }

        if(head_ == 0)
            return buf_[size_-1];
        else
            return buf_[head_-1];
    }

    /// \brief Empties the buffer
    void reset(void)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = tail_;
        count_ = 0;
    }

    /// \brief Checks if the buffer is empty
    bool empty(void) const
    {
        //if head and tail are equal, we are empty
        return count_ == 0;
    }

    /// \brief Checks if the buffer is full
    bool full(void) const
    {
        //If tail is ahead the head by 1, we are full
        return size_ == count_;
    }

    /// \brief Returns the number of items the buffer can carry
    size_t capacity(void) const
    {
        return size_;
    }

    /// \brief Returns the number of items in the buffer currently
    size_t count(void) const
    {
        return count_;
    }

private:
    mutable std::mutex mutex_;
    std::unique_ptr<T[]> buf_;
    size_t size_;
    size_t head_;
    size_t tail_;
    size_t count_;

    T defaultT_;
};

#endif //__CIRCULAR_BUFFER_H

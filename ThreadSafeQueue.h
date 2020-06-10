#pragma once
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

template <class T>
class ThreadSafeQueue
{
public:
   ThreadSafeQueue(int64_t size = 40, bool removeOldestWhenFull = true)
   : m_size(size)
   , m_removeOldestWhenFull(removeOldestWhenFull)
   {
   }

   bool push(T data)
   {
      std::lock_guard<std::mutex> lk(mut);
      if (current_size == m_size && m_removeOldestWhenFull)
      {
          data_queue.pop_front();
          data_queue.push_back(data);
          return true;
      }
      else if (current_size == m_size && !m_removeOldestWhenFull)
      {
         return false; // Queue is full and we do not remove the oldest item.
      }
      else
      {
        data_queue.push_back(data);
        current_size++;
        return true;
      }
   }


   bool tryPop(T& value)
   {
      std::lock_guard<std::mutex> lk(mut);
      if (data_queue.empty())
      {
         return false;
      }
      value = std::move(data_queue.front());
      data_queue.pop_front();
      current_size--;
      return true;
   }

   bool copyRawQueue(std::deque<T>& copy)
   {
       std::lock_guard<std::mutex> lk(mut);
       std::copy(data_queue.begin(), data_queue.end(), copy.begin());
       if(copy.size() == data_queue.size()) // weak check if the copying went OK.
       {
           return true;
       }
           return false;
   }

   bool empty() const
   {
      std::lock_guard<std::mutex> lk(mut);
      return data_queue.empty();
   }

   void clearAndShrink()
   {
      std::lock_guard<std::mutex> lk(mut);
      data_queue.clear();
      // Since the queue may still hold allocated and unused memory, we free it.
      data_queue.shrink_to_fit();
      current_size = 0;
   }

   void clear()
   {
      std::lock_guard<std::mutex> lk(mut);
      data_queue.clear();;
      current_size = 0;
   }

   int64_t size()
   {
       std::lock_guard<std::mutex> lk(mut);
       return current_size;
   }

   bool isFull()
   {
       std::lock_guard<std::mutex>lk(mut);
       return current_size == m_size;
   }

private:
   mutable std::mutex mut;

   std::deque<T> data_queue;

   int64_t m_size;

   int64_t current_size = 0;

   bool m_removeOldestWhenFull;
};

#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <mutex>
#include <condition_variable>
#include <string>
#include <thread>
#include "ThreadSafeQueue.h"

class MessageItem{

public:

    MessageItem(int32_t bufferSizeInBytes)
        :m_bufferSizeInBytes(bufferSizeInBytes)
        ,m_buffer(new char[bufferSizeInBytes])
    {}

    MessageItem()
    {}

    // If this item is used for messages of different sizes.
    void formatMessage(int32_t nrOfValidBytes)
    {
        m_nrOfValidBytes = nrOfValidBytes;
    }

    void writeMessage(const char* data_ptr, size_t size)
    {
       if (size <= m_bufferSizeInBytes)
       {
          std::memcpy(m_buffer, data_ptr, size);
       }
    }
    size_t m_bufferSizeInBytes; // nr of Bytes.
    char* m_buffer;
    size_t m_nrOfValidBytes;
};

template<int32_t MessageSize = 40, int32_t QueueSize = 1600>
class AsyncFile
{
public:

     AsyncFile()
         :m_queue(40)
         ,tempBuffer()
    {
          tempBuffer.resize(40);
          std::thread m_worker(&AsyncFile::logging_thread, this);
          m_worker.detach(); // let the logging_thread live on its own.
    }

   ~AsyncFile()
    {
        finished = true;
    }

     void write(MessageItem& buffer) // 40 should probably be a
                                                         // configurable parameter.
     {
        std::unique_lock<std::mutex> lk(queue_mutex);
        m_queue.push(buffer);
        if (m_queue.isFull()) // fill the queue, 40 char each time, until we have 16000 chars in total. Then
                                                // notify the writing thread we are ready.
        {
           m_queue.copyRawQueue(tempBuffer);
           m_queue.clear();
           full = true;
           cv_write.notify_one();
        }
        lk.unlock();
     }

   void done()
   {
       finished = true;
   }

private:

    ThreadSafeQueue<MessageItem> m_queue;

    std::deque<MessageItem> tempBuffer;

    std::mutex queue_mutex;

    std::mutex write_mutex;

    std::condition_variable cv_read;

    std::condition_variable cv_write;

    size_t counter = 0;

    bool finished = false;

    bool full = false;

    void logging_thread()
    {
        FILE* file = fopen("test_file.bin", "wb");

        while(!finished) // Pass "finished" as a parameter by ref? Protect by a mutex?
        {
            std::unique_lock<std::mutex> lk(write_mutex);
            cv_write.wait(lk, [&]{return full;});
            for(auto item : tempBuffer)
            {
                fwrite(&item.m_buffer, sizeof(char), item.m_nrOfValidBytes, file);
            }
            full = false;
        }
        fclose(file);
    }

}; // class AsyncFile

//template class CircularQueue<1600,40,char>;

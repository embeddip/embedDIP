#pragma once

extern "C"
{
#include "core/memory_manager.h"
}

#include <cstddef>
#include <utility>
#include <cstring>

namespace embedDIP
{

    /**
     * @brief RAII wrapper for raw memory allocated by memory_alloc/memory_free
     */
    class Memory
    {
    public:
        Memory() : ptr_(nullptr), size_(0) {}

        explicit Memory(std::size_t size)
            : ptr_(memory_alloc(size)), size_(size)
        {
            // On failure, ptr_ will be nullptr - check with get() or bool operator
            // No exceptions in embedded systems
        }

        ~Memory()
        {
            if (ptr_)
            {
                memory_free(ptr_);
            }
        }

        // Non-copyable
        Memory(const Memory &) = delete;
        Memory &operator=(const Memory &) = delete;

        // Movable
        Memory(Memory &&other) noexcept
            : ptr_(other.ptr_), size_(other.size_)
        {
            other.ptr_ = nullptr;
            other.size_ = 0;
        }

        Memory &operator=(Memory &&other) noexcept
        {
            if (this != &other)
            {
                if (ptr_)
                {
                    memory_free(ptr_);
                }
                ptr_ = other.ptr_;
                size_ = other.size_;
                other.ptr_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }

        void *data() { return ptr_; }
        const void *data() const { return ptr_; }

        std::size_t size() const { return size_; }

    private:
        void *ptr_;
        std::size_t size_;
    };

    /**
     * @brief RAII wrapper for typed memory (like malloc() + array of T)
     */
    template <typename T>
    class MemoryBlock
    {
    public:
        explicit MemoryBlock(std::size_t count)
            : ptr_(static_cast<T *>(memory_alloc(sizeof(T) * count))), count_(count)
        {
            // On failure, ptr_ will be nullptr - check with get() or bool operator
            // No exceptions in embedded systems
        }

        ~MemoryBlock()
        {
            if (ptr_)
            {
                memory_free(ptr_);
            }
        }

        // Disable copy
        MemoryBlock(const MemoryBlock &) = delete;
        MemoryBlock &operator=(const MemoryBlock &) = delete;

        // Enable move
        MemoryBlock(MemoryBlock &&other) noexcept
            : ptr_(other.ptr_), count_(other.count_)
        {
            other.ptr_ = nullptr;
            other.count_ = 0;
        }

        MemoryBlock &operator=(MemoryBlock &&other) noexcept
        {
            if (this != &other)
            {
                if (ptr_)
                {
                    memory_free(ptr_);
                }
                ptr_ = other.ptr_;
                count_ = other.count_;
                other.ptr_ = nullptr;
                other.count_ = 0;
            }
            return *this;
        }

        T *data() { return ptr_; }
        const T *data() const { return ptr_; }

        std::size_t size() const { return count_; }

        T &operator[](std::size_t i) { return ptr_[i]; }
        const T &operator[](std::size_t i) const { return ptr_[i]; }

    private:
        T *ptr_;
        std::size_t count_;
    };

} // namespace embedDIP

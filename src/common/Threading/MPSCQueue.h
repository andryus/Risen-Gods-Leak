/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <atomic>
#include <utility>
#include <optional>

// C++ implementation of Dmitry Vyukov's lock free MPSC queue
// http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
template<typename T>
class MPSCQueue
{
public:
    MPSCQueue() : _head(new Node()), _tail(_head.load(std::memory_order_relaxed))
    {
        Node* front = _head.load(std::memory_order_relaxed);
        front->Next.store(nullptr, std::memory_order_relaxed);
    }

    ~MPSCQueue()
    {
        while (this->Dequeue());

        Node* front = _head.load(std::memory_order_relaxed);
        delete front;
    }
    
    template<typename... Args>
    void Enqueue(Args&&... args)
    {
        Node* node = new Node(std::forward<Args>(args)...);
        EnqueueNode(*node);
    }

    void Enqueue(T&& input)
    {
        Node* node = new Node(std::move(input));
        EnqueueNode(*node);
    }

    std::optional<T> Dequeue(void)
    {
        Node* tail = _tail.load(std::memory_order_relaxed);
        Node* next = tail->Next.load(std::memory_order_acquire);
        if (!next)
            return {};

        T result = std::move(next->Data);
        _tail.store(next, std::memory_order_release);
        delete tail;

        return result;
    }

private:

    struct Node
    {
        Node() = default;
        explicit Node(T&& data) : Data(std::move(data)) { Init(); }
        template<typename... Args>
        explicit Node(Args&&... args) : Data(std::forward<Args>(args)...) { Init(); }

        T Data;
        std::atomic<Node*> Next;

    private:

        void Init() { Next.store(nullptr, std::memory_order_relaxed); }
    };

    void EnqueueNode(Node& node)
    {
        Node* prevHead = _head.exchange(&node, std::memory_order_acq_rel);
        prevHead->Next.store(&node, std::memory_order_release);
    }

    std::atomic<Node*> _head;
    std::atomic<Node*> _tail;

    MPSCQueue(MPSCQueue const&) = delete;
    MPSCQueue& operator=(MPSCQueue const&) = delete;
};


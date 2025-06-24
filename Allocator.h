// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#pragma once

#include "Ast.h"
#include "Location.h"
#include "DenseHash.h"
#include "Common.h"

#include <vector>

namespace Luau
{

    class Allocator
    {
    public:
        Allocator();
        Allocator(Allocator&&);

        Allocator& operator=(Allocator&&) = delete;

        ~Allocator();

        void* allocate(size_t size);

        template<typename T, typename... Args>
        T* alloc(Args&&... args)
        {
            static_assert(std::is_trivially_destructible<T>::value, "Objects allocated with this allocator will never have their destructors run!");

            T* t = static_cast<T*>(allocate(sizeof(T)));
            new (t) T(std::forward<Args>(args)...);
            return t;
        }

    private:
        struct Page
        {
            Page* next;

            char data[8192];
        };

        Page* root;
        size_t offset;
    };

}

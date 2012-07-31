//
//  GLTest.hpp
//  Megacanvas
//
//  Created by Joe Groff on 7/3/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_GLTest_hpp
#define Megacanvas_GLTest_hpp

#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include "Engine/Util/GLMeta.hpp"
#include "Engine/Vec.hpp"
#include <algorithm>
#include <utility>

namespace Mega { namespace test {
    extern "C" {
        int createTestGLContext(void);
        void destroyTestGLContext(void);
        void syncTestGLContext(void);
    }
    
    class GLContextTestFixture : public CppUnit::TestFixture {
    public:
        void setUpTestFramebuffer();
        void setUp() override
        {
            if (!createTestGLContext())
                throw std::runtime_error("unable to create opengl context");
            this->setUpTestFramebuffer();
        }

        void tearDown() override
        {
            destroyTestGLContext();
        }
    };
    
    template<typename T, std::size_t N, typename...TT>
    inline bool arrayEquals(T const (&array)[N], TT &&...values)
    {
        static_assert(sizeof...(TT) == N, "must pass an expected value for every element of the array");
        T expected[N] = {T(std::forward<TT>(values))...};
        return std::equal(std::begin(array), std::end(array), std::begin(expected));
    }

    template<typename T, typename...TT>
    inline bool arrayEquals(llvm::ArrayRef<T> array, TT &&...values)
    {
        if (sizeof...(TT) != array.size())
            return false;
        T expected[sizeof...(TT)] = {T(std::forward<TT>(values))...};
        return std::equal(std::begin(array), std::end(array), std::begin(expected));
    }
    
    template<typename T, typename...TT>
    inline void pValues(std::ostream &os, T &&x, TT &&...xx)
    {
        os << std::forward<T>(x);
        char __attribute__((unused)) pass[] = {(os << ", " << std::forward<TT>(xx), '\0')...};
    }
    
    template<typename T, std::size_t N>
    inline void pArray(std::ostream &os, T (&arr)[N])
    {
        if (N > 0) {
            os << arr[0];
            for (T *i = arr+1; i < std::end(arr); ++i)
                os << ", " << *i;
        }
    }
    
    template<typename T>
    inline void pArray(std::ostream &os, llvm::ArrayRef<T> arr)
    {
        if (arr.size() > 0) {
            os << arr[0];
            for (T const *i = arr.begin()+1; i < arr.end(); ++i)
                os << ", " << *i;
        }
    }
    
    template<typename T, typename...UU>
    inline std::string assertArrayEqualsMessage(T &&actual, UU &&...expected)
    {
        std::ostringstream os;
        os << "array did not match expected value"
              "\n  expected: ";
        pValues(os, std::forward<UU>(expected)...);
        os << "\n  actual:   ";
        pArray(os, std::forward<T>(actual));
        return os.str();
    }
}}

namespace std {
    template<typename T, size_t N>
    inline std::ostream &operator<<(std::ostream &os, std::array<T,N> a) {
        Mega::test::pArray(os, llvm::ArrayRef<T>{a.begin(), a.end()});
        return os;
    }
    inline std::ostream &operator<<(std::ostream &os, uint8_t byte) {
        return os << (int)byte;
    }
}

namespace Mega {
    std::ostream &operator<<(std::ostream &os, GLError err);
    std::ostream &operator<<(std::ostream &os, Vec vec);
}
#define MEGA_CPPUNIT_ASSERT_GL_NO_ERROR \
    CPPUNIT_ASSERT_EQUAL(GLError(GL_NO_ERROR), GLError(glGetError()))
#define MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(actual, ...) \
    CPPUNIT_ASSERT_MESSAGE(assertArrayEqualsMessage(actual, __VA_ARGS__), arrayEquals(actual, __VA_ARGS__))
#endif

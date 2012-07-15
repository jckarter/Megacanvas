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
#include <algorithm>
#include <utility>

extern "C" {
    int createTestGLContext(void);
    void destroyTestGLContext(void);
    void syncTestGLContext(void);
}

namespace Mega { namespace test {
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
    
    template<typename T, std::size_t N, typename...UU>
    inline std::string assertArrayEqualsMessage(T (&actual)[N], UU &&...expected)
    {
        std::ostringstream os;
        os << "array did not match expected value"
              "\n  expected: ";
        pValues(os, std::forward<UU>(expected)...);
        os << "\n  actual:   ";
        pArray(os, actual);
        return os.str();
    }
}}

namespace Mega {
    std::ostream &operator<<(std::ostream &os, GLError err);
}
#define MEGA_CPPUNIT_ASSERT_GL_NO_ERROR \
    CPPUNIT_ASSERT_EQUAL(GLError(GL_NO_ERROR), GLError(glGetError()))
#define MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(actual, ...) \
    CPPUNIT_ASSERT_MESSAGE(assertArrayEqualsMessage(actual, __VA_ARGS__), arrayEquals(actual, __VA_ARGS__))
#endif

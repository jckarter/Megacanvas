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
#include "Engine/Util/GL.h"
#include <algorithm>

#ifdef __cplusplus
extern "C" {
#endif
    int createTestGLContext(void);
    void destroyTestGLContext(void);
#ifdef __cplusplus
}
#endif

namespace Mega { namespace test {
    class GLContextTestFixture : public CppUnit::TestFixture {
    public:
        void setUp() override
        {
            if (!createTestGLContext())
                throw std::runtime_error("unable to create opengl context");
        }

        void tearDown() override
        {
            destroyTestGLContext();
        }
    };
    
    template<typename T, size_t N, typename...TT>
    inline bool arrayEquals(T const (&array)[N], TT &&...values)
    {
        static_assert(sizeof...(TT) == N, "must pass an expected value for every element of the array");
        T expected[N] = {T(std::forward<TT>(values))...};
        return std::equal(std::begin(array), std::end(array), std::begin(expected));
    }
}}

#define MEGA_GL_ASSERT_NO_ERROR CPPUNIT_ASSERT_EQUAL(GLenum(GL_NO_ERROR), glGetError())

#endif

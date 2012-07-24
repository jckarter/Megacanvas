//
//  ViewTest.cpp
//  Megacanvas
//
//  Created by Joe Groff on 7/3/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "GLTest.hpp"
#include "Engine/Canvas.hpp"
#include "Engine/View-priv.hpp"

namespace Mega { namespace test {
    class ViewTest : public GLContextTestFixture {
        CPPUNIT_TEST_SUITE(ViewTest);
        CPPUNIT_TEST_SUITE_END();

        Owner<Canvas> canvas;
        Owner<View> view;

    public:
        void setUp() override
        {
            this->GLContextTestFixture::setUp();
            std::string error;
            this->canvas = Canvas::load("EngineTests/TestData/Test1.mega", &error);
            if (!this->canvas)
                throw std::runtime_error(error);
            this->view = View::create(this->canvas.get());
            if (!this->view->prepare(&error))
                throw std::runtime_error(error);
            this->view->resize(127.0, 127.0);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
        }

        void tearDown() override
        {
            this->view = nullptr;
            this->canvas = nullptr;
            this->GLContextTestFixture::tearDown();
        }

    };
    CPPUNIT_TEST_SUITE_REGISTRATION(ViewTest);
}}

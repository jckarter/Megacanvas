//
//  main.cpp
//  EngineTests
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

namespace Mega {
    char const *shaderPath = "Engine/Shaders";
}

int main(int argc, const char * argv[])
{
    CppUnit::TextUi::TestRunner runner;
    auto &registry = CppUnit::TestFactoryRegistry::getRegistry();
    runner.addTest(registry.makeTest());
    runner.run();
    return 0;
}

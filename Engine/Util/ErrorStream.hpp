//
//  ErrorStream.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/27/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_ErrorStream_hpp
#define Megacanvas_ErrorStream_hpp

#include <sstream>
#include <stdexcept>

#define MEGA_STRING(inserts) \
    (static_cast<::std::ostringstream const &>(::std::ostringstream() << inserts).str())

#define MEGA_RUNTIME_ERROR(inserts) \
    throw ::std::runtime_error(MEGA_STRING(inserts))

namespace Mega {
    
}

#endif

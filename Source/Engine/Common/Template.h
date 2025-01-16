#pragma once
#include "STL14.h"

namespace Inno
{
    template <typename U, bool cond>
    using EnableType = typename std::enable_if<cond, U>::type;

    template <typename U, bool cond>
    using DisableType = typename std::enable_if<!cond, U>::type;
}
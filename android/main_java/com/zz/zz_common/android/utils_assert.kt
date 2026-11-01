package com.zz.zz_common.android

//断言
fun utils_assert(value: Boolean)
{
    if (!value)
        throw AssertionError("zz_common: assert fail")
}
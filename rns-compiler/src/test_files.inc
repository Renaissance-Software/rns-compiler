#pragma once

#define NEW_TEST(_src_file_) { #_src_file_, (s64)strlen(#_src_file_) }
const RNS::String test_files[] = {
    NEW_TEST(
        main :: ()
        {
        }
    ),
    NEW_TEST(
        main :: ()
        {
            return;
        }
    ),
    NEW_TEST(
        main :: () -> s32
        {
            return 0;
        }
    ),
};

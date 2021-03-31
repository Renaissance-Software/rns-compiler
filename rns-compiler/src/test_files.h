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
    NEW_TEST(
    main :: () -> s32
    {
        sum: s32 = 0;
        for i : 4
        {
            if i == 2
            {
                break;
            }

            sum = sum + i;
        }

        return sum;
    }
    ),
};

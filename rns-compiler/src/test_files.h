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
    NEW_TEST(
    main :: () -> s32
    {
    sum: s32 = 0;
        for i : 4
        {
            if i > 1
            {
                if i == 2
                {
                    break;
                }
            }

            sum = sum + i;
        }

        return sum;
    }
    ),

// https://godbolt.org/z/E9s4xzbWb
        NEW_TEST(main :: () -> s32
    {
    sum: s32 = 0;
    for i : 4
    {
        if i > 1
        {
            if i == 2
            {
                sum = sum * sum;
                break;
            }
            sum = sum + 1;
        }
        else
        {
            sum = sum + 12;
            if i == 3
            {
                sum = sum + 1231;
                if sum == 1231
                {
                    for j : 10
                    {
                        sum = sum + 1;
                    }
                }
                sum = sum + sum;
            }
            else if i == sum
            {
                sum = sum - 100;
            }
            else
            {
                sum = sum - 3;
                break;
            }
            sum = sum + 2;
        }
        sum = sum + i;
    }
    return sum;
    }
    ),
    // https://godbolt.org/z/c46PvWdzE
        NEW_TEST(
            main :: () -> s32
    {
        a: s32 = 5;
        if a > 5
        {
            if a > 10
            {
                if a > 16
                {
                    return 50;
                }
                else if a > 14
                {
                    if a == 15
                    {
                        return 15;
                    }
                    else
                    {
                        return 16;
                    }
                }
                else
                {
                    if a == 11
                    {
                        return 11;
                    }
                }
            }
        }
        else if a > 0
        {
            if a == 3
            {
                return 3;
            }
        }
        else
        {
            return 0;
        }
        return 6;
    }
    ),
        // @TODO: This test crashes the compiler because a return has been emitted after all paths emitted a return
        // Think of skipping the dead code. How?
        // @UPDATE: this is no longer crashing. We are skipping the rest of the block statements; the return is implicit Is this approach any good? - davidgm94 - April 3, 2021
        NEW_TEST(
            main :: () -> s32
    {
        a: s32 = 0;
        if (a == 0)
        {
            return 1;
        }
        else
        {
            return 0;
        }
        
        return 2;
    }

    ),
        NEW_TEST(
    foo :: () -> s32
    {
        return 5;
    }
    main :: () -> s32
    {
        a: s32 = foo() + 1;
        return a + foo();
    }
    ),
        NEW_TEST(
            main :: () -> s32
    {
        a: s32 = 5;
        b: &s32 = &a;
        if @b == 5
        {
            @b = 6;
        }
        else
        {
            @b = 3;
        }
        return @b;
    }
    ),
        NEW_TEST(
            foo :: (a: s32, b: s32) -> s32
    {
        return a + b;
    }
    main :: () -> s32
    {
    a: s32 = 1;
    b: s32 = 2;
        return foo(a, b);
    }
    ),
        NEW_TEST(
            add_one :: (n: &s32)
    {
        if @n == 2
        {
            @n = @n + 1;
        }
        else
        {
            @n = @n - 1;
        }
    }
    main :: () -> s32
    {
        a: s32 = 5;
        add_one(&a);
        return a;
    }
    ),
};

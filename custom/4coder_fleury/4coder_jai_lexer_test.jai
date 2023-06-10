jai_test :: (some_var : int)
{
    a : my_struct;
    a.a;
    a.abcdefg;
    //b : my_enum;
    {
        a.abcdefg;
    }
    printf(some_var);
}

child_struct :: struct
{
    b : my_enum;
}

my_struct :: struct
{
    a : int;
    b : child_struct;
    abcdefg : int;
}

my_enum :: enum
{
    FirstEnum;
    SecondEnum;
}
#define A \
    bla \
    foo \
    bar \
    baz \
    \

[MyAttribute]
void main()
{
    if (a &&
        b)
        DoC();
    if (a[]
        && b)
        DoC();
    if (a[] &&
        b)
        DoC();
    if (a
        []
        &&
        b)
        DoC();
    
    a[i] = a + b;
    a[i] =
        a + b;
    a[i]
        = a + b;
    
    a[i] = a +
        b
    ;
    
    if (false)
    ;
    
    int a, int
        b, int c;
    
    {{{{{{
                            {
                                {
                                    
                                }
                            }
                        }}}}}}
    
    [Attribute]
    [OtherAtt]
    class MyClass
    {
        [AttributeA] public int a;
        
        [AttributeB]
        public int
            b1[5];
        [AttributeB] public int b2
            [5];
        
        [AttributeC]
        int[] 
            DoShit(int[] a,
                   int[] b);
        
        [AttributeD]
        [Att]
        int[] d;
    }
    
    char* option_names[] =
    {
        "OPTION 1",
        "OPTION 2",
        "OPTION 3",
    };
    
    int num = GetNum
        (a, b, c,
         d, e, f)
        + a / b
        * c[i]
        - a
        [b];
    
    if (a)
        b = c +
            d;
    
    if (a)
        a = new Test
        {
        };
    
    if (a)
        if (b)
            if (c)
                DoSomeThing();
    
    if (a)
        for (int i = 0;
             i < 100;
             ++i)
            if (a[i] == 10 &&
                b[i] == 100)
                DoOtherStuff();
    
    while (true)
        for (;;)
            if (false)
                a;
    
    FuncA(a,
          FuncB(b,
                FuncC(c,
                      FuncD()
                      )
                )
          );
    
    // NOTE(long): I could make code inside paren indent correctly,
    // but it would be more complicated and I would have to modify the layout_index_x_shift function.
    // Define LONG_INDEX_INDENT_PAREN if you want to try it.
    {
        (
         a =
         new A
         {
         };
         );
        
        (
         a = a +
         b;
         )
        
        A(something,
          if (a)
          if (b)
          if (c)
          if (d)
          DoStuff();
          if (c)
          a = new A
          {
          };
          if (d)
          if (e)
          a = new A
          {
          };
          DoOtherStuff());
    }
    
    // NOTE(long): These are incorrect but who give a shit
    {
        if (d)
            if (e)
                SomeOtherThing = 
                    a +
                    b;
        else
            Sad();
        
        if (a)
            do
            {
            }
        while (0);
    }
}

{
    
    {
        
        {
            
        }
        
        {
            
            {
                
            }
            
            {
                
            }
            
            {
                
            }
            
        }
        
        {
            
        }
        
    }
    
    {
        
        {
            
        }
        
    }
    
}

{
    
    {
        
        {
            
        }
        
    }
    
    {
        
        {
            
        }
        
    }
    
}

{
    
    {
        
        {
            
        }
        
    }
    
    {
        
        {
            
        }
        
    }
    
}

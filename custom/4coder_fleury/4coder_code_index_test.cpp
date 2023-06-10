
{
    {
        
    }
}

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
    if (a[]
        &&
        b)
        DoC();
    
    a[i] = a + b;
    a[i] =
        a + b;
    a[i]
        = a + b;
    
    if (a)
        b = c +
        d;
    
    if (a)
        if (b)
        if (c)
        DoSomeThing();
    
    if (a)
        for (int i = 0; i < 100; ++i)
        if (a[i] == 10 &&
            b[i] == 100)
        DoOtherStuff();
    
    while (true)
        for (;;)
        if (false)
        a;
    
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

// Simple test cases for the c4 compiler to find leaks and errors


// Case 1: hello world
// main()
// {
//     printf("Hello, World!\n");
//     return 0;
// }

// Case 2: If Statments:
main()
{
    int a;
    a = 5;
    if (a < 10)
      printf("a is less than 10\n");
    else
      printf("a is 10 or greater\n");
    return 0;
}
  

// Case 3: While loop
// main()
// {
//     int i;
//     i = 0;
//     while (i < 5)
//     {
//       printf("i = %d\n", i);
//       i = i + 1;
//     }
//     return 0;
// }
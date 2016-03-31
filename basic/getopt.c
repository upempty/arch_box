#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv)
{
  char *cstr = NULL;
  int index;
  int c;

  while ((c = getopt (argc, argv, "abc:")) != -1)
  {
    printf("in while to get c=%c, optarg=%s\n", c, optarg);
    switch (c)
    {
      case 'a':
        printf("a optind=%d,argc=%d\n", optind, argc);
        break;
      case 'b':
        printf("b optind=%d,argc=%d\n", optind, argc);
        break;
      case 'c':
        cstr = optarg;
        break;
      case '?':
        printf("help ?\n");
        return 1;
      default:
        printf("others optind=%d\n", optind);
    }
  }

  for (index = 0; index < argc; index++)
    printf ("ind=%d, %s\n", index, argv[index]);

  for (index = optind; index < argc; index++)
    printf ("ind=%d,Non-option argument %s\n", index, argv[index]);

  return 0;

}
/* output */
/*
[root@localhost sanbox]# gcc getopt.c 
[root@localhost sanbox]# ./a.out -b btext -a atext -c ctext
in while to get c=b, optarg=(null)
b optind=2,argc=7
in while to get c=a, optarg=(null)
a optind=4,argc=7
in while to get c=c, optarg=ctext
ind=0, ./a.out
ind=1, -b
ind=2, -a
ind=3, -c
ind=4, ctext
ind=5, btext
ind=6, atext
ind=5,Non-option argument btext
ind=6,Non-option argument atext
 */

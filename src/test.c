#include <stdio.h>
#include <stdlib.h>

#include "gem.h"

int main(/*@unused@*/ int argc, /*@unused@*/ char *argv[])
{
  int      status;
  gemCntxt *context;

  status = gem_initialize(&context);

  status = gem_terminate(context);

  return 0;
}

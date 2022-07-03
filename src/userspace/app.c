#include <stdint.h>

#include "sys.h"

static uint64_t a;
static uint64_t b;

static void fibNext() {
    uint64_t c = a;
    a = b;
    b = a + c;
}

void main() {
    a = 1;
    b = 1;

    while (b < 10000000000000000000ull)
        fibNext();

    printf("Hi,  I'm app; I've stopped fib-ing with a: %u and b: %u\n", a, b);

    // print("Say something, please? ");
    // char* l = M_readline();
    // if (l) {
    //     printf("  Thanks for saying \"%s\"\n", l);
    //     free(l);
    // }

    const uint64_t chunk = 100000000ull / 8; // Chunk that's not too fast, not too slow, for target system
    //asm volatile("xchgw %bx, %bx");
    printf("We'll stop when a is %u\n", chunk * 10);
    for (a = 0; a < chunk; a++)
        if (a % chunk == 0)
            printf("a: %u\n", a);
    printf("Final a: %u (which is no longer less than %u\n", a, chunk);
    printf("is %u less than %u? %s\n", a, chunk, a < chunk ? "yes" : "no");
    // Bochs is stopping at a different way-too-soon every time, not the same way-too-soon -- which makes me think that for some reason we're
    //  not coming back from an interrupt...  I thought I was coming back if instruction pointer was in user space, or something close to that...
    //  So let's investigate.

    // Hmm, well, it seems to go back to userspace plenty of times in bochs... So why the heck is it stopping WAY too soon, and at a different
    //   value every time???
}


/*

  I guess we want to set up an entry point in sys, which calls main(), and then calls exit().  Wow, cool!!

  How do I want do have a printf?
  I see two paths:

  1. Varargs to the kernel.
     I think I'll have int 80 for all syscalls, so have have rax be the function number.
     So for a varargs function, rbx can be the number of vararg items on the stack.
       (I'm picturing cpu restored kernel stack thanks to tss, and put my interrupt stack frame, including caller rsp there, so I'd look
         at that frame->sp to get these arguments.)
  2. Have userspace sprintf function, and then have int 80 print function that isn't varargs and doesn't do formatting.

  I see pros and cons to each.

  Minimizing time in the kernel is probably good.
  Varargs to the kernel is likely to be useful for some things, even if we go with route 2 here.
  We're not avoiding kernel call with option 2; we're actually entering kernel more often...
    Well, that's assuming we use kernel malloc to return kernel memory, which we actually don't want a lot of the time...  Stuff to
      think about.  But it'd be nice to have a userspace malloc that returns memory from the process's existing page of memory if available,
      and *it* mallocs another page from the kernel if necessary.  A bit complicated (not compared to some of the stuff I've already done,
      unless I'm missing something -- I think I see pretty clearly how to do this -- but more complicated than I want or need right now), and
      not necessary right now, but probably the right way overall.  Is that what libc does?
  But in any case, userspace sprintf requires userspace malloc -- can't access kernel memory, obviously.

  So I think my simplest path of having fun and moving forward is varargs to the kernel, and go ahead with a kernel printf, even though in the
    long run I'd prefer to have the work of formatting done in userspace.

  And it *is* actually starting to sound kind of fun and interesting to have a userspace runtime library for malloc and sprintf.

  */

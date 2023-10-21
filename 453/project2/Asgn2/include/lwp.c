#include <lwp.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/mman.h>

// define global variables

// making round robin scheduler
// do not need init and shutdown because our structure is just a thread struct
struct scheduler rr_publish = {NULL, NULL, admit, remove, next, qlen};
scheduler RoundRobin = &rr_publish;

// global scheduler
scheduler schedule;
// global threadpool --> make thread pointer, maybe static
thread head = NULL;

// Need to keep track of the return address that we replaced with the address of the function arg
// need to keep track of the scheduler
int tid_counter;
tid_counter = 1;


tid_t lwp_create(lwpfun function, void *argument) {
    /*
    Creates a new thread and admits it to the current scheduler. The thread’s resources will consist of a
    context and stack, both initialized so that when the scheduler chooses this thread and its context is
    loaded via swap_rfiles() it will run the given function. This may be called by any thread.
    */
    long page_size;
    struct rlimit rlp;
    int result;
    rlim_t resource_limit;
    void *s;
    context *c;
    rfile *new_rfile;
    unsigned long *stack_pointer;

    // need to allocate memory for the context struct
    c = malloc(sizeof(context));
    if (c == NULL)
    {
        perror("Error allocating memory for context struct");
        exit(EXIT_FAILURE);
    }
    c->tid = tid_counter++;
    c->status = 0; // not running yet

    // need to allocate memory for the rfile struct
    new_rfile = malloc(sizeof(rfile));
    if (new_rfile == NULL)
    {
        perror("Error allocating memory for rfile struct");
        exit(EXIT_FAILURE);
    }

    // Determine how big the stack should be using sysconf(3)
    page_size = sysconf(_SC_PAGE_SIZE);

    // Get the value of the resource limit for the stack size (RLIMIT_STACK) using getrlimit(2). Use the soft limit
    result = getrlimit(RLIMIT_STACK, &rlp);

    // If RLIMIT_STACK does not exist or if its value is RLIM_INFINITY, use a stack size of 8MB.
    if (result == -1 || rlp.rlim_cur == RLIM_INFINITY)
    {
        resource_limit = 8 * 1024 * 1024;
    }
    else
    {
        resource_limit = rlp.rlim_cur;
        // make sure the resouce limit is a multiple of the page size, otherwise round up to the nearest multiple of the page size
        if (resource_limit % page_size != 0)
        {
            resource_limit = resource_limit + (page_size - (resource_limit % page_size));
        }
    }

    // Allocate a stack the size of our resource limit, MAP_STACK ensures stack is on 16-byte boundary
    // stack pointer will be at a low memory address
    stack_pointer = mmap(NULL, resource_limit, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack_pointer == MAP_FAILED)
    {
        perror("Error allocating memory for stack");
        exit(EXIT_FAILURE);
    }
    if (*stack_pointer % 16 != 0)
    {
        perror("Stack not properly aligned");
        exit(EXIT_FAILURE);
    }

    stack_pointer = stack_pointer + resource_limit; // now our stack pointer is at high memory address
    c->stack = stack_pointer; // Set base of the stack
    c->stacksize = resource_limit;

    // need to push the address of the function wrapper onto the stack
    *stack_pointer = lwp_wrap; // this will push the address of the function wrapper onto the stack
    stack_pointer--;           // this will subtract the size of an unsiged long from the stack pointer

    // need to set all the registers for the new lwp using the function arguments from above
    // put the the arg pointer goes into the register %rdi
    new_rfile->rdi = function;
    new_rfile->rsi = argument;
    new_rfile->rbp = stack_pointer; // should this go before we add the addres to the stack?
    new_rfile->rsp = stack_pointer;
    new_rfile->fxsave = FPU_INIT;
    // Do I need to set the registers to 0?

    // TODO: admit the context to the scheduler 
    
}


static void lwp_wrap(lwpfun fun, void *arg)
{
    /* call the given lwpfucntion with the given argument.
    calls lwp_exit() with its return value*/
    int rval;
    rval = fun(arg);
    lwp_exit(rval);
}

void  lwp_exit(int status) {
    /*Starts the threading system by converting the calling thread—the original system thread—into a LWP
    by allocating a context for it and admitting it to the scheduler, and yields control to whichever thread the
    scheduler indicates. It is not necessary to allocate a stack for this thread since it already has one.*/

}

tid_t lwp_gettid(void) {

}

void  lwp_start(void) {
    /*Starts the threading system by converting the calling thread—the original system thread—into a LWP
    by allocating a context for it and admitting it to the scheduler, and yields control to whichever thread the
    scheduler indicates. It is not necessary to allocate a stack for this thread since it already has one.  */
    
    // TODO: allocate a context for the calling thread

    // TODO: admit the context to the scheduler

    //TODO: VERY LAST THING we do in this function is switch the stack to the first lwp, then when we return
    // we will return to lwp_wrap. To do this switch I will get the next thread from the scheduler.
    // Then I will use swap_rfiles to switch the stack to this thread. All the info about threads will
    // be stored in the scheduler, allowing this process to work.
}

tid_t lwp_wait(int *){
    /*Deallocates the resources of a terminated LWP. If no LWPs have terminated and there still exist
    runnable threads, blocks until one terminates. If status is non-NULL, *status is populated with its
    termination status. Returns the tid of the terminated thread or NO_THREAD if it would block forever
    because there are no more runnable threads that could terminate.*/
}

void  lwp_set_scheduler(scheduler fun) {
    // if fun is null initialize round robin
    if (fun != NULL)
        schedule = fun.init();
    else {
        schedule = RoundRobin;
    }

}

scheduler lwp_get_scheduler(void) {
    return 

}

thread tid2thread(tid_t tid) {

}

// void init(void) {
//     /* initialize any structures     */
// }

// void shutdown(void){        
//     /* tear down any structures      */
//     free(threadPool);
// }

void admit(thread new){
    /* add a thread to the pool      */

    // check if we have empty thread pool
    if (head == NULL) {
        head = new;
    }
    else {
        // loop on qlen
        thread curr_thread = head;
        while (curr_thread->sched_one != NULL) {
            curr_thread = curr_thread->sched_one;
        }
        // set next to new thread
        curr_thread->sched_one = new;
    }

}
void remove(thread victim) {
    /* remove a thread from the pool */

    // check if we have empty thread pool
    if (head != NULL) {
        // loop on qlen for tid
        thread prev_thread = head;
        thread curr_thread = head;
        int found_flag = 0;
        while(curr_thread->sched_one != NULL && !found_flag) {
            if (curr_thread->tid != victim->tid){
                found_flag = 1;
            }
            else {
                prev_thread = curr_thread;
                curr_thread = curr_thread->sched_one;
            }
        }

        if (found_flag) {
            // get the next thread to link prev to next, either sets to the next thread or NULL
            thread next_thread = curr_thread->sched_one;
            // // check the next thread
            // if (curr_thread->sched_one != NULL) {
            //     // set next to new thread
            prev_thread->sched_one = next_thread;
            // }
            // else {
            //     prev_thread->sched_one = NULL;
            // }
        }
        else {
            perror("Error finding thread to remove");
            exit(EXIT_FAILURE);
        }
    }
}

thread next(void) {            
    /* select a thread to schedule   */
    // QUESTION: Maybe the waiting for yielded process and then kick out next here or in wait?
    // waiting for the current thread to yield; may have to approach differently
    while(head->status != LWP_TERM);
    // put current thread at the end of the queue, should put NULL if no other processes in pool?
    thread finished = head;
    head = head->sched_one;
    finished->sched_one = NULL;
    admit(finished);
    // return the next thread
    return head;
}

int qlen(void) {
    /* number of ready threads       */
    // is the readiness of a thread stored in status? loops 
    // through queue and check ready status or return qlen?
    int ready = 0;
    thread curr_thread = head;
    while(curr_thread != NULL && curr_thread->status == LWP_LIVE) {
        ready++;
        curr_thread = curr_thread->sched_one;
    }
    return ready;

}


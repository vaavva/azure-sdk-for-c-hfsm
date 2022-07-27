//#define _POSIX_SOURCE  199309L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Just for this example
#include <string.h>
#include <unistd.h>

// For timer functionality
#include <time.h>
#include <signal.h>

// For Mutex
#include <pthread.h>

// From https://sodocumentation.net/posix/topic/4644/timers

pthread_mutex_t mutex;
void thread_handler(union sigval sv);



void thread_handler(union sigval sv) {
  char *s = sv.sival_ptr;

  if (0 != pthread_mutex_lock(&mutex))
  {
      perror("pthread_mutex_lock failed");
      exit(EXIT_FAILURE);
  }
  
  /* Will print "5 seconds elapsed." */
  puts(s); fflush(stdout);

  if (0 != pthread_mutex_unlock(&mutex))
  {
      perror("pthread_mutex_lock failed");
      exit(EXIT_FAILURE);
  }
}

// HFSM_TODO: Error handling intentionally missing.
int main(int argc, char* argv[])
{
  (void)argc; (void)argv;

  char info[] = "1 second elapsed.";
  timer_t timerid;
  struct sigevent sev;
  struct itimerspec trigger;

  if (0 != pthread_mutex_init(&mutex, NULL))
  {
      perror("pthread_mutex_init() failed");
      return EXIT_FAILURE;
  }

  /* Set all `sev` and `trigger` memory to 0 */
  memset(&sev, 0, sizeof(struct sigevent));
  memset(&trigger, 0, sizeof(struct itimerspec));

  /* 
    * Set the notification method as SIGEV_THREAD:
    *
    * Upon timer expiration, `sigev_notify_function` (thread_handler()),
    * will be invoked as if it were the start function of a new thread.
    *
    */
  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_notify_function = &thread_handler;
  sev.sigev_value.sival_ptr = &info;

  /* Create the timer. In this example, CLOCK_REALTIME is used as the
    * clock, meaning that we're using a system-wide real-time clock for
    * this timer.
    */
  timer_create(CLOCK_REALTIME, &sev, &timerid);

  /* Timer expiration will occur withing 5 seconds after being armed
    * by timer_settime().
    */
  // Single shot:
  trigger.it_value.tv_sec = 1;
  // Periodic:
  trigger.it_interval.tv_nsec = 1000000000 / 2;

  /* Arm the timer. No flags are set and no old_value will be retrieved.
    */
  timer_settime(timerid, 0, &trigger, NULL);

  /* Wait 10 seconds under the main thread. In 5 seconds (when the
    * timer expires), a message will be printed to the standard output
    * by the newly created notification thread.
    */
  for (int i = 0; i < 10; i++)
  {
    if (0 != pthread_mutex_lock(&mutex))
    {
        perror("pthread_mutex_lock failed");
        exit(EXIT_FAILURE);
    }
    printf("main(): Waiting\n"); fflush(stdout);
    if (0 != pthread_mutex_unlock(&mutex))
    {
        perror("pthread_mutex_lock failed");
        exit(EXIT_FAILURE);
    }
    sleep(1);
  }

  /* Delete (destroy) the timer */
  timer_delete(timerid);
  if (0 != pthread_mutex_destroy(&mutex))
  {
      perror("pthread_mutex_destroy() failed");
      return EXIT_FAILURE;
  }

  return 0;
}

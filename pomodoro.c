#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define IGNORE(x) (void)(x)

void (*signal(int sig, void (*func)(int)))(int);

/*
 * Should I start a new pomodoro?
 */

int startPomodoro = 0;

/*
 * Am I waiting for start of a new pomodoro?
 */

int busyWaiting = 1;

/*
 * Number of pomodoros run previously (in this round).
 */

int counter;

/*
 * goto beginning of fresh pomodoro without incrementing counter
 */

void restartPomodoro(int);

/*
 * do nothing
 */

void startBusyWaiting(int);

/*
 * current time in seconds — overflow check?
 */

time_t now(void);

int main(void) {
    /*
     * a pomodoro has WORK seconds of work time, followed by
     * SLEEP seconds of sleep time; this is substituted after
     * MAX_POMODOROS by LONG_SLEEP seconds of sleep times and
     * the cycle repeats
     */

    const int WORK = 25*60,
          SLEEP = 5*60,
          LONG_SLEEP = 25*60,
          MAX_POMODOROS = 4;

    struct sigaction act;

    /*
     * react to the signals USR1 and USR2 by restarting the
     * pomodoro (if it's finished, start a new one) and stopping
     * all actions, respectively
     */

    act.sa_flags = 0;
    act.sa_handler = restartPomodoro;
    sigaction(SIGUSR1, &act, NULL);

    act.sa_handler = startBusyWaiting;
    sigaction(SIGUSR2, &act, NULL);

busyWaiting:
    while (busyWaiting) {
        printf("P -\n");
        fflush(stdout);
        sleep(1);
    }

    counter = 0;
    while (1) {
startPomodoro:
        if (counter == MAX_POMODOROS)
            counter = 0;

        counter++;

        /*
         * doesn't do anything (sensible) if this hook file
         * doesn't exist
         */

        system("~/.pymodoro/hooks/start-pomodoro.py");
        startPomodoro = 0;
        busyWaiting = 0;

        /*
         * work cycle
         */

        time_t t1 = now(),
               delta;
        while ((delta = now() - t1) <= WORK) {
            long long int seconds, minutes;
            delta = WORK - delta;
            seconds = delta%60;
            minutes = (delta-seconds)/60;

            sleep(1);

            /*
             * color codes as required by xmonad
             */

            printf("<fc=#ffff00>P[%u]: %02lld:%02lld</fc>\n", counter, minutes, seconds);
            fflush(stdout);

            if (startPomodoro) goto startPomodoro;
            if (busyWaiting) goto busyWaiting;
        }

        system("~/.pymodoro/hooks/complete-pomodoro.py");

        /*
         * sleep cycle
         */
        t1 = now();
        while (1) {
            long long int seconds, minutes;
            delta = now() - t1;
            if (counter == MAX_POMODOROS)
                delta = LONG_SLEEP - delta;
            else
                delta = SLEEP - delta;

            seconds = llabs(delta)%60;
            minutes = llabs(delta)/60;

            sleep(1);

            if (delta < 0)
                printf("<fc=#FF0000>!!B: %02lld:%02lld</fc>\n", minutes, seconds);
            else
                printf("<fc=#0000FF>B: %02lld:%02lld</fc>\n", minutes, seconds);

            fflush(stdout);

            if (startPomodoro) goto startPomodoro;
            if (busyWaiting) goto busyWaiting;
        }
    }

    return EXIT_SUCCESS;
}

void restartPomodoro(int sig) {
    IGNORE(sig);
    startPomodoro = 1;
    busyWaiting = 0;
}

void startBusyWaiting(int sig)  {
    IGNORE(sig);
    busyWaiting = 1;
}

time_t now(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL)) {
        perror("gettimeofday (in now)");
        exit(EXIT_FAILURE);
    }

    return tv.tv_sec;
}

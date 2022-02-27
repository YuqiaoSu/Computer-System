/*************************************************************************
 *  Copyright 2020 Gene Cooperman                                        *
 *                                                                       *
 *  This file is part of McMini.                                         *
 *                                                                       *
 *  McMini is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by *
 *  the Free Software Foundation, either version 3 of the License, or    *
 *  (at your option) any later version.                                  *
 *                                                                       *
 *  McMini is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *  GNU General Public License for more details.                         *
 *                                                                       *
 *  You should have received a copy of the GNU General Public License    *
 *  along with McMini.  If not, see <https://www.gnu.org/licenses/>.     *
 *************************************************************************/

/******************** Non-deterministic thread ordering  **********************/
/* The goal of this version is to demonstrate the simplest deadlock bug       */
/* using two threads (often used in introducing this in teaching).            */
/******************************************************************************/

// FIXME:  Need to test for liveness:
//   https://simgrid.org/doc/latest/Configuring_SimGrid.html#specifying-a-liveness-property

#include <pthread.h>
#include "mcmini.h"

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

static void* thread2_start(void * dummy) {
  while (1) {
int x = mc_choose(5);
    pthread_mutex_lock(&mutex2);
    pthread_mutex_lock(&mutex1);
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
  }

  return NULL;
}

int r=0;
int cs=0;

int main(int argc, char* argv[])
{
  pthread_t thread2;
  pthread_create(&thread2, NULL, &thread2_start, NULL);

  while (1) {
    r=1;
    cs=0;
    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);
    cs=1;
    // Is there a way to create an extra state here,
    // and then set cs=0 when we exit this critical section state?
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    cs=0;
    r=0;
  }

  return 0;
}

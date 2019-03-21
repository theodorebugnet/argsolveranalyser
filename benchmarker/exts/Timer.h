/*
 * timer.h
 *
 *  Created on: 11 May 2018
 *      Author: odinaldo
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <ctime>

using namespace std;

class
Timer {
private:
  clock_t _start_time;
  clock_t _accummulated;
  bool _paused;

public:
  Timer ();
  void reset ();
  void resume();
  void restart();
  void start();
  void pause();
  int elapsed () const;
  float milli_elapsed () const;
};

#endif /* TIMER_H_ */

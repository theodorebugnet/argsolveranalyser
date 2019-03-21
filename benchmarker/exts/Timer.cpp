/*
 * timer.cpp
 *
 *  Created on: 11 May 2018
 *      Author: odinaldo
 */

#include "Timer.h"

Timer::Timer () {
  _accummulated=0;
  _paused=true;
  _start_time=0;
}
void Timer::reset () {
  _accummulated=0;
  _paused=true;
}

void Timer::start() {
  if (_paused) {
    _start_time=clock();
    _paused=false;
  };
}
void Timer::restart () {
  _accummulated=0;
  _start_time = clock();
  _paused=false;
}

void Timer::resume() {
  if (_paused) {
  _start_time = clock();
  _paused=false;
  }
}
void Timer::pause() {
  if (!_paused) {
    _accummulated=_accummulated+(clock()-_start_time);
    _paused=true;
  };
}

int Timer::elapsed () const {
  if (_paused) {
    return int (_accummulated);
  } else {
   return int (_accummulated+(clock()-_start_time)) / CLOCKS_PER_SEC;
  }
}

float Timer::milli_elapsed () const {
  if (_paused) {
    return float (_accummulated * 1000) / CLOCKS_PER_SEC;
  } else {
    return float (_accummulated+(clock()-_start_time))*1000 / CLOCKS_PER_SEC;
  }
}



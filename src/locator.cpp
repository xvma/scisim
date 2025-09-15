#include "supervisor.h"
#include "winder.h"
#include "locator.h"

const int timerResolution = 70;     // default tick latency for moving timers
//_________________________________________________________
//
// Object constructor. Set common styles and resize the control
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Locator::Locator(QWidget *parent /*=0*/) :
  QFrame(parent)
{
  setLineWidth(1);
  setFrameStyle(NoFrame | Plain);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  // initial values
  extraWidth = 0;

  m_timeLeft = 0;
  m_movement_timer = 0;
  m_movingStatus = NONE;
  m_middleX = 0;
  m_curSpeed = 0;
  m_emitReachEvent = true;
}
//_________________________________________________________

Locator::~Locator()
{
}
//_________________________________________________________
//
// Event handler for timer counters
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Locator::timerEvent(QTimerEvent* te)
{
  // count params for movement process to move the widget to new pos
  int delta = 0;
  int prevDelta = 0;
  if (te->timerId() == m_movement_timer)
  {
    int prevTimeLeft = m_timeLeft;
    // decreasing time left until 0
    m_timeLeft = m_timeLeft <= timerResolution ? 0 : (m_timeLeft - timerResolution);
    // count distance according to the current moving state
    switch(m_movingStatus)
    {
      case STARTING:
        delta = ((m_timeReach - m_timeLeft) * (m_timeReach - m_timeLeft) * m_accel / 1000000) >> 1;
        prevDelta = ((m_timeReach - prevTimeLeft) * (m_timeReach - prevTimeLeft) * m_accel / 1000000) >> 1;
        m_curSpeed = m_accel * (m_timeReach - m_timeLeft) / 1000;
        break;
      case MOVING:
        delta = (m_timeReach - m_timeLeft) * m_speed / 1000;
        prevDelta = (m_timeReach - prevTimeLeft) * m_speed / 1000;
        break;
      case BRAKING:
        delta = abs(m_destX - m_middleX) - ((m_timeLeft * m_timeLeft * m_accel / 1000000) >> 1);
        prevDelta = abs(m_destX - m_middleX) - ((prevTimeLeft * prevTimeLeft * m_accel / 1000000) >> 1);
        m_curSpeed -= m_accel * (m_timeReach - m_timeLeft) / 1000;
        if (m_curSpeed < 0) m_curSpeed = 0;
        break;
      case NONE:
      default:
        break;
    }
    // delta - new distance from middleX
    QPoint newPos(m_destX > m_startX ? m_middleX + delta : m_middleX - delta, y());
    // moving widget
    move(newPos);
    // notify supervisor
    if (delta != prevDelta && m_emitReachEvent)
      emit movement(m_id, newPos, m_destX >= m_startX ? (delta - prevDelta) : (prevDelta - delta));
    //if timer expired
    if (m_timeLeft == 0)
    {
      //kill timer and switch machine onto the next movement phase
      killTimer(m_movement_timer);
      m_movement_timer = 0;
      switch(m_movingStatus)
      {
        case STARTING:
          m_movingStatus = MOVING;
          startMoving();
          break;
        case MOVING:
          m_movingStatus = BRAKING;
          startMoving();
          break;
        case BRAKING:
          // after the braking phase the goal has been reached
          m_movingStatus = NONE;
          //update logger object
          updateLoggerItem(m_id, Logger::TIME_MOVE);
          // notify supervisor if necessary
          if (m_emitReachEvent)
            emit goalReached(m_session);
          break;
        case NONE:
          m_curSpeed = 0;
          break;
        default:
          break;
      }
    }
  }
}
//_________________________________________________________
//
// Stop locator movement. Method kills movement timer and
// immediately runs the pull up phase
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Locator::stopMoving(bool doEmit /*= true*/)
{
  // if locator is not moving or pulling up then quit
  m_emitReachEvent = doEmit;
  if (m_movement_timer == 0 || m_movingStatus == BRAKING) return;
  // kill movement timer
  killTimer(m_movement_timer);
  m_movement_timer = 0;

  // count brake distance
  int brakeDelta = getBrakeDistance();
  m_destX = m_destX > m_startX ? x() + brakeDelta : x() - brakeDelta;

  // set widget x-pos as the starting point
  m_startX = x();
  m_middleX = x();

  // run new timer for braking phase using calculated brake distance
  m_movingStatus = BRAKING;
  m_timeLeft = round(1000 * sqrt((brakeDelta << 1) / (float)m_accel));
  m_timeReach = m_timeLeft;
  m_emitReachEvent = doEmit;
  m_movement_timer = startTimer(timerResolution);
}
//_________________________________________________________
//
// Calculate brake distance based on the current speed
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Locator::getBrakeDistance()
{
  // check if the object is moving
  if (m_accel == 0 || !isMoving()) return 0;
  return m_curSpeed * m_curSpeed / (m_accel << 1);
}
//_________________________________________________________
//
// Protected movement state machine method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Locator::startMoving()
{
  // set current x-pos as the phase starting position
  m_middleX = x();
  // count the whole distance to move
  int distance = abs(m_destX - m_startX);

  // calculate necessary distance and time left
  switch(m_movingStatus)
  {
    case STARTING:  // state for the movement beginning
      distance = (distance > (extraWidth << 1)) ? extraWidth : (distance >> 1);
      m_timeLeft = round(1000 * sqrt((distance << 1) / (float)m_accel));
      break;
    case MOVING: // state for the constant speed movement
      distance = (distance > (extraWidth << 1)) ? (distance - (extraWidth << 1)) : 0;
      m_timeLeft = 1000 * distance / m_speed;
      if (m_timeLeft > 0)
        break;
      // if constant speed moving phase is absent switch onto the braking phase
      m_movingStatus = BRAKING;
      distance = abs(m_destX - m_startX);
    case BRAKING: // state for the movement completion
      distance = (distance > (extraWidth << 1)) ? extraWidth : (distance >> 1);
      m_timeLeft = round(1000 * sqrt((distance << 1) / (float)m_accel));
      break;
    case NONE:
    default:
      break;
  }
  // wind the specific movement timer
  m_timeReach = m_timeLeft;
  m_movement_timer = startTimer(timerResolution);
}
//_________________________________________________________
//
// Public locator movement method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Locator::reachObject(QString idSession, int x, int y, bool doEmit /* = true*/)
{
  // Check if the speed exists
  if (m_speed == 0) return;

  // Check if we're already moving to this destination
  if (isMoving() && x == m_destX)
  {
    // assign session and emit event flag
    setDestPos(x, y);
    m_session = idSession;
    m_emitReachEvent = doEmit;
    return;
  }

  // Check if locator is already there, no need to reach
  if (!isMoving() && pos().x() == x)
  {
    // just set params and inform supervisor if necessary
    setDestPos(x, y);
    m_curSpeed = 0;
    m_session = idSession;
    m_emitReachEvent = doEmit;
    if (doEmit)
      goalReached(m_session);
    return;
  }

  // return if it's moving now
  if (isMoving()) return;

  // update idle counter for the object
  updateLoggerItem(m_id, Logger::TIME_IDLE);
  // set widget x-pos as the starting point
  m_startX = pos().x();
  m_curSpeed = 0;
  // init supervisor task session
  m_session = idSession;
  // set destination
  setDestPos(x, y);
  // launch the state machine
  m_movingStatus = STARTING;
  m_emitReachEvent = doEmit;
  startMoving();
}
//_________________________________________________________
//
// Update destination position
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Locator::setDestPos(int x, int y)
{
  m_destX = x;
  m_destY = y;
}
//_________________________________________________________
//
// Update bobbins size to use with animated offsprings
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Locator::setBobbinsSize(QSize srcSize)
{
  m_bobbinsSize.setWidth(srcSize.width());
  m_bobbinsSize.setHeight(srcSize.height());
}




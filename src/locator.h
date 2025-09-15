#ifndef LOCATOR_H
#define LOCATOR_H

#include <QFrame>
#include <QtGui>
#include "anim.h"
#include "logger.h"
//_________________________________________________________
//
// Class represents locator widget which is possible to move
// and brake with acceleration
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class Locator : public QFrame
{
  Q_OBJECT
public:
  // Locator movement states
  enum Movement
  {
    NONE = 0,   // Locator is not moving
    STARTING,   // Locator is speeding up
    MOVING,     // Locator is moving with constant speed
    BRAKING     // Locator is pulling up
  };
  explicit Locator(QWidget *parent = 0);
  virtual ~Locator();

  QString getId() {return m_id;}
  QString getSession() {return m_session;}
  bool isMoving() {return m_movement_timer > 0;}
  int getDistance() {return extraWidth;}
  int getDestX() {return m_destX;}
  int getDestY() {return m_destY;}
  int getCurrentSpeed() {return m_curSpeed;}
  Movement getMovingState() {return m_movingStatus;}

  void setDestPos(int x, int y);
  virtual void reachObject(QString idSession, int x, int y, bool doEmit=true);
  void stopMoving(bool doEmit = true);
  void setBobbinsSize(QSize srcSize);
  int getBrakeDistance();

signals:
  void goalReached(QString idSession);
  void movement(QString, QPoint, int);
  void updateLoggerItem(QString idObject, Logger::FieldNames field);

public slots:

protected:
  virtual void timerEvent(QTimerEvent *);
  void startMoving();

  int m_speed;          // max constant locator speed
  int m_accel;          // acceleration value
  QString m_id;         // object id

  QString m_session;    // supervisor task session id
  int m_timeLeft;       // time left counter
  int m_timeReach;      // reaching time value
  int m_startX;         // starting point x position
  int m_destX;          // destination x position
  int m_destY;          // destination y position

  int extraWidth;       // max brake distance value

  int m_movement_timer;       // movement timer id
  Movement m_movingStatus;    // current movement state
  int m_middleX;              // current phase starting point (on starting, on movement or on braking)
  int m_curSpeed;             // current speed
  bool m_emitReachEvent;      // true if necessary to notify supervisor about object moving and arriving

  QSize m_bobbinsSize;        // counted sizes using for drawing bobbins / sleeve
};

#endif

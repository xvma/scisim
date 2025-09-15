#ifndef MAN_H
#define MAN_H

#include <QFrame>
#include <QtGui>
#include "invdatabase.h"
//_________________________________________________________
//
// Class represents man service widget which is possible to move
// with contsant speed
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ManService : public QFrame
{
  Q_OBJECT
public:
  // Man states
  enum Status
  {
    IDLE = 0,           // man has not got any task.
    READY,              // man is reaching or has reached a destination
    BUSY                // man is in process of solving task
  };
  // Machine actions
  enum OperFunc
  {
    REACH = 0,          // move to the winder, spooler, sleever
    START_WINDER,       // start winder task in progress
    ROTATE_SPOOLER,     // rotate spooler task in progress
    CHANGE_SPOOLER,     // change spooler task in progress
    LOAD_SLEEVER,       // reload sleever task in progress
    CUT_EDGE            // cut bobbin edge task in progress
  };
  explicit ManService(ManServiceModel &model, QWidget *parent = 0);
  virtual ~ManService();

  QString getId() {return m_id;}
  Status getStatus() {return m_status;}
  bool isMoving() {return m_movement_timer>0;}
  void setStatus(Status state);

  void setDestPos(int x, int y);
  void reachObject(QString idSession, int x, int y);
  void loadSleever(QString idSession);
  void changeSpooler(QString idSession);
  void rotateSpooler(QString idSession);
  void startWinder(QString idSession);
  void cutEdgeOnWinder(QString idSession);
  void stopMoving(bool doEmit = true);

signals:
  void goalReached(QString);
  void taskCompleted(QString);

public slots:

protected:
  virtual void timerEvent(QTimerEvent *);
  virtual void paintEvent(QPaintEvent *);

private:
  void startMachine(OperFunc operation);

  Status m_status;            // current man service status
  int m_speed;                // constant man service speed
  int m_timeStartWinder;      // start winder time value
  int m_timeRotateSpooler;    // spooler rotate time value
  int m_timeChangeSpooler;    // spooler change time value
  int m_timeLoadSleever;      // sleever reload time value
  int m_timeCutEdge;          // cut edge time value

  QString m_id;               // object id
  QString m_session;          // supervisor task session id
  int m_timeLeft;             // time left counter
  int m_timeReach;            // reaching time value
  int m_startX;               // starting point x position
  int m_destX;                // destination x position
  int m_destY;                // destination y position

  int m_movement_timer;       // movement timer id
  int m_startWinder_timer;    // start winder timer id
  int m_cutEdge_timer;        // cutedge timer id
  int m_rotateSpooler_timer;  // rotate spooler timer id
  int m_changeSpooler_timer;  // change spooler timer id
  int m_loadSleever_timer;    // load sleever timer id
};

#endif

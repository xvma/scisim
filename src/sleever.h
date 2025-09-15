#ifndef SLEEVER_H
#define SLEEVER_H

#include <QFrame>
#include <QtGui>
#include "anim.h"
#include "locator.h"
#include "invdatabase.h"
//_________________________________________________________
//
// Class represents sleever widget.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class Sleever : public Locator
{
  Q_OBJECT
public:
  // Sleever states
  enum Status
  {
    IDLE = 0,         // sleever has not got any task. Can be moving to spooler having this state
    BUSY,             // sleever is in process of putting sleeve to winder
    READY,            // sleever is reaching or has reached the winder
    EMPTY,            // sleever is out of sleeves and is reaching or has reached the service zone
    PREPARING,        // sleever is preparing new sleeve
    WAIT              // sleever is waiting until doffer stop moving.
  };
  // Machine actions
  enum OperFunc
  {
    REACH = 0,        // move to the winder
    PUTRES,           // put sleeve
    PREPARE           // prepare the new one
  };
  explicit Sleever(SleeverModel &model, QWidget *parent = 0);
  virtual ~Sleever();

  Status getStatus() {return m_status;}
  int getSleeves() {return m_sleeves;}
  int getRings() {return m_rings;}
  bool isEmpty() {return (m_status == EMPTY || m_sleeves == 0);}

  void setStatus(Status state);
  void setInventory(int sleeves, int rings);
  void putResult(QString idSession, int sleeves, int rings);
  virtual void reachObject(QString idSession, int x, int y, bool doEmit = true);

signals:
  void taskCompleted(QString);
  void emptySleever(QString idSleever);

public slots:

protected:
  virtual void timerEvent(QTimerEvent *);
  virtual void paintEvent(QPaintEvent *);

private:
  void startMachine(OperFunc operation);
  void createAnimator(Animator::Type);
  void destroyAnimator();

  Status m_status;              // current sleever status
  int m_timePutDown;            // time setting for putting sleeve process
  int m_timePrepare;            // time setting for preparing new sleeve
  int m_sleeves;                // amount of sleeves onboard
  int m_rings;                  // amount of rings onboard

  Animator *m_anim;             // animator reference

  int m_putres_timer;           // timer id for putting sleeve process
  int m_prepare_timer;          // timer id for prepare sleeve process
};

#endif

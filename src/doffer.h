#ifndef DOFFER_H
#define DOFFER_H

#include <QFrame>
#include <QtGui>
#include "anim.h"
#include "locator.h"
#include "invdatabase.h"
//_________________________________________________________
//
// Class represents doffer widget.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class Doffer : public Locator
{
  Q_OBJECT
public:
  // Doffer states
  enum Status
  {
    IDLE = 0,     // doffer has not got any task. Can be moving to winder having this state
    DELIVER,      // doffer has bobbings onboard. Can be moving to spooler having this state
    READY,        // doffer is reaching or has reached the winder
    BUSY,         // doffer is in process of getting bobbing from winder or putting one bobbing to the spooler
    WAIT,         // doffer is waiting until sleever arrives. Usually it's takes place when both doffer & sleever are sent to winder
    WAITWINDER    // doffer has arrived and is waiting for winder ready state
  };
  // Machine actions
  enum OperFunc
  {
    GETRES = 0,   // get bobbins onboard
    REACH,        // move to the winder/spooler
    PUTRES        // put bobbins
  };

  explicit Doffer(DofferModel &model, QWidget *parent = 0);
  virtual ~Doffer();

  Status getStatus() {return m_status;}
  int getAmount() {return m_amount;}
  int getControlHeight() {return controlHeight;}

  void setStatus(Status state);
  void getResult(QString idSession, int amount);
  void resetAmount(){m_amount = 0;}
  void putResult(QString idSession, int row, int column, int amount);
  virtual void reachObject(QString idSession, int x, int y, bool doEmit=true);

signals:
  void bobAboard(QString idSession);
  void bobPlaced(QString idSession);
  void taskCompleted(QString);

public slots:

protected:
  virtual void timerEvent(QTimerEvent *);
  virtual void paintEvent(QPaintEvent *);

private:
  void startMachine(OperFunc operation);
  void createAnimator(Animator::Type, int yPos);
  void destroyAnimator();

  Status m_status;      // current doffer status
  int m_timeGetIn;      // time setting for getting bobbins process
  int m_timePutDown;    // time setting for putting bobbins process
  int m_amount;         // amount of bobbings onboard

  Animator *m_anim;     // animator reference
  int controlHeight;    // drawing control height (real height includes bobbins as well)

  int m_getres_timer;   // timer id for getting bobbins process
  int m_putres_timer;   // timer id for putting bobbins process
};

#endif

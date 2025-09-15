#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include <QMdiArea>

#include "logger.h"
#include "invdatabase.h"
#include "winder.h"
#include "doffer.h"
#include "sleever.h"
#include "spooler.h"
#include "man.h"
//_________________________________________________________
//
// Class represents supervisor widget. It manages task queue which contains task session records.
// Every record refers to necessary objects and contain specific information. Also supervisor manages
// database models and their connections with child widgets objects. Communication is done with help of
// signals and slots.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class Supervisor : public QFrame
{
  Q_OBJECT
public:
  // Task states
  enum TaskStatus
  {
    NEW = 0,        // New task session is created by supervisor on request from child object
    PROGRESS,       // The task session is in progress now
    PAUSED,         // Task session is waiting for the handler or some conditions to switch to PROGRESS
    CANCELLED,      // Task session was cancelled
    DONE            // Task session done. Both cancelled and done tasks are deleted from queue
  };
  // Available task types
  enum TaskType
  {
    START_WINDER = 0,         // Man task: reach the winder and start it
    ROTATE_SPOOLER,           // Man task: reach the spooler and rotate it
    CHANGE_SPOOLER,           // Man task: reach the spooler and change it
    LOAD_SLEEVER,             // Man task: reach the sleever in service zone and reload it
    DELIVER_BOBBINS,          // Doffer task: reach the winder, take bobbins and deliver them to spooler
    DELIVER_SLEEVE,           // Sleever task: reach the winder and put the sleeve on it
    MOVE_SLEEVER,             // Sleever task: reach the service zone
    MOVE_DOFFER_SLEEVER,      // Doffer task: reach the winder and wait for sleever
    HANDLE_COLLISION,         // Supervisor task: wait until both objects doffer & sleever stop, then link them
    CUTEDGE_WINDER            // Man task: reach the winder and cut bobbins
  };
  // Supervisor can choose available man using the following strategies
  enum ManStrategy
  {
    FREE_OR_LEASTBUSY = 0,    // Man task will be assigned to the least busy one
    FREE_OR_NEAREST,          // Man task will be assigned to the nearest one
    NEAREST_OR_LEASTBUSY      // Man task will be assigned to the nearest person, if not found then to the least busy one
  };
  // Task Session
  struct SpoolerReservation
  {
    QString idSpooler;
    int row;
    int column;
  };
  struct TaskSession
  {
    QString idSession;                    // Task session guid
    TaskStatus status;                    // Status
    TaskType type;                        // Type
    QString idAssignee;                   // Assignee id (i.e. Doffer Id)
    QString idObject;                     // Object id (i.e. Winder Id)
    int places;                           // Objects amount to deliver or set
    QVector<SpoolerReservation> reserve;  // Array of spooler reservations
    QVector<QString> linkedObjects;       // Array of linked objects which will move together with AssignedId object
    QPoint destPoint;                     // Destination point of linked object
    bool waitDoffer;                      // true if necessary to wait until doffer stop
    bool waitSleever;                     // true if necessary to wait until sleever stop
    QString getId() {return idSession;}
  };


  explicit Supervisor(QWidget *parent = 0);
  virtual ~Supervisor();

  ConfigModel &getConfigModel() {return m_config;}
  void start();
  void stop();
  bool startWinders();
  int toPixels(int sourceValue);
  int toMillimeters(int sourceValue);
  void SetWholeWidthPixels(int width);

  // Return the object pointer with id
  template<class T> static T* getItemById(QString id, QList<T*> &list)
  {
    foreach(T *it, list)
    {
      if (it->getId() == id)
        return it;
    }
    return NULL;
  }

signals:
  void appendLoggerItem(QString idObject);
  void updateLoggerItem(QString idObject, Logger::FieldNames field);

public slots:
  void manReached(QString idSession);
  void dofferArrived(QString idSession);
  void sleeverArrived(QString idSession);
  void taskCompleted(QString idSession);
  void bobbinsAboard(QString idSession);
  void bobbinPlaced(QString idSession);
  void bobbinsReady(QString idWinder);
  void bobbinsCutNeeded(QString idWinder);
  void winderFailed(QString idWinder);
  void winderAlert(QString idWinder);
  void sleeverEmpty(QString idSleever);
  void spoolerFilled(QString idSpooler);
  void dofferMoved(QString idDoffer, QPoint newPos, int delta);
  void sleeverMoved(QString idSleever, QPoint newPos, int delta);
  void updateLogger(QString idObject, Logger::FieldNames field);

protected:
  virtual void timerEvent(QTimerEvent *);

private:
  void initContainers(int x, int y);
  void modelClear();
  void seed();
  void sync();
  void callSleever(QString idWinder);

  ManService *getFreeMan();
  ManService *getLeastBusyMan();
  ManService *getNearestMan(int xPos);
  ManService *getManByStrategy(int xPos = 0, ManStrategy ms = NEAREST_OR_LEASTBUSY);
  void startMachine(TaskSession *ts);
  void runManServiceTask(TaskSession *ts);
  bool tryReserveSpooler(QString idSpooler, SpoolerReservation &spres);
  void cancelSpoolerReservation(QVector<SpoolerReservation> &resarray);
  void runDofferingTask(TaskSession *ts);
  void runSleeverTask(TaskSession *ts);
  void runHandleCollisionTask(TaskSession *ts);
  bool createLoadSleeverTask(Sleever *sleever);
  bool testObjectId(TaskSession *ts, QString idObject);
  bool testObjectId(QString idObject);
  void addObjectId(TaskSession *ts, QString idObject);
  void activateLinkedObjects(TaskSession *ts, bool isDofferLinked);
  void processCollision(Doffer *doffer, Sleever *sleever, bool isDofferLead, int delta);
  void handleCollision(Doffer *doffer, Sleever *sleever, bool isDofferPriority, bool waitDoffer, bool waitSleever);
  void moveDofferAndSleever(QString idDofferSession, QString idWinder, QPoint dest, bool allowReadyDoffer);
  void cancelTask(TaskSession *ts);
  void moveSpoolerToTail(QString idSpooler);
  void countAspectRatio(int space);
  int countNewSleeverX(TaskSession *ts, Doffer *doffer, Sleever *sleever, int destX);
  void setLocatorRectanges(Locator *priObject, Locator *secObject, QRect &primaryRect, QRect &secondaryRect);
  void setGroupBobbinsReady(QString idWinder);

  // Object models
  QList<WinderModel *> m_windersModel;      // database models
  QList<Winder *> m_winders;                // child objects
  QList<QFrame *> m_services;               // service zones

  QList<DofferModel *> m_doffersModel;      // database models
  QList<Doffer *> m_doffers;                // child objects

  QList<SleeverModel *> m_sleeversModel;    // database models
  QList<Sleever *> m_sleevers;              // child objects

  QList<SpoolerModel *> m_spoolersModel;    // database models
  QList<Spooler *> m_spoolers;              // child objects

  QList<ManServiceModel *> m_menModel;      // database models
  QList<ManService *> m_men;                // child objects

  ConfigModel m_config;                     // database model

  QList<TaskSession *> m_tasks;             // Task session queue
  int m_task_timer;                         // Task scan timer id
  int m_db_timer;                           // DB Sync timer id
  int m_wholeWidthPixels;                   // Calculated value of the supervisor width
  float m_aspectRatio;                      // Calculated aspect ratio to convert mm to pixels

  int m_margin;                             // doffer & sleever constant margin

  // Generic methods
  // Return max height for the child object from the list
  template<class T> int getMaxHeight(QList<T*> &list)
  {
    int max = 0;
    foreach(T *it, list)
    {
      if (it->height() > max)
        max = it->height();
    }
    return max;
  }


};

#endif

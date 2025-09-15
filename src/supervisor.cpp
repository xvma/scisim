#include <QUuid>
#include <QDebug>
#include "supervisor.h"

const int timerResolution = 100;    // default time latency for scan task timer
const int dbSyncResolution = 1000;  // default time latency for db update action
const int margin = 80; // buffer zone in mm for the doffer & sleever
//_________________________________________________________
//
// Object constructor. Set default values for parameters
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Supervisor::Supervisor(QWidget *parent /*=0*/): QFrame(parent)
{
  setFrameStyle(NoFrame | Plain);
  // init timers ids
  m_task_timer = 0;
  m_db_timer = 0;
  m_wholeWidthPixels = 0;

  m_aspectRatio = 0.0;
  m_margin = 5;
}
//_________________________________________________________
//
// Object destructor. Destroy child objects, models etc.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Supervisor::~Supervisor()
{
  stop();
}
//_________________________________________________________
//
// Seeding method. Get all models from database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::seed()
{
  modelClear();
  /*InventoryDatabase::seedWindersView(m_windersModel);
  InventoryDatabase::seedDoffersView(m_doffersModel);
  InventoryDatabase::seedSleeversView(m_sleeversModel);
  InventoryDatabase::seedSpoolersView(m_spoolersModel);
  InventoryDatabase::seedMenView(m_menModel);
  InventoryDatabase::seedConfigView(m_config);*/

  QSqlDatabase db = InventoryDatabase::open();
  InventoryDatabase::getConfigView(db, m_config);
  InventoryDatabase::getWindersView(db, m_windersModel, m_config.timeCoefficient);
  InventoryDatabase::getDoffersView(db, m_doffersModel, m_config.timeCoefficient);
  InventoryDatabase::getSleeversView(db, m_sleeversModel, m_config.timeCoefficient);
  InventoryDatabase::getSpoolersView(db, m_spoolersModel);
  InventoryDatabase::getMenView(db, m_menModel, m_config.timeCoefficient);
  InventoryDatabase::close(db);
}
//_________________________________________________________
//
// Update doffers and sleevers data
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::sync()
{
  DofferSyncModel dsm;
  SleeverSyncModel ssm;
  QList<DofferSyncModel> doffers;
  QList<SleeverSyncModel> sleevers;

  // create doffer update models list
  foreach (Doffer *doffer, m_doffers)
  {
    TaskSession *ts = NULL;
    bool found = false;
    // find a progress task for current doffer
    foreach (ts, m_tasks)
    {
      if (ts->status == PROGRESS &&
          ts->idAssignee == doffer->getId() &&
          (ts->type == DELIVER_BOBBINS || ts->type == MOVE_DOFFER_SLEEVER))
      {
        found = true;
        break;
      }
    }

    // init update model
    dsm.idDoffer = doffer->getId();
    dsm.xPos = toMillimeters(doffer->x());
    dsm.curSpeed = toMillimeters(doffer->getCurrentSpeed());
    dsm.status = doffer->getStatus();

    // init data from task
    dsm.idWinder = "";
    dsm.idSpooler = "";
    dsm.row = 0;
    dsm.column = 0;
    if (found)
    {
      dsm.idWinder = ts->idObject;
      if (ts->reserve.size() > 0)
      {
        dsm.idSpooler = ts->reserve[0].idSpooler;
        dsm.row = ts->reserve[0].row;
        dsm.column = ts->reserve[0].column;
      }
    }

    doffers.append(dsm);
  }
  // create sleever update models list
  foreach (Sleever *sleever, m_sleevers)
  {
    TaskSession *ts = NULL;
    bool found = false;
    // find a progress task for current sleever
    foreach (ts, m_tasks)
    {
      if (ts->status == PROGRESS &&
          ts->idAssignee == sleever->getId() &&
          (ts->type == DELIVER_SLEEVE || ts->type == MOVE_SLEEVER))
      {
        found = true;
        break;
      }
    }

    // init update model
    ssm.idSleever = sleever->getId();
    ssm.xPos = toMillimeters(sleever->x());
    ssm.curSpeed = toMillimeters(sleever->getCurrentSpeed());
    ssm.status = sleever->getStatus();
    ssm.idWinder = "";
    ssm.amountSleeves = sleever->getSleeves();
    ssm.amountRings = sleever->getRings();

    // init data from task
    if (found)
      ssm.idWinder = ts->idObject;

    sleevers.append(ssm);
  }

  QSqlDatabase db = InventoryDatabase::open();              // open database
  db.transaction();                                         // start transaction
  if (InventoryDatabase::updateDoffers(db, doffers) &&      // update models
      InventoryDatabase::updateSleevers(db, sleevers))
    db.commit();                                            // commit if success
  else
    db.rollback();                                          // rollback if failed
  InventoryDatabase::close(db);                             // close database
}

//_________________________________________________________
//
// Create child containers from xPos, yPos
// Models should be ready
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::initContainers(int xPos, int yPos)
{
  int x = xPos;
  int y = yPos;
  int space = toPixels(m_config.spaceBetweenWinders);           //calculate space in pixels between winders
  int serviceZoneWidth = toPixels(m_config.serviceZoneWidth);   //calculate service zone width in pixels

  // Create winders container
  int counter = 0;
  QString prevKey = "";
  QString currentKey = "";

  // assume that winder models are sorted by doffer id
  // so when doffer id changed we put a service zone there
  foreach(WinderModel *it, m_windersModel)
  {
    currentKey = it->idDoffer;
    if (prevKey != currentKey)
    {
      // doffer id changed
      if (counter > 0)
      {
        // settle new service zone after the winder's group
        QFrame *szone = new QFrame(this);
        szone->hide();
        szone->move(x, y);
        szone->setFrameStyle(QFrame::Box | QFrame::Sunken);
        m_services.append(szone);
        x += serviceZoneWidth;
      }
      // move to the new group
      x += serviceZoneWidth;

      // reset keys
      prevKey = currentKey;
      counter = 0;
    }

    // create and place winder widget
    Winder *winder = new Winder(*it, m_config.timeCoefficient, this);
    winder->hide();
    winder->move(x, y);
    m_winders.append(winder);
    x += winder->width() + space;
    // create signal-slot communication with supervisor
    connect(winder, SIGNAL(bobbinsReady(QString)), this, SLOT(bobbinsReady(QString)));
    connect(winder, SIGNAL(bobbinsCutNeeded(QString)), this, SLOT(bobbinsCutNeeded(QString)));
    connect(winder, SIGNAL(winderFailed(QString)), this, SLOT(winderFailed(QString)));
    connect(winder, SIGNAL(winderAlert(QString)), this, SLOT(winderAlert(QString)));

    counter++;
  }
  if (counter > 0)
  {
    // settle the last service zone
    QFrame *szone = new QFrame(this);
    szone->hide();
    szone->move(x, y);
    szone->setFrameStyle(QFrame::Box | QFrame::Sunken);
    m_services.append(szone);
    x += serviceZoneWidth;
  }

  int width = x;    // remember this width to set it later as the supervisor width

  // Create doffer container
  x = xPos;
  y += getMaxHeight<Winder>(m_winders);
  int i = 0;
  int dofferHeight = 0;

  foreach(DofferModel *it, m_doffersModel)
  {
    // if we're out of service zones stop the loop
    if (i >= m_services.size())
      break;
    // Create doffer and place it to the service zone
    Doffer *doffer = new Doffer(*it, this);
    QFrame *serv = m_services.at(i);
    doffer->hide();
    doffer->move(serv->x(), y);
    dofferHeight = doffer->getControlHeight();
    m_doffers.append(doffer);

    // create signal-slot communication with supervisor
    connect(doffer, SIGNAL(goalReached(QString)), this, SLOT(dofferArrived(QString)));
    connect(doffer, SIGNAL(bobAboard(QString)), this, SLOT(bobbinsAboard(QString)));
    connect(doffer, SIGNAL(bobPlaced(QString)), this, SLOT(bobbinPlaced(QString)));
    connect(doffer, SIGNAL(taskCompleted(QString)), this, SLOT(taskCompleted(QString)));
    connect(doffer, SIGNAL(movement(QString,QPoint,int)), this, SLOT(dofferMoved(QString,QPoint,int)));
    //create doffer log item
    appendLoggerItem(doffer->getId());
    connect(doffer, SIGNAL(updateLoggerItem(QString,Logger::FieldNames)), this, SLOT(updateLogger(QString,Logger::FieldNames)));

    i++;
  }

  // Create sleever container
  x = xPos;
  y += dofferHeight + space;
  i = 0;

  foreach(SleeverModel *it, m_sleeversModel)
  {
    // if we're out of service zones stop the loop
    if (i >= m_services.size())
      break;
    // Create sleever and place it to the service zone
    Sleever *sleever = new Sleever(*it, this);
    QFrame *serv = m_services.at(i);
    sleever->hide();
    sleever->move(serv->x(), y);
    m_sleevers.append(sleever);

    // create signal-slot communication with supervisor
    connect(sleever, SIGNAL(goalReached(QString)), this, SLOT(sleeverArrived(QString)));
    connect(sleever, SIGNAL(taskCompleted(QString)), this, SLOT(taskCompleted(QString)));
    connect(sleever, SIGNAL(emptySleever(QString)), this, SLOT(sleeverEmpty(QString)));
    connect(sleever, SIGNAL(movement(QString,QPoint,int)), this, SLOT(sleeverMoved(QString,QPoint,int)));
    //create sleever log item
    appendLoggerItem(sleever->getId());
    connect(sleever, SIGNAL(updateLoggerItem(QString,Logger::FieldNames)), this, SLOT(updateLogger(QString,Logger::FieldNames)));

    i++;
  }

  // Create spooler container
  x = xPos + serviceZoneWidth;
  y += getMaxHeight<Sleever>(m_sleevers) + space;
  i = 0;
  QString prevId = "";
  QList<Spooler *> section;   //contains groupped references to the spooler pairs
  int spWidth = 0;

  foreach(SpoolerModel *it, m_spoolersModel)
  {
    Spooler *spooler = new Spooler(*it, this);
    //if doffer changed then move to the next group
    if (it->idDoffer != prevId)
    {
      prevId = it->idDoffer;
      if (i > 0)
      {
        // if we're out of service zones stop the loop
        if (i - 1 >= m_services.size())
          break;
        // get the x-Pos of the service zone
        QFrame *serv = m_services.at(i - 1);
        x = serv->x() + (serviceZoneWidth << 1);
        // if group exists
        if (section.size() > 0)
        {
          // count the offset and move spoolers in the middle of winder group
          int offset = (serv->x() - section[0]->x() - spWidth) / 2;
          foreach (Spooler *sp, section)
          {
            QPoint pt = sp->pos();
            sp->move(pt.x() + offset, pt.y());
          }
        }
        // reset section
        spWidth = 0;
        section.clear();
      }
      i++;
    }

    // add spooler to container
    spooler->hide();
    spooler->move(x, y);
    m_spoolers.append(spooler);
    section.append(spooler);
    // create signal-slot communication with supervisor
    connect(spooler, SIGNAL(filledUp(QString)), this, SLOT(spoolerFilled(QString)));
    // move to next spooler
    x += spooler->width() + space;
    spWidth += spooler->width() + space;
  }
  // manage the last spooler group
  if (i > 0)
  {
    // get the last service zone
    QFrame *serv = m_services.at(i - 1);
    if (section.size() > 0)
    {
      // count the offset and move spoolers in the middle of winder group
      int offset = (serv->x() - section[0]->x() - spWidth) / 2;
      foreach (Spooler *sp, section)
      {
        QPoint pt = sp->pos();
        sp->move(pt.x() + offset, pt.y());
      }
    }
  }
  section.clear();

  // Create man-service container
  x = xPos;
  y += getMaxHeight<Spooler>(m_spoolers) + space;
  foreach(ManServiceModel *it, m_menModel)
  {
    ManService *man = new ManService(*it, this);

    // create signal-slot communication with supervisor
    connect(man, SIGNAL(goalReached(QString)), this, SLOT(manReached(QString)));
    connect(man, SIGNAL(taskCompleted(QString)), this, SLOT(taskCompleted(QString)));

    //add widget to container
    man->hide();
    man->move(x, y);
    m_men.append(man);

    //move to the next one
    x += man->width() + space;
  }

  // calculate supervisor widget size
  y += getMaxHeight<ManService>(m_men) + space;
  resize(width, y);
}
//_________________________________________________________
//
// Public method of starting supervisor activity by seeding models and creating containers
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::start()
{
  int space = 10;

  seed();                             // Seeding models
  countAspectRatio(space);            // Calculate aspect ratio for mm -> pxl convertions
  m_margin = toPixels(margin);
  initContainers(space, space * 2);   // Create child widget containers

  // show winders
  foreach(Winder *it, m_winders)
    it->show();
  // show service zones
  foreach(QFrame *it, m_services)
  {
    it->resize(toPixels(m_config.serviceZoneWidth), height() - space * 2);
    it->show();
  }
  // show doffers
  foreach(Doffer *it, m_doffers)
    it->show();
  // show sleevers
  foreach(Sleever *it, m_sleevers)
    it->show();
  // show spoolers
  foreach(Spooler *it, m_spoolers)
    it->show();
  // show man-services
  foreach(ManService *it, m_men)
    it->show();

  m_task_timer = startTimer(timerResolution);     // start supervisor task timer
  m_db_timer = startTimer(dbSyncResolution);      // start database update timer
}
//_________________________________________________________
//
// Public method of stopping supervisor activity
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::stop()
{
  // killing all timers
  if (m_task_timer > 0)
  {
    killTimer(m_task_timer);
    m_task_timer = 0;
  }
  if (m_db_timer > 0)
  {
    killTimer(m_db_timer);
    m_db_timer = 0;
  }

  // Clean up task queue
  foreach(TaskSession *it, m_tasks)
    if (it != NULL) delete it;
  m_tasks.clear();

  // Clean up containers
  foreach(Winder *it, m_winders)
    if (it != NULL) delete it;
  foreach(QFrame *it, m_services)
    if (it != NULL) delete it;
  foreach(Doffer *it, m_doffers)
    if (it != NULL) delete it;
  foreach(Sleever *it, m_sleevers)
    if (it != NULL) delete it;
  foreach(Spooler *it, m_spoolers)
    if (it != NULL) delete it;
  foreach(ManService *it, m_men)
    if (it != NULL) delete it;

  m_winders.clear();
  m_services.clear();
  m_doffers.clear();
  m_sleevers.clear();
  m_spoolers.clear();
  m_men.clear();

  modelClear();         //Clean up models
}
//_________________________________________________________
//
// Clean up of models
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Supervisor::modelClear()
{
  foreach(WinderModel *it, m_windersModel)
    if (it != NULL) delete it;
  foreach(DofferModel *it, m_doffersModel)
    if (it != NULL) delete it;
  foreach(SleeverModel *it, m_sleeversModel)
    if (it != NULL) delete it;
  foreach(SpoolerModel *it, m_spoolersModel)
    if (it != NULL) delete it;
  foreach(ManServiceModel *it, m_menModel)
    if (it != NULL) delete it;

  m_windersModel.clear();
  m_doffersModel.clear();
  m_sleeversModel.clear();
  m_spoolersModel.clear();
  m_menModel.clear();
}
//_________________________________________________________
//
// Find the first free man-service
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ManService *Supervisor::getFreeMan()
{
  foreach(ManService *it, m_men)
  {
    if (it->getStatus() == ManService::IDLE)
      return it;
  }
  return NULL;
}
//_________________________________________________________
//
// Find the first least busy man-service
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ManService *Supervisor::getLeastBusyMan()
{
  // Create dictionary containing amount of tasks for each man-service
  QMap<QString, int> stat;
  foreach(TaskSession *it, m_tasks)
  {
    // taking into account only man-service tasks
    if (it->type != START_WINDER &&
        it->type != ROTATE_SPOOLER &&
        it->type != CHANGE_SPOOLER &&
        it->type != CUTEDGE_WINDER &&
        it->type != LOAD_SLEEVER)
      continue;
    // append or increment the dictionary item
    if (!stat.contains(it->idAssignee))
      stat[it->idAssignee] = 1;
    else
      stat[it->idAssignee]++;
  }

  // find man-service id with minimum tasks amount
  QString minId = "";
  int minimum = -1;
  foreach(QString key, stat.keys())
  {
    if (stat[key] < minimum || minimum == -1)
    {
      minimum = stat[key];
      minId = key;
    }
  }
  if (minId == "") return NULL;

  //return man-service by id
  return getItemById<ManService>(minId, m_men);
}
//_________________________________________________________
//
// Find the nearest busy man-service
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ManService *Supervisor::getNearestMan(int xPos)
{
  int delta = -1;
  ManService *nearest = NULL;
  // find man-service
  foreach(ManService *it, m_men)
  {
    if (!it->isMoving())    // which is not moving now
    {
      // and has minimal distance from the object xPos
      int curDelta = abs(it->x() + it->width() / 2 - xPos);
      if (curDelta < delta || delta == -1)
      {
        delta = curDelta;
        nearest = it;
      }
    }
  }
  return nearest;
}
//_________________________________________________________
//
// Query a man-service depending on strategy
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ManService *Supervisor::getManByStrategy(int xPos/* = 0*/, ManStrategy ms/* = NEAREST_OR_LEASTBUSY*/)
{
  ManService *man = getFreeMan();
  switch(ms)
  {
    case FREE_OR_LEASTBUSY:
      if (man == NULL)
        man = getLeastBusyMan();
      break;
    case FREE_OR_NEAREST:
      if (man == NULL)
        man = getNearestMan(xPos);
      break;
    case NEAREST_OR_LEASTBUSY:
      man = getNearestMan(xPos);
      if (man == NULL)
        man = getLeastBusyMan();
      break;
    default:
      break;
  }

  return man;
}
//_________________________________________________________
//
// Create task session to power up winders
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Supervisor::startWinders()
{
  //query a man-service
  ManService *man = getManByStrategy(0, FREE_OR_LEASTBUSY);
  if (man == NULL) return false;

  // Create task for each winder
  foreach (Winder *it, m_winders)
  {
    // which is empty or failed
    if (it->getStatus() == Winder::EMPTY ||
        it->getStatus() == Winder::FAIL)
    {
      TaskSession *task = new TaskSession;
      task->idSession = QUuid::createUuid().toString();
      task->status = NEW;
      task->type = START_WINDER;
      task->idAssignee = man->getId();
      task->idObject = it->getId();
      task->places = 0;
      m_tasks.append(task);
    }
  }

  return true;
}
//_________________________________________________________
//
// Create task for sleever loading
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Supervisor::createLoadSleeverTask(Sleever *sleever)
{
  if (sleever == NULL) return false;
  // query a man-service
  ManService *man = getManByStrategy(sleever->x());
  if (man == NULL) return false;

  // Add new task session
  TaskSession *task = new TaskSession;
  task->idSession = QUuid::createUuid().toString();
  task->status = NEW;
  task->type = LOAD_SLEEVER;
  task->idAssignee = man->getId();
  task->idObject = sleever->getId();
  task->places = 0;
  m_tasks.append(task);
  return true;
}
//_________________________________________________________
//
// Slot creates task for spooler rotate or change
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::spoolerFilled(QString idSpooler)
{
  //check spooler
  Spooler *dest = getItemById<Spooler>(idSpooler, m_spoolers);
  if (dest == NULL) return;
  // query man-service
  ManService *man = getManByStrategy(dest->x());
  if (man == NULL) return;

  // Create new task
  TaskSession *task = new TaskSession;
  task->idSession = QUuid::createUuid().toString();
  task->status = NEW;
  task->type = dest->isAbleToRotate() ? ROTATE_SPOOLER : CHANGE_SPOOLER;
  task->idAssignee = man->getId();
  task->idObject = idSpooler;
  task->places = 0;
  m_tasks.append(task);
}
//_________________________________________________________
//
// Create sleever delivering task
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::callSleever(QString idWinder)
{
  // find winder model
  WinderModel *it = getItemById<WinderModel>(idWinder, m_windersModel);
  if (it == NULL) return;

  // check winder
  Winder *winder = getItemById<Winder>(idWinder, m_winders);
  if (winder == NULL) return;
  if (winder->getStatus() == Winder::FAIL) return;

  // create new task
  TaskSession *task = new TaskSession;
  task->idSession = QUuid::createUuid().toString();
  task->status = NEW;
  task->type = DELIVER_SLEEVE;
  task->idAssignee = it->idSleever;
  task->idObject = idWinder;
  task->places = it->isHalfMode ? 1 : 2;
  m_tasks.append(task);
}
//_________________________________________________________
//
// Slot activates doffering task creation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::bobbinsReady(QString idWinder)
{
  //find winder model
  WinderModel *it = getItemById<WinderModel>(idWinder, m_windersModel);
  if (it == NULL) return;

  //check winder
  Winder *winder = getItemById<Winder>(idWinder, m_winders);
  if (winder == NULL) return;
  if (winder->getStatus() == Winder::FAIL) return;

  //create new task
  TaskSession *task = new TaskSession;
  task->idSession = QUuid::createUuid().toString();
  task->status = NEW;
  task->type = DELIVER_BOBBINS;
  task->idAssignee = it->idDoffer;
  task->idObject = idWinder;
  task->places = it->isHalfMode ? 1 : 2;
  m_tasks.append(task);
}
//_________________________________________________________
//
// Slot activates cutedge task creation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::bobbinsCutNeeded(QString idWinder)
{
  // Check winder
  Winder *winder = getItemById<Winder>(idWinder, m_winders);
  if (winder == NULL) return;
  if (winder->getStatus() != Winder::CUTEDGE) return;
  // Query a man-service
  ManService *man = getManByStrategy(winder->x());
  if (man == NULL) return;
  // Create new task
  TaskSession *task = new TaskSession;
  task->idSession = QUuid::createUuid().toString();
  task->status = NEW;
  task->type = CUTEDGE_WINDER;
  task->idAssignee = man->getId();
  task->idObject = idWinder;
  task->places = 0;
  m_tasks.append(task);
}
//_________________________________________________________
//
// Slot activates moving sleever to the service zone task creation.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::sleeverEmpty(QString idSleever)
{
  //Create new task
  TaskSession *task = new TaskSession;
  task->idSession = QUuid::createUuid().toString();
  task->status = NEW;
  task->type = MOVE_SLEEVER;
  task->idAssignee = idSleever;
  task->places = 0;
  m_tasks.append(task);
}
//_________________________________________________________
//
// Slot activates moving doffer and sleever task creation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::winderAlert(QString idWinder)
{
  // Check winder
  WinderModel *winderModel = getItemById<WinderModel>(idWinder, m_windersModel);
  if (winderModel == NULL) return;
  // Check doffer
  Doffer *doffer = getItemById<Doffer>(winderModel->idDoffer, m_doffers);
  if (doffer == NULL) return;
  // Check doffer status
  if (doffer->getStatus() != Doffer::IDLE)
    return;

  // Create new task
  TaskSession *task = new TaskSession;
  task->idSession = QUuid::createUuid().toString();
  task->status = NEW;
  task->type = MOVE_DOFFER_SLEEVER;
  task->idAssignee = winderModel->idDoffer;
  task->idObject = idWinder;
  task->places = 0;
  m_tasks.append(task);
}
//_________________________________________________________
//
// Create handle collision task
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::handleCollision(Doffer *doffer, Sleever *sleever, bool isDofferPriority, bool waitDoffer, bool waitSleever)
{
  if (doffer == NULL || sleever == NULL) return;

  TaskSession *task = new TaskSession;
  task->idSession = QUuid::createUuid().toString();
  task->status = NEW;
  task->type = HANDLE_COLLISION;
  task->idAssignee = doffer->getId();
  task->idObject = sleever->getId();
  task->places = isDofferPriority;
  task->waitDoffer = waitDoffer;
  task->waitSleever = waitSleever;
  m_tasks.append(task);
}
//_________________________________________________________
//
// Slot activates removing of failed winder doffering or sleever tasks
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::winderFailed(QString idWinder)
{
  // Cancel all new doffer & sleever tasks with this winder
  foreach (TaskSession *ts, m_tasks) {
    if (ts->idObject == idWinder && ts->status == NEW &&
       (ts->type == DELIVER_BOBBINS || ts->type == DELIVER_SLEEVE))
      cancelTask(ts);
  }
}
//_________________________________________________________
//
// Cancel task session
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::cancelTask(TaskSession *ts)
{
  if (ts == NULL) return;
  //qDebug() << "<====== Cancelled " << ts->type << ts->idAssignee << ts->idObject << ts->idSession;

  switch(ts->type)
  {
    case START_WINDER:
    case ROTATE_SPOOLER:
    case CHANGE_SPOOLER:
    case LOAD_SLEEVER:
    case CUTEDGE_WINDER:
      {
        // For man service cancelling leads to idle status
        // Check man-service
        ManService *it = getItemById<ManService>(ts->idAssignee, m_men);
        // Set status as idle
        if (it != NULL)
          it->setStatus(ManService::IDLE);
        break;
      }
    case DELIVER_BOBBINS:
      {
        // cancel possible spooler reserve
        cancelSpoolerReservation(ts->reserve);
        // check doffer
        Doffer *it = getItemById<Doffer>(ts->idAssignee, m_doffers);
        if (it != NULL)
        {
          // Set doffer status as idle
          it->setStatus(Doffer::IDLE);
          // Reset amount of bobbins
          it->resetAmount();
        }
        // Start linked objects
        activateLinkedObjects(ts, false);
        break;
      }
    case DELIVER_SLEEVE:
      {
        // Check sleever
        Sleever *it = getItemById<Sleever>(ts->idAssignee, m_sleevers);
        // Set status to idle
        if (it != NULL)
          it->setStatus(Sleever::IDLE);
        // Start linked objects
        activateLinkedObjects(ts, true);
        break;
      }
    case MOVE_SLEEVER:
    case MOVE_DOFFER_SLEEVER:
    case HANDLE_COLLISION:
      break;

    default:
      break;
  }
  // set task session status
  ts->status = CANCELLED;
}
//_________________________________________________________
//
// Event handler for timer counters
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::timerEvent(QTimerEvent* te)
{
  // supervisor task management timer
  if (te->timerId() == m_task_timer)
  {
    // delete cancelled and done tasks from queue
    for(QList<TaskSession *>::iterator it = m_tasks.begin(); it != m_tasks.end();)
    {
      if ((*it)->status == DONE || (*it)->status == CANCELLED)
      {
        //qDebug() << "Delete " << (*it)->idSession << (*it)->status;
        delete (*it);
        it = m_tasks.erase(it);
      }
      else
        ++it;
    }
    // try to start every new or paused task
    foreach (TaskSession *ts, m_tasks)
    {
      if (ts->status == NEW || ts->status == PAUSED)
        startMachine(ts);
    }
  }
  // database update timer
  if (te->timerId() == m_db_timer)
  {
    // update doffer & sleever models
    sync();
  }
}
//_________________________________________________________
//
// Private task starter
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::startMachine(TaskSession *ts)
{
  switch(ts->type)
  {
    case START_WINDER:
    case ROTATE_SPOOLER:
    case CHANGE_SPOOLER:
    case LOAD_SLEEVER:
    case CUTEDGE_WINDER:
      runManServiceTask(ts);
      break;
    case DELIVER_BOBBINS:
    case MOVE_DOFFER_SLEEVER:
      runDofferingTask(ts);
      break;
    case DELIVER_SLEEVE:
    case MOVE_SLEEVER:
      runSleeverTask(ts);
      break;
    case HANDLE_COLLISION:
      runHandleCollisionTask(ts);
      break;
    default:
      break;
  }
}
//_________________________________________________________
//
// Private task starter for man-service
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::runManServiceTask(TaskSession *ts)
{
  // cancel task if man-service is wrong
  ManService *man = getItemById<ManService>(ts->idAssignee, m_men);
  if (man == NULL)
  {
    cancelTask(ts);
    return;
  }
  // pause task if man-service is not idle
  if (man->getStatus() != ManService::IDLE)
  {
    // if assigned man is busy ask to do a favor a free one
    /*man = getFreeMan();
    if (man != NULL)
      ts->idAssignee = man->getId();
    else
    {*/
      // assigned man is not available yet
      ts->status = PAUSED;
      return;
    //}
  }
  // set to progress
  ts->status = PROGRESS;

  QFrame *obj = NULL;
  switch(ts->type)
  {
    case START_WINDER:
    case CUTEDGE_WINDER:
      // get winder from object id
      obj = getItemById<Winder>(ts->idObject, m_winders);
      break;
    case ROTATE_SPOOLER:
    case CHANGE_SPOOLER:
      // get spooler from object id
      obj = getItemById<Spooler>(ts->idObject, m_spoolers);
      break;
    case LOAD_SLEEVER:
      // get sleever from object id
      obj = getItemById<Sleever>(ts->idObject, m_sleevers);
      break;
    default:
      break;
  }
  // if object is wrong cancel task
  if (obj == NULL)
  {
    cancelTask(ts);
    return;
  }

  //set man-service status to ready
  man->setStatus(ManService::READY);
  man->update();
  // call reaching object method
  man->reachObject(ts->idSession, obj->x(), obj->y());
}
//_________________________________________________________
//
// Create spooler reservation
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Supervisor::tryReserveSpooler(QString idSpooler, SpoolerReservation &spres)
{
  // check spooler
  Spooler *spooler = getItemById<Spooler>(idSpooler, m_spoolers);
  if (spooler == NULL) return false;

  // init spooler reservation structure
  spres.idSpooler = spooler->getId();
  return spooler->reserve(spres.row, spres.column);
}
//_________________________________________________________
//
// Cancel spooler reservation
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::cancelSpoolerReservation(QVector<SpoolerReservation> &resarray)
{
  // for each reserved item
  foreach(SpoolerReservation it, resarray)
  {
    // check spooler
    Spooler *spooler = getItemById<Spooler>(it.idSpooler, m_spoolers);
    if (spooler == NULL) continue;
    // cancel reservation
    spooler->cancelReserve(it.row, it.column);
  }
}
//_________________________________________________________
//
// Private task starter for doffer
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::runDofferingTask(TaskSession *ts)
{
  // check if doffer & winder correct. If not cancel task
  Doffer *doffer = getItemById<Doffer>(ts->idAssignee, m_doffers);
  Winder *winder = getItemById<Winder>(ts->idObject, m_winders);
  if (doffer == NULL || winder == NULL)
  {
    cancelTask(ts);
    return;
  }

  //qDebug() << "Doffer task ====================";
  //qDebug() << ts->type << ts->idAssignee << ts->idObject << ts->status << ts->idSession ;

  // if doffer is linked pause task
  if (testObjectId(doffer->getId()))
  {
    //qDebug() << doffer->getId() << "has been linked -- pausing" << ts->type << ts->idSession;
    ts->status = PAUSED;
    return;
  }

  switch(ts->type)
  {
    case MOVE_DOFFER_SLEEVER:   // moving doffer and sleever to winder
    {
      // jump to doffer arrived routine if doffer waits for winder ready
      if (doffer->getStatus() == Doffer::WAITWINDER && !doffer->isMoving())
      {
        ts->status = PROGRESS;
        dofferArrived(ts->idSession);
        return;
      }
      // cancel task if doffer is not idle or is already moving
      if (doffer->getStatus() != Doffer::IDLE || doffer->isMoving())
      {
        cancelTask(ts);
        return;
      }

      // set task to progress
      ts->status = PROGRESS;
      //qDebug() << "MOVE_DOFFER_SLEEVER " << winder->getId() << ts->idSession;
      // reach the winder
      moveDofferAndSleever(ts->idSession, winder->getId(), winder->getBobbinsRect().topLeft(), false);
      // set doffer status to winder waiting
      doffer->setStatus(Doffer::WAITWINDER);
      doffer->update();
      break;
    }
    case DELIVER_BOBBINS:   // reach the winder, take bobbins and deliver them to spooler
    {
      // if doffer is waiting for sleever jump to doffer arrived routine
      if (doffer->getStatus() == Doffer::WAIT)
      {
        ts->status = PROGRESS;
        doffer->setStatus(doffer->getAmount() > 0 ? Doffer::DELIVER : Doffer::READY);
        dofferArrived(ts->idSession);
        return;
      }
      // pause task if doffer is not idle
      if (doffer->getStatus() != Doffer::IDLE)
      {
        ts->status = PAUSED;
        return;
      }
      // stop doffer if doffer is moving
      // set task to paused
      if (doffer->isMoving())
      {
        doffer->stopMoving(false);
        ts->status = PAUSED;
        return;
      }

      // reserve spooler positions depending on bobbins amount
      int counter = ts->places;
      ts->reserve.clear();
      foreach(SpoolerModel *spl, m_spoolersModel)
      {
        SpoolerReservation spres;
        if (spl->idDoffer == ts->idAssignee)
          for(; counter > 0; counter--)
          {
            if (!tryReserveSpooler(spl->idSpooler, spres))
              break;
            ts->reserve.append(spres);
          }
        if (counter == 0) break;
      }

      // pause task if reservation failed
      if (counter > 0)
      {
        cancelSpoolerReservation(ts->reserve);
        ts->status = PAUSED;
        return;
      }

      // set task to progress
      ts->status = PROGRESS;
      // set doffer state to ready
      doffer->setStatus(Doffer::READY);
      // set bobbins size for animation
      doffer->setBobbinsSize(winder->getBobbinsRect().size());
      //qDebug() << "DELIVER_BOBBINS to " << winder->getId() << ts->idSession;
      // reach the winder
      moveDofferAndSleever(ts->idSession, winder->getId(), winder->getBobbinsRect().topLeft(), true);
      doffer->update();
      break;
    }
    default:
      break;
  }
}
//_________________________________________________________
//
// Move doffer and sleever to the winder position
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::moveDofferAndSleever(QString idDofferSession, QString idWinder, QPoint dest, bool allowReadyDoffer)
{
  // find session
  TaskSession *ts = getItemById<TaskSession>(idDofferSession, m_tasks);
  // find winder model
  WinderModel *it = getItemById<WinderModel>(idWinder, m_windersModel);
  if (it == NULL) return;

  // find winder, doffer, sleever
  Winder *winder = getItemById<Winder>(idWinder, m_winders);
  Doffer *doffer = getItemById<Doffer>(it->idDoffer, m_doffers);
  Sleever *sleever = getItemById<Sleever>(it->idSleever, m_sleevers);
  if (winder == NULL || doffer == NULL || sleever == NULL) return;

  // consider if moving is possible
  bool moveDoffer = ((allowReadyDoffer && doffer->getStatus() == Doffer::READY) ||
                     (doffer->getStatus() == Doffer::IDLE)) &&
                     !doffer->isMoving() && !testObjectId(doffer->getId());
  bool moveSleever = (sleever->getStatus() == Sleever::IDLE ||
                      sleever->getStatus() == Sleever::PREPARING) &&
                    !sleever->isMoving() && !testObjectId(sleever->getId());
  if (!moveDoffer && !moveSleever) return;

  // calculate new sleever position
  int sleeverX = countNewSleeverX(ts, doffer, sleever, dest.x());
  // send sleever & doffer to winder
  if (moveDoffer)
    doffer->reachObject(idDofferSession, dest.x(), dest.y());
  if (moveDoffer && moveSleever)
    sleever->reachObject("", sleeverX, sleever->y(), false);
}
//_________________________________________________________
//
// Calculate new sleever position based on the spooler and doffer position
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Supervisor::countNewSleeverX(TaskSession *ts, Doffer *doffer, Sleever *sleever, int destX)
{
  int xOffset = 0;
  Spooler *spooler = NULL;
  // if reservation exists get the first one
  if (ts != NULL && ts->reserve.size() > 0)
  {
    // check spooler
    spooler = getItemById<Spooler>(ts->reserve[0].idSpooler, m_spoolers);
    if (spooler == NULL) return sleever->x();
    // calculate the offset
    xOffset = ts->reserve[0].column * spooler->getCellWidth();
  }
  else  //otherwise get the first spooler position
    foreach(SpoolerModel *spl, m_spoolersModel)
    {
      if (spl->idDoffer == doffer->getId())
      {
        // if spooler not found take the sleever position
        spooler = getItemById<Spooler>(spl->idSpooler, m_spoolers);
        if (spooler == NULL) return sleever->x();
        break;
      }
    }
  // update the offset
  xOffset+=spooler->x();

  // calculate new sleever pos
  return (xOffset > destX ? (destX - sleever->width() - (m_margin << 1)) : (destX + doffer->width() + (m_margin << 1)));
}
//_________________________________________________________
//
// Private task starter for sleever
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::runSleeverTask(TaskSession *ts)
{
  // qDebug() << "Sleever task ====================";
  // qDebug() << ts->type << ts->idAssignee << ts->idObject << ts->status << ts->idSession ;

  // Check sleever. If wrong cancel task
  Sleever *sleever = getItemById<Sleever>(ts->idAssignee, m_sleevers);
  if (sleever == NULL)
  {
    cancelTask(ts);
    return;
  }
  // if sleever is linked pause task
  if (testObjectId(sleever->getId()))
  {
    //qDebug() << sleever->getId() << "has been linked -- pausing";
    ts->status = PAUSED;
    return;
  }

  switch(ts->type)
  {
    case DELIVER_SLEEVE:  // reach the winder and put the sleeve on it
      {
        // check if winder correct. If not, cancel task
        Winder *winder = getItemById<Winder>(ts->idObject, m_winders);
        if (winder == NULL)
        {
          cancelTask(ts);
          return;
        }
        // check if sleever is not idle and not wait for doffer. If so, pause task
        if (sleever->getStatus() != Sleever::IDLE && sleever->getStatus() != Sleever::WAIT)
        {
          ts->status = PAUSED;
          return;
        }
        // if sleever is moving then stop it
        // set task to paused
        if (sleever->isMoving())
        {
          sleever->stopMoving(false);
          ts->status = PAUSED;
          return;
        }

        // set task to progress
        ts->status = PROGRESS;
        // set sleever state to ready
        sleever->setStatus(Sleever::READY);
        sleever->update();
        // set bobbins size for animation
        sleever->setBobbinsSize(winder->getBobbinsRect().size());
        // reach the winder
        QPoint dest = winder->getBobbinsRect().topLeft();
        //qDebug() << "DELIVER_SLEEVE" << sleever->getId() << sleever->getStatus() << " to " << winder->getId() << ts->idSession;
        sleever->reachObject(ts->idSession, dest.x(), dest.y());
        break;
      }
    case MOVE_SLEEVER:    // reach the service zone
      {
        // cancel task if the sleever is not empty
        if (sleever->getStatus() != Sleever::EMPTY)
        {
          cancelTask(ts);
          return;
        }
        // pause task if the sleever is moving
        if (sleever->isMoving())
        {
          sleever->stopMoving(false);
          ts->status = PAUSED;
          return;
        }

        //get the nearest service zone
        int nearestService = m_services.last()->x();
        // calculate delta for each service zone to find minimum
        foreach(QFrame *it, m_services)
        {
          int delta = it->x() - sleever->x();
          if ((delta >= 0 && it->x() < nearestService))
            nearestService = it->x();
        }
        // set task to progress
        ts->status = PROGRESS;
        //qDebug() << "MOVE_SLEEVER to service zone" << ts->idSession;
        // send sleever to service zone
        sleever->reachObject(ts->idSession, nearestService, 0);
        break;
      }
    default:
      break;
  }
}
//_________________________________________________________
//
// Notification slot which runs the next man-service task session phase
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::manReached(QString idSession)
{
  // check session
  TaskSession *ts = getItemById<TaskSession>(idSession, m_tasks);
  if (ts == NULL) return;

  // cancel task if man-service is wrong
  ManService *man = getItemById<ManService>(ts->idAssignee, m_men);
  if (man == NULL)
  {
    cancelTask(ts);
    return;
  }
  // Man has been reached the object. Run the object action
  switch(ts->type)
  {
    case START_WINDER:
      man->startWinder(idSession);
      break;
    case CUTEDGE_WINDER:
      man->cutEdgeOnWinder(idSession);
      break;
    case ROTATE_SPOOLER:
      man->rotateSpooler(idSession);
      break;
    case CHANGE_SPOOLER:
      man->changeSpooler(idSession);
      break;
    case LOAD_SLEEVER:
      man->loadSleever(idSession);
      break;
    default:
      break;
  }
  // for the spooler:
  switch(ts->type)
  {
    case ROTATE_SPOOLER:
    case CHANGE_SPOOLER:
    {
      // Change spooler status to reflect the spooler action
      Spooler *spooler = getItemById<Spooler>(ts->idObject, m_spoolers);
      if (spooler != NULL)
      {
        spooler->setStatus(Spooler::BUSY);
        spooler->update();
      }
      break;
    }
    default:
      break;
  }
}
//_________________________________________________________
//
// Slot Notification for task completion
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::taskCompleted(QString idSession)
{
  // check session
  TaskSession *ts = getItemById<TaskSession>(idSession, m_tasks);
  if (ts == NULL) return;

  //qDebug() << "<------- Completed " << ts->type << ts->idAssignee << ts->idObject << ts->idSession;

  switch(ts->type)
  {
    case START_WINDER:  // man service has finished the start winder task
      {
        // check winder
        Winder *winder = getItemById<Winder>(ts->idObject, m_winders);
        if (winder != NULL)
        {
          // set winder state to loaded and start windong
          winder->setStatus(Winder::LOADED);
          winder->startWinding();
        }
        break;
      }
    case CUTEDGE_WINDER:  // man service has finished the cut bobbins winder task
      {
        // check winder
        Winder *winder = getItemById<Winder>(ts->idObject, m_winders);
        if (winder != NULL)
        {
          // set winder state to ready
          winder->setStatus(Winder::READY);
          // switch off the cut edge mode
          winder->setCutEdgeMode(false);
          winder->update();
          // check if the whole winder group is ready to call the doffer
          setGroupBobbinsReady(winder->getId());
        }
        break;
      }
    case ROTATE_SPOOLER:
    case CHANGE_SPOOLER:  // man service has finished changing spooler task
      {
        // check spooler
        Spooler *spooler = getItemById<Spooler>(ts->idObject, m_spoolers);
        if (spooler != NULL)
        {
          // run replace action
          spooler->replace();
          // change status
          spooler->setStatus(Spooler::PROGRESS);
          spooler->update();
          // re-order spoolers thus the empty one become the last
          moveSpoolerToTail(spooler->getId());
        }
        break;
      }
    case LOAD_SLEEVER:  // man service has finished reloading sleever task
      {
        // check sleever and sleever model
        Sleever *item = getItemById<Sleever>(ts->idObject, m_sleevers);
        SleeverModel *it = getItemById<SleeverModel>(ts->idObject, m_sleeversModel);
        if (item == NULL || it == NULL) break;
        // run set inventory action
        item->setInventory(it->sleeveSlots, it->rings);
        // change status
        item->setStatus(Sleever::IDLE);
        item->update();
        break;
      }
    case DELIVER_BOBBINS:   // doffer has finished delivering
      {
        if (ts->reserve.size() > 0)
        {
          // run spooler putdown action for existing reservation
          Spooler *spooler = getItemById<Spooler>(ts->reserve[0].idSpooler, m_spoolers);
          if (spooler != NULL)
            spooler->putdown(ts->reserve[0].row, ts->reserve[0].column);
        }
        // activate linked objects
        activateLinkedObjects(ts, false);
        break;
      }
    case DELIVER_SLEEVE:  // sleever has finished delivering
      {
        // check winder
        Winder *winder = getItemById<Winder>(ts->idObject, m_winders);
        if (winder != NULL && winder->getStatus() != Winder::FAIL)
          winder->setStatus(Winder::LOADED);    // set loaded state
        // activate linked objects
        activateLinkedObjects(ts, true);
        break;
      }
    case MOVE_SLEEVER:
      break;
    case MOVE_DOFFER_SLEEVER: // doffer has finished movement
      {
        // check doffer
        Doffer *it = getItemById<Doffer>(ts->idAssignee, m_doffers);
        if (it != NULL)
          it->setStatus(Doffer::IDLE);    // set idle state
        break;
      }
    case HANDLE_COLLISION: // supervisor has finished collision analysis
      {
        // activate linked objects
        activateLinkedObjects(ts, ts->places == 0);
        break;
      }
    default:
      break;
  }
  ts->status = DONE;
}
//_________________________________________________________
//
// Slot activated after doffer has reached the winder (READY) or spooler (DELIVER)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::dofferArrived(QString idSession)
{
  // check task session
  TaskSession *ts = getItemById<TaskSession>(idSession, m_tasks);
  if (ts == NULL) return;

  // cancel task if doffer is wrong
  Doffer *doffer = getItemById<Doffer>(ts->idAssignee, m_doffers);
  if (doffer == NULL)
  {
    cancelTask(ts);
    return;
  }

  // Check if sleever is under the doffer
  if (doffer->getStatus() == Doffer::READY || doffer->getStatus() == Doffer::DELIVER)
  {
    // cancel task if winder or winderModel are wrong
    Winder *winder = getItemById<Winder>(ts->idObject, m_winders);
    WinderModel *winModel = getItemById<WinderModel>(ts->idObject, m_windersModel);
    if (winder == NULL || winModel == NULL)
    {
      cancelTask(ts);
      return;
    }
    // cancel task if sleever is wrong
    Sleever *sleever = getItemById<Sleever>(winModel->idSleever, m_sleevers);
    if (sleever == NULL)
    {
      cancelTask(ts);
      return;
    }

    // calculate doffer and sleever rectangles
    QRect dofferRect;
    QRect sleeverRect;
    setLocatorRectanges(doffer, sleever, dofferRect, sleeverRect);

    // test intersection and calculate new xpos for sleever
    int newX = 0;
    bool askMove = false;
    if ((sleeverRect.right() >= dofferRect.left() && sleeverRect.right() <= dofferRect.right()) ||
        (dofferRect.left() <= sleeverRect.left() && dofferRect.right() >= sleeverRect.right()))
    {
      // set new sleever pos at the left of doffer
      newX = dofferRect.left() - sleever->width() - (m_margin << 1);
      askMove = true;
    }
    else if ((sleeverRect.left() >= dofferRect.left() && sleeverRect.left() <= dofferRect.right()) ||
             (sleeverRect.left() <= dofferRect.left() && sleeverRect.right() >= dofferRect.right()))
    {
      //set new sleever pos at the right of doffer
      newX = dofferRect.right() + (m_margin << 1);
      askMove = true;
    }

    if (askMove)
    {
      if (!sleever->isMoving())
      {
        // ask sleever to move away if it's (idle and not moving) or linked
        if (sleever->getStatus() == Sleever::IDLE ||
            sleever->getStatus() == Sleever::PREPARING ||
            testObjectId(ts, sleever->getId()))
          sleever->reachObject(sleever->getSession(), newX, sleever->y(), false);
        else
        {
          // static ready-to-ready collision: link doffer to sleever
          ts->destPoint.setX(doffer->getDestX());
          ts->destPoint.setY(doffer->getDestY());
          // create task to handle collision
          handleCollision(doffer, sleever, false, false, false);
          return;
        }
      }

      // pause task
      ts->status = PAUSED;
      doffer->setStatus(Doffer::WAIT);
      return;
    }

    if (sleever->isMoving() &&
        (sleever->getStatus() == Sleever::IDLE ||
         sleever->getStatus() == Sleever::PREPARING) )
    {
      // sleever goes to doffer, need to wait for it
      ts->status = PAUSED;
      doffer->setStatus(Doffer::WAIT);
      return;
    }
  }

  if (doffer->getStatus() == Doffer::READY)   // doffer reached the winder and ready to take bobbins
  {
    // cancel task if winder is wrong
    Winder *winder = getItemById<Winder>(ts->idObject, m_winders);
    if (winder == NULL || winder->getStatus() == Winder::FAIL)
    {
      cancelTask(ts);
      return;
    }
    // update winder status
    winder->setStatus(Winder::EMPTY);
    winder->update();
    // start getting bobbins subtask
    doffer->getResult(idSession, ts->places);
  }
  else if (doffer->getStatus() == Doffer::DELIVER)  // doffer reached the spooler and ready to put bobbins
  {
    if (ts->reserve.size() > 0)
    {
      // cancel task if the spooler is wrong
      Spooler *spooler = getItemById<Spooler>(ts->reserve[0].idSpooler, m_spoolers);
      if (spooler == NULL)
      {
        cancelTask(ts);
        return;
      }
      // start putting bobbins subtask
      doffer->putResult(ts->idSession, ts->reserve[0].row, ts->reserve[0].column, ts->places);
    }
    else
    {
      cancelTask(ts);
      return;
    }
  }
  else if (doffer->getStatus() == Doffer::WAITWINDER) // doffer reached the winder
  {
    // cancel task if winder is wrong
    Winder *winder = getItemById<Winder>(ts->idObject, m_winders);
    if (winder == NULL)
    {
      cancelTask(ts);
      return;
    }
    //qDebug() << "WAITWINDER arrived " << winder->getId() << winder->getStatus() << ts->idSession;
    // wait for winder if it's not ready
    if (winder->getStatus() != Winder::READY)
    {
      ts->status = PAUSED;
      return;
    }
    // complete task
    taskCompleted(ts->idSession);
    return;
  }
}
//_________________________________________________________
//
// Slot activated when doffer completed the bobbins loading
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::bobbinsAboard(QString idSession)
{
  // check task session
  TaskSession *ts = getItemById<TaskSession>(idSession, m_tasks);
  if (ts == NULL) return;

  // cancel task if doffer is wrong
  Doffer *doffer = getItemById<Doffer>(ts->idAssignee, m_doffers);
  if (doffer == NULL || ts->places == 0 || ts->reserve.size() != ts->places)
  {
    cancelTask(ts);
    return;
  }
  // get destination spooler from reserve data field
  Spooler *spooler = getItemById<Spooler>(ts->reserve[0].idSpooler, m_spoolers);
  if (spooler == NULL)
  {
    cancelTask(ts);
    return;
  }

  // create sleever task
  callSleever(ts->idObject);

  // start reaching subtask
  doffer->reachObject(ts->idSession, spooler->x() + ts->reserve[0].column * spooler->getCellWidth(), spooler->y());
}
//_________________________________________________________
//
// Slot activated when doffer has put the first bobbin.
// last bobbin will activate taskCompleted slot
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::bobbinPlaced(QString idSession)
{
  // check task session and doffer
  TaskSession *ts = getItemById<TaskSession>(idSession, m_tasks);
  if (ts == NULL) return;

  // cancel task if doffer is wrong
  Doffer *doffer = getItemById<Doffer>(ts->idAssignee, m_doffers);
  if (doffer == NULL || ts->places == 0 || ts->reserve.size() != ts->places)
  {
    cancelTask(ts);
    return;
  }

  // cancel task if destination spooler is wrong
  Spooler *spooler = getItemById<Spooler>(ts->reserve[0].idSpooler, m_spoolers);
  if (spooler == NULL)
  {
    cancelTask(ts);
    return;
  }
  // run spooler action and set cell status
  spooler->putdown(ts->reserve[0].row, ts->reserve[0].column);

  // reach this or another destination spooler
  ts->places--;
  ts->reserve.removeFirst();
  if (ts->reserve.size() == 0)
  {
    cancelTask(ts);
    return;
  }
  // cancel task if destination spooler is wrong
  spooler = getItemById<Spooler>(ts->reserve[0].idSpooler, m_spoolers);
  if (spooler == NULL)
  {
    cancelTask(ts);
    return;
  }

  // start reaching subtask
  doffer->reachObject(ts->idSession, spooler->x() + ts->reserve[0].column * spooler->getCellWidth(), spooler->y());
}
//_________________________________________________________
//
// Slot activated after sleever has reached the winder (READY) or service zone (EMPTY)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::sleeverArrived(QString idSession)
{
  //check task session
  TaskSession *ts = getItemById<TaskSession>(idSession, m_tasks);
  if (ts == NULL) return;

  // cancel task if sleever is wrong
  Sleever *sleever = getItemById<Sleever>(ts->idAssignee, m_sleevers);
  if (sleever == NULL)
  {
    cancelTask(ts);
    return;
  }

  if (sleever->getStatus() == Sleever::READY) // sleever reached the winder and ready to put sleeve
  {
    // cancel task if winder or winder model are wrong
    Winder *winder = getItemById<Winder>(ts->idObject, m_winders);
    WinderModel *winderModel = getItemById<WinderModel>(ts->idObject, m_windersModel);
    if (winder == NULL || winderModel == NULL || winder->getStatus() == Winder::FAIL)
    {
      cancelTask(ts);
      return;
    }

    Doffer *doffer = getItemById<Doffer>(winderModel->idDoffer, m_doffers);
    if (doffer != NULL)
    {
      // calculate sleever and doffer rectangles
      QRect dofferRect;
      QRect sleeverRect;
      setLocatorRectanges(sleever, doffer, sleeverRect, dofferRect);

      // test intersection and calculate new xpos for doffer
      int newX = 0;
      bool askMove = false;
      if ((dofferRect.right() >= sleeverRect.left() && dofferRect.right() <= sleeverRect.right()) ||
          (dofferRect.left() <= sleeverRect.left() && dofferRect.right() >= sleeverRect.right()))
      {
        // set new doffer pos at the left of sleever
        newX = sleeverRect.left() - doffer->width() - (m_margin << 1);
        askMove = true;
      }
      else if ((dofferRect.left() >= sleeverRect.left() && dofferRect.left() <= sleeverRect.right()) ||
               (sleeverRect.left() <= dofferRect.left() && sleeverRect.right() >= dofferRect.right()))
      {
        // set new doffer pos at the right of sleever
        newX = sleeverRect.right() + (m_margin << 1);
        askMove = true;
      }

      if (askMove)
      {
        if (!doffer->isMoving())
        {
          // ask doffer to move left if it's idle and not moving or linked
          if (doffer->getStatus() == Doffer::IDLE ||
              testObjectId(ts, doffer->getId()))
            doffer->reachObject(doffer->getSession(), newX, doffer->y(), false);
        }

        //pause task
        ts->status = PAUSED;
        sleever->setStatus(Sleever::WAIT);
        sleever->update();
        return;
      }
    }
    // start sleever putting sleeve subtask
    sleever->putResult(idSession, ts->places, ts->places >> 1);
  }
  else if (sleever->getStatus() == Sleever::EMPTY)   // sleever reached the service zone
  {
    taskCompleted(ts->idSession);
    createLoadSleeverTask(sleever);     // create man-service task to reload sleever
  }
}
//_________________________________________________________
//
// Test object id existense in linked objects collection
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Supervisor::testObjectId(TaskSession *ts, QString idObject)
{
  if (ts == NULL) return false;
  return ts->linkedObjects.contains(idObject);
}
//_________________________________________________________
//
// test object if it's linked to any other object
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Supervisor::testObjectId(QString idObject)
{
  // Test object against every progress, paused or handle collision task
  foreach(TaskSession *ts, m_tasks)
  {
    if (ts->status == PROGRESS || ts->status == PAUSED)
    {
      if (testObjectId(ts, idObject))
          return true;
      if (ts->type == HANDLE_COLLISION &&
          (ts->idAssignee == idObject || ts->idObject == idObject))
        return true;
    }
  }
  return false;
}
//_________________________________________________________
//
// Add object id to linked obj collection
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::addObjectId(TaskSession *ts, QString idObject)
{
  // Check if the object already added
  if (testObjectId(ts, idObject))
    return;
  if (ts == NULL) return;
  ts->linkedObjects.append(idObject);
}
//_________________________________________________________
//
// Start pending subtasks for doffers or sleevers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::activateLinkedObjects(TaskSession *ts, bool isDofferLinked)
{
  if (ts == NULL) return;

  foreach(QString id, ts->linkedObjects)
  {
    if (isDofferLinked)     // doffer part
    {
      // check doffer
      Doffer *doffer = getItemById<Doffer>(id, m_doffers);
      if (doffer == NULL) continue;
      // get doffer task session with previously saved destination point
      TaskSession *dofferTask = getItemById<TaskSession>(doffer->getSession(), m_tasks);
      if (dofferTask == NULL) continue;
      //if task still in progress send doffer to the destination point
      if (dofferTask->status == PROGRESS)
        doffer->reachObject(doffer->getSession(), dofferTask->destPoint.x(), dofferTask->destPoint.y());
    }
    else  // sleever part
    {
      // check sleever
      Sleever *sleever = getItemById<Sleever>(id, m_sleevers);
      if (sleever == NULL) continue;
      // get sleever task session with previously saved destination point
      TaskSession *sleeverTask = getItemById<TaskSession>(sleever->getSession(), m_tasks);
      if (sleeverTask == NULL) continue;
      //if task still in progress send sleever to the destination point
      if (sleeverTask->status == PROGRESS)
        sleever->reachObject(sleever->getSession(), sleeverTask->destPoint.x(), sleeverTask->destPoint.y());
    }
  }

  ts->linkedObjects.clear();
}
//_________________________________________________________
//
// Start analysis for doffer vs sleever collision
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::runHandleCollisionTask(TaskSession *ts)
{
  //qDebug() << "HANDLE_COLLISION" << ts->type << ts->idAssignee << ts->idObject << ts->status << ts->idSession ;

  // cancel task if doffer or sleever are wrong
  Doffer *doffer = getItemById<Doffer>(ts->idAssignee, m_doffers);
  Sleever *sleever = getItemById<Sleever>(ts->idObject, m_sleevers);
  if (doffer == NULL || sleever == NULL)
  {
    //qDebug() << "Cancelling";
    cancelTask(ts);
    return;
  }

  // pause task if doffer or sleever are moving
  if (doffer->isMoving() || sleever->isMoving())
  {
    //qDebug() << "Wait for " << doffer->getId() << doffer->isMoving() << sleever->getId() << sleever->isMoving() << "Pausing" << ts->idSession;
    ts->status = PAUSED;
    return;
  }

  // link secondary object to the primary one
  ts->status = PROGRESS;
  bool dofferPriority = ts->places;  // using places to store the doffer priority
  if (dofferPriority)
  {
    // get doffer task session
    TaskSession *dts = getItemById<TaskSession>(doffer->getSession(), m_tasks);
    if (dts != NULL)
    {
      //qDebug() << "Linking " << sleever->getId() << "to" << doffer->getId();
      // link sleever to doffer
      addObjectId(dts, sleever->getId());
      // if doffer was moving send it to the destination
      if (ts->waitDoffer)
      {
        doffer->reachObject(doffer->getSession(), dts->destPoint.x(), dts->destPoint.y());
        //qDebug() << "Send " << doffer->getId();
      }
    }
    else
    {
      // cancel collision due to wrong doffer task session
      //qDebug() << "Invalid DOFFER session. Cancelling collision.";
      TaskSession *sleeverTask = getItemById<TaskSession>(sleever->getSession(), m_tasks);
      // if sleever was moving send it to the destination
      if (sleeverTask != NULL && sleeverTask->status == PROGRESS)
      {
        sleever->reachObject(sleever->getSession(), sleeverTask->destPoint.x(), sleeverTask->destPoint.y());
        //qDebug() << "Send back " << sleever->getId();
      }
    }
  }
  else
  {
    // get sleever task session
    TaskSession *sts = getItemById<TaskSession>(sleever->getSession(), m_tasks);
    if (sts != NULL)
    {
      //qDebug() << "Linking " << doffer->getId() << "to" << sleever->getId();
      // link doffer to sleever
      addObjectId(sts, doffer->getId());
      // if sleever was moving send it to the destination
      if (ts->waitSleever)
      {
        sleever->reachObject(sleever->getSession(), sts->destPoint.x(), sts->destPoint.y());
        //qDebug() << "Send " << sleever->getId();
      }
    }
    else
    {
      // cancel collision due to wrong sleever task session
      //qDebug() << "Invalid SLEEVER session. Cancelling collision.";
      TaskSession *dofferTask = getItemById<TaskSession>(doffer->getSession(), m_tasks);
      // if doffer was moving send it to the destination
      if (dofferTask != NULL && dofferTask->status == PROGRESS)
      {
        doffer->reachObject(doffer->getSession(), dofferTask->destPoint.x(), dofferTask->destPoint.y());
        //qDebug() << "Send back " << doffer->getId();
      }
    }
  }

  taskCompleted(ts->getId());
}
//_________________________________________________________
//
// Calculate doffer sleever rectangles. Pays attention to the brake distance and constant margins
// Useful for further intersection analysis
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Supervisor::setLocatorRectanges(Locator *priObject, Locator *secObject,
                             QRect &primaryRect, QRect &secondaryRect)
{
  // count brake distance
  int priExtra = priObject->getBrakeDistance();
  int secExtra = secObject->getBrakeDistance();
  // moving flags
  bool priMoving = priObject->isMoving();
  bool secMoving = secObject->isMoving();
  // moving to the right flag
  bool priMovingRight = priMoving && priObject->getDestX() > priObject->x();;
  bool secMovingRight = secMoving && secObject->getDestX() > secObject->x();

  // create rectangles with margins
  primaryRect.setTopLeft(QPoint(priObject->x() - m_margin, priObject->y()));
  primaryRect.setSize(QSize(priObject->width() + (m_margin << 1), priObject->height()));
  secondaryRect.setTopLeft(QPoint(secObject->x() - m_margin, secObject->y()));
  secondaryRect.setSize(QSize(secObject->width() + (m_margin << 1), secObject->height()));

  // if objects are moving append braking distance before it
  if (priMoving != secMoving || priMovingRight != secMovingRight)
  {
    if (priMoving)
    {
      if (priMovingRight)
        primaryRect.setWidth(primaryRect.width() + priExtra);
      else
        primaryRect.setLeft(primaryRect.left() - priExtra);
      //qDebug() << priObject->getId() << primaryRect;
    }

    if (secMoving)
    {
      if (secMovingRight)
        secondaryRect.setWidth(secondaryRect.width() + secExtra);
      else
        secondaryRect.setLeft(secondaryRect.left() - secExtra);
      //qDebug() << secObject->getId() << secondaryRect;
    }
  }

}
//_________________________________________________________
//
// Handles collision between doffer and sleever. If collision detected
// both objects stop. After that the object with less priority links to another one.
// Linked objects moves together until the host task session completed or cancelled
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::processCollision(Doffer *doffer, Sleever *sleever, bool isDofferLead, int delta)
{
  Q_UNUSED(delta)

  Locator *priObject = isDofferLead ? (Locator *)doffer : (Locator *)sleever;
  Locator *secObject = isDofferLead ? (Locator *)sleever : (Locator *)doffer;
  QRect primaryRect;
  QRect secondaryRect;

  // calculate object rectangles
  setLocatorRectanges(priObject, secObject, primaryRect, secondaryRect);
  if ((primaryRect.left() < secondaryRect.right() && primaryRect.right() > secondaryRect.left()))
  {
    // collision detected. Compairing priorities
    int dofferPrio = doffer->getStatus() == Doffer::BUSY ? 0 : (doffer->getStatus() == Doffer::DELIVER ? 2 : 3);
    int sleeverPrio = sleever->getStatus() == Sleever::BUSY ? 0 : (sleever->getStatus() == Sleever::READY ? 1 : 4);
    if (dofferPrio != sleeverPrio)
    {
      // stop both objects
      bool waitDoffer = false;
      bool waitSleever = false;
      TaskSession *dts = getItemById<TaskSession>(doffer->getSession(), m_tasks);
      if (dts != NULL)
      {
        // save destination point in the task session
        dts->destPoint.setX(doffer->getDestX());
        dts->destPoint.setY(doffer->getDestY());
        // stop the object
        doffer->stopMoving(false);
        waitDoffer = true;
      }
      TaskSession *sts = getItemById<TaskSession>(sleever->getSession(), m_tasks);
      if (sts != NULL)
      {
        // save destination point in the task session
        sts->destPoint.setX(sleever->getDestX());
        sts->destPoint.setY(sleever->getDestY());
        // stop the object
        sleever->stopMoving(false);
        waitSleever = true;
      }

      //create task to handle collision
      handleCollision(doffer, sleever, dofferPrio < sleeverPrio, waitDoffer, waitSleever);
    }
  }
}
//_________________________________________________________
//
// Slot activated after doffer has been moved. Here supervisor tests possible collisions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::dofferMoved(QString idDoffer, QPoint newPos, int delta)
{
  Q_UNUSED(newPos)
  // check doffer and moving distance
  Doffer *doffer = getItemById(idDoffer, m_doffers);
  if (doffer == NULL || delta == 0) return;
  // check task session
  TaskSession *ts = getItemById<TaskSession>(doffer->getSession(), m_tasks);
  if (ts == NULL) return;

  // calculate doffer packed state
  bool dofferPacked = (doffer->getStatus() == Doffer::IDLE ||
                       doffer->getStatus() == Doffer::WAIT ||
                       doffer->getStatus() == Doffer::WAITWINDER ||
                       doffer->getStatus() == Doffer::READY);

  // check sleevers for collision
  foreach(Sleever *sleever, m_sleevers)
  {
    // if sleever already linked move it
    if (testObjectId(ts, sleever->getId()))
    {
      sleever->move(sleever->x() + delta, sleever->y());
      continue;
    }

    //if both are packed continue
    bool sleeverPacked = (sleever->getStatus() == Sleever::IDLE ||
                          sleever->getStatus() == Sleever::PREPARING ||
                          sleever->getStatus() == Sleever::READY ||
                          sleever->getStatus() == Sleever::EMPTY);
    if (dofferPacked && sleeverPacked)  continue;

    // check intersection and handle possible collisions
    processCollision(doffer, sleever, true, delta);
  }
}
//_________________________________________________________
//
// Slot activated after sleever has been moved. Here supervisor tests possible collisions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::sleeverMoved(QString idSleever, QPoint newPos, int delta)
{
  Q_UNUSED(newPos)
  // check doffer and moving distance
  Sleever *sleever = getItemById(idSleever, m_sleevers);
  if (sleever == NULL || delta == 0) return;
  // check task session
  TaskSession *ts = getItemById<TaskSession>(sleever->getSession(), m_tasks);
  if (ts == NULL) return;

  // calculate sleever packed state
  bool sleeverPacked = (sleever->getStatus() == Sleever::IDLE ||
                        sleever->getStatus() == Sleever::PREPARING ||
                        sleever->getStatus() == Sleever::READY ||
                        sleever->getStatus() == Sleever::EMPTY);


  //check doffers for collision
  foreach(Doffer *doffer, m_doffers)
  {
    //if doffer already linked move it with sleever
    if (testObjectId(ts, doffer->getId()))
    {
      doffer->move(doffer->x() + delta, doffer->y());
      continue;
    }

    //if both are packed continue
    bool dofferPacked = (doffer->getStatus() == Doffer::IDLE ||
                         doffer->getStatus() == Doffer::WAIT ||
                         doffer->getStatus() == Doffer::WAITWINDER ||
                         doffer->getStatus() == Doffer::READY);
    if (dofferPacked && sleeverPacked)  continue;

    // check intersection and handle possible collisions
    processCollision(doffer, sleever, false, delta);
  }
}
//_________________________________________________________
//
// Reorder spooler models and set the current one to the end of list,
// thus reservation will take place here as the last place
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::moveSpoolerToTail(QString idSpooler)
{
  // check spooler model
  SpoolerModel *model = getItemById<SpoolerModel>(idSpooler, m_spoolersModel);
  if (model == NULL) return;

  // get other spooler ids having the same doffer as current
  QList<QString> ids;
  foreach(SpoolerModel *it, m_spoolersModel)
  {
    if (it->idDoffer == model->idDoffer)
      ids.append(it->idSpooler);
  }

  //loop through all spooler models
  QList<SpoolerModel *>::iterator item = m_spoolersModel.end();
  for(QList<SpoolerModel *>::iterator it = m_spoolersModel.begin(); it != m_spoolersModel.end(); ++it)
  {
    if ((*it)->idDoffer == model->idDoffer)
    {
      // if it is an idSpooler save iterator
      if ((*it)->getId() == idSpooler)
      {
        item = it;
        continue;
      }
      // it is a spooler from the same group as idSpooler
      if (item != m_spoolersModel.end())
      {
        // assign the pointer so the it one will shift to the left
        *item = *it;
        // assign iterator to continue shifting
        item = it;
      }
    }
  }

  // set the last one to the idSpooler model pointer
  if (item != m_spoolersModel.end())
    *item = model;
}
//_________________________________________________________
//
// Convert millimeters to pixels using aspect ratio value
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int Supervisor::toPixels(int sourceValue)
{
  return round(sourceValue * m_aspectRatio);
}
//_________________________________________________________
//
// Convert pixels to millimeters using aspect ratio value
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Supervisor::toMillimeters(int sourceValue)
{
  if (m_aspectRatio == 0) return 0;
  return round(sourceValue * 1 / m_aspectRatio);
}
//_________________________________________________________
//
// Save external width value as base for aspect ratio calculation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::SetWholeWidthPixels(int width)
{
  // return if supervisor is working
  if (m_task_timer != 0) return;
  m_wholeWidthPixels = width;
}
//_________________________________________________________
//
// Calculate the aspect ratio
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::countAspectRatio(int space)
{
  // should not calculate if supervisor is working
  if (m_task_timer != 0) return;
  int wholeWidthMillimeters = m_config.serviceZoneWidth;    //initial value

  // gather all widths for winders and service zones
  int counter = 0;
  QString prevKey = "";
  QString currentKey = "";
  foreach(WinderModel *it, m_windersModel)
  {
    currentKey = it->idDoffer;
    if (prevKey != currentKey)
    {
      if (counter > 0)
        wholeWidthMillimeters += m_config.serviceZoneWidth;

      // new winder group start
      wholeWidthMillimeters += m_config.serviceZoneWidth;

      // reset keys
      prevKey = currentKey;
      counter = 0;
    }

    // add winder width
    wholeWidthMillimeters += it->width + m_config.spaceBetweenWinders;
    counter++;
  }
  if (counter > 0)
    wholeWidthMillimeters += m_config.serviceZoneWidth;

  if (wholeWidthMillimeters <= (space << 1))
    m_aspectRatio = 0.0;
  else
  {
    // count aspect ratio from the whole width in millimeters and whole window windth in pixels
    m_aspectRatio = (float)m_wholeWidthPixels / (wholeWidthMillimeters - (space << 1));
    // 0.07 is the minimal allowed aspect ratio
    if (m_aspectRatio < 0.07)
      m_aspectRatio = 0.07;
  }
}
//_________________________________________________________
//
// Check group of winders if it's ready to call the doffer
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::setGroupBobbinsReady(QString idWinder)
{
  // Find the winder model
  WinderModel *model = getItemById<WinderModel>(idWinder, m_windersModel);
  if (model == NULL) return;

  // Gather all winder ids from the same doffer group
  QVector<QString> groupIds;
  foreach(WinderModel *it, m_windersModel)
  {
    if (it->idDoffer == model->idDoffer)
      groupIds.append(it->getId());
  }

  // Verify cutedge flag for all group items
  foreach(Winder *it, m_winders)
  {
    if (!groupIds.contains(it->getId()))
      continue;
    // return if current winder has not have cutted bobbins
    if (it->getCutEdgeMode())
      return;
  }

  // If all are ready call doffer for each
  foreach(Winder *it, m_winders)
  {
    if (groupIds.contains(it->getId()))
      bobbinsReady(it->getId());
  }
}
//_________________________________________________________
//
// Update logger item field
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Supervisor::updateLogger(QString idObject, Logger::FieldNames field)
{
  updateLoggerItem(idObject, field);
}

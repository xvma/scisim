#include "supervisor.h"
#include "man.h"

const int timerResolution = 70;   // default tick latency for timers
//_________________________________________________________
//
// Object constructor. Set parameters from model, common styles and sizes
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ManService::ManService(ManServiceModel &model, QWidget *parent /*=0*/) :
  QFrame(parent)
{
  setLineWidth(1);
  setFrameStyle(NoFrame | Plain);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  // assume man sizes as 1000 mm x 300 mm
  Supervisor *supervisor = (Supervisor *)parent;
  int controlWidth = supervisor->toPixels(1000);
  int controlHeight = supervisor->toPixels(300);
  resize(controlWidth, controlHeight);

  // init params from database model
  m_id = model.idMan;
  m_speed = supervisor->toPixels(model.speed);
  m_timeStartWinder = model.timeStartWinder;
  m_timeRotateSpooler = model.timeRotateSpooler;
  m_timeChangeSpooler = model.timeChangeSpooler;
  m_timeLoadSleever = model.timeLoadSleever;
  m_timeCutEdge = model.timeCutEdge;

  // set idle state
  m_status = IDLE;
  m_timeLeft = 0;
  m_timeReach = 0;

  // init timer ids
  m_movement_timer = 0;
  m_startWinder_timer = 0;
  m_rotateSpooler_timer = 0;
  m_changeSpooler_timer = 0;
  m_loadSleever_timer = 0;
  m_cutEdge_timer = 0;
}
//_________________________________________________________

ManService::~ManService()
{

}
//_________________________________________________________
//
// Draw the control content according to its state
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::paintEvent(QPaintEvent *pe)
{
  Q_UNUSED(pe)

  QPainter painter;
  QRect rct = rect();

  // open drawing context
  painter.begin(this);
  painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));

  // draw background according to the state
  switch(m_status)
  {
    case READY:
      painter.fillRect(rct, QBrush(QColor(95, 178, 150), Qt::SolidPattern));
      break;
    case BUSY:
      painter.fillRect(rct, QBrush(Qt::black, Qt::Dense5Pattern));
      break;
    case IDLE:
    default:
      painter.fillRect(rct, QBrush(Qt::gray, Qt::SolidPattern));
      break;
  }

  // draw caption
  painter.drawText(rct.left() + 5, 15, m_id);

  // close drawing context
  painter.end();
}
//_________________________________________________________
//
// Stop man movement. Method kills movement timer
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::stopMoving(bool doEmit /*= true*/)
{
  // check if we're moving
  if (m_movement_timer == 0) return;
  // kill timer
  killTimer(m_movement_timer);
  m_movement_timer = 0;
  // notify supervisor if needed
  if (doEmit)
    emit goalReached(m_session);
}
//_________________________________________________________
//
// Timer handler event
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::timerEvent(QTimerEvent* te)
{
  // count params for movement process to move the widget to new pos
  int delta = 0;
  if (te->timerId() == m_movement_timer)
  {
    // decreasing time left until 0
    m_timeLeft = m_timeLeft <= timerResolution ? 0 : (m_timeLeft - timerResolution);
    // calculate the new distance
    delta = (m_timeReach - m_timeLeft) * m_speed / 1000;
    // move widget
    move(m_destX > m_startX ? m_startX + delta : m_startX - delta, pos().y());
    // if timer expired stop moving
    if (m_timeLeft == 0)
      stopMoving();
  }
  // all other timers are just a kind of duration
  // all of them should idle the manservice after timer killing
  // and notify supervisor via task completion
  else if (te->timerId() == m_startWinder_timer ||
           te->timerId() == m_rotateSpooler_timer ||
           te->timerId() == m_changeSpooler_timer ||
           te->timerId() == m_loadSleever_timer ||
           te->timerId() == m_cutEdge_timer)
  {
    killTimer(te->timerId());
    m_startWinder_timer = 0;
    m_rotateSpooler_timer = 0;
    m_changeSpooler_timer = 0;
    m_loadSleever_timer = 0;
    m_cutEdge_timer = 0;
    setStatus(IDLE);
    update();
    emit taskCompleted(m_session);
  }
}
//_________________________________________________________
//
// Private task machine starter
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::startMachine(OperFunc operation)
{
  // set necessary timer and start
  // other timer counters should be set to 0
  switch(operation)
  {
    case REACH:
      {
        // calculate distance to move and time duration for this
        int distance = abs(m_destX - m_startX);
        m_timeLeft = 1000 * distance / m_speed;
        m_timeReach = m_timeLeft;
        // start the timer
        m_startWinder_timer = 0;
        m_rotateSpooler_timer = 0;
        m_changeSpooler_timer = 0;
        m_loadSleever_timer = 0;
        m_cutEdge_timer = 0;
        m_movement_timer = startTimer(timerResolution);
        break;
      }
    // all other tasks are timer actions in specific duration
    case START_WINDER:
      setStatus(BUSY);
      update();
      m_timeLeft = m_timeStartWinder;

      m_rotateSpooler_timer = 0;
      m_changeSpooler_timer = 0;
      m_loadSleever_timer = 0;
      m_movement_timer = 0;
      m_cutEdge_timer = 0;
      m_startWinder_timer = startTimer(m_timeStartWinder);
      break;
    case CUT_EDGE:
      setStatus(BUSY);
      update();
      m_timeLeft = m_timeCutEdge;

      m_rotateSpooler_timer = 0;
      m_changeSpooler_timer = 0;
      m_loadSleever_timer = 0;
      m_movement_timer = 0;
      m_startWinder_timer = 0;
      m_cutEdge_timer = startTimer(m_timeCutEdge);
      break;
    case ROTATE_SPOOLER:
      setStatus(BUSY);
      update();
      m_timeLeft = m_timeRotateSpooler;

      m_changeSpooler_timer = 0;
      m_loadSleever_timer = 0;
      m_movement_timer = 0;
      m_startWinder_timer = 0;
      m_cutEdge_timer = 0;
      m_rotateSpooler_timer = startTimer(m_timeRotateSpooler);
      break;
    case CHANGE_SPOOLER:
      setStatus(BUSY);
      update();
      m_timeLeft = m_timeChangeSpooler;

      m_loadSleever_timer = 0;
      m_movement_timer = 0;
      m_startWinder_timer = 0;
      m_rotateSpooler_timer = 0;
      m_cutEdge_timer = 0;
      m_changeSpooler_timer = startTimer(m_timeChangeSpooler);
      break;
    case LOAD_SLEEVER:
      setStatus(BUSY);
      update();
      m_timeLeft = m_timeLoadSleever;

      m_movement_timer = 0;
      m_startWinder_timer = 0;
      m_rotateSpooler_timer = 0;
      m_changeSpooler_timer = 0;
      m_cutEdge_timer = 0;
      m_loadSleever_timer = startTimer(m_timeLoadSleever);
      break;
  }
}
//_________________________________________________________
//
// Public status setting method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::setStatus(Status state)
{
  m_status = state;
}
//_________________________________________________________
//
// Public action method for reaching
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::reachObject(QString idSession, int x, int y)
{
  // man should not be busy
  if (m_speed == 0 || m_status == BUSY) return;
  // stop if we're moving
  if (m_movement_timer != 0)
  {
    killTimer(m_movement_timer);
    m_movement_timer = 0;
  }
  // set destination pos, init session and start the state machine
  m_startX = pos().x();
  m_session = idSession;
  setDestPos(x, y);
  startMachine(REACH);
}
//_________________________________________________________
//
// Public action method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::loadSleever(QString idSession)
{
  // man should be ready
  if (m_status != READY) return;
  m_session = idSession;
  startMachine(LOAD_SLEEVER);
}
//_________________________________________________________
//
// Public action method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::changeSpooler(QString idSession)
{
  // man should be ready
  if (m_status != READY) return;
  m_session = idSession;
  startMachine(CHANGE_SPOOLER);
}
//_________________________________________________________
//
// Public action method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::rotateSpooler(QString idSession)
{
  // man should be ready
  if (m_status != READY) return;
  m_session = idSession;
  startMachine(ROTATE_SPOOLER);
}
//_________________________________________________________
//
// Public action method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::startWinder(QString idSession)
{
  // man should be ready
  if (m_status != READY) return;
  m_session = idSession;
  startMachine(START_WINDER);
}
//_________________________________________________________
//
// Public action method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::cutEdgeOnWinder(QString idSession)
{
  // man should be ready
  if (m_status != READY) return;
  m_session = idSession;
  startMachine(CUT_EDGE);
}
//_________________________________________________________
//
// Public set destination position method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ManService::setDestPos(int x, int y)
{
  m_destX = x;
  m_destY = y;
}






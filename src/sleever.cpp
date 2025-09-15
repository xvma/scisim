#include "supervisor.h"
#include "winder.h"
#include "sleever.h"

const int timerResolution = 70;     // default tick latency for timers

//_________________________________________________________
//
// Object constructor. Set parameters from model, count max brake distance as extraWidth
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sleever::Sleever(SleeverModel &model, QWidget *parent /*=0*/) :
  Locator(parent)
{
  // count extra width and drawing control sizes
  Supervisor *supervisor = (Supervisor *)parent;
  extraWidth = supervisor->toPixels(model.speed * model.speed / (model.acceleration << 1));
  int controlWidth = supervisor->toPixels(model.width);
  int controlHeight = controlWidth >> 2;
  resize(controlWidth, controlHeight);

  // init params from database model
  m_timePutDown = model.timePutDown;
  m_timePrepare = model.prepare;
  m_id = model.idSleever;
  m_speed = supervisor->toPixels(model.speed);
  m_accel = supervisor->toPixels(model.acceleration);
  setInventory(model.sleeveSlots, model.rings);

  // set idle state
  m_status = IDLE;
  m_anim = NULL;

  m_putres_timer = 0;
  m_prepare_timer = 0;
}
//_________________________________________________________
//
// Object destructor. Destroy animator object if exists
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sleever::~Sleever()
{
  destroyAnimator();
}
//_________________________________________________________
//
// Draw the control content according to its state
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sleever::paintEvent(QPaintEvent *pe)
{
  Q_UNUSED(pe)

  // get sizes and dimentions
  QPainter painter;
  QRect srcRect = rect();
  QRect rct(srcRect.left(), srcRect.top(), srcRect.width(), srcRect.height());

  // open drawing context
  painter.begin(this);
  painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));

  // draw background according to its state
  switch(m_status)
  {
    case READY:
    case WAIT:
      painter.fillRect(rct, QBrush(QColor(95, 178, 150), Qt::SolidPattern));
      break;
    case BUSY:
      painter.fillRect(rct, QBrush(Qt::black, Qt::Dense5Pattern));
      break;
    case EMPTY:
      painter.fillRect(rct, QBrush(QColor(255, 97, 135), Qt::SolidPattern));
      break;
    case PREPARING:
      painter.fillRect(rct, QBrush(QColor(115, 174, 206), Qt::SolidPattern));
      break;
    case IDLE:
    default:
      painter.fillRect(rct, QBrush(Qt::gray, Qt::SolidPattern));
      break;
  }

  // draw caption
  painter.drawText(rct.left() + 5, 15, QString("%1: S=%2, R=%3")
                   .arg(m_id)
                   .arg(m_sleeves)
                   .arg(m_rings));

  // close drawing context
  painter.end();
}
//_________________________________________________________
//
// Event handler for timer counters
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sleever::timerEvent(QTimerEvent* te)
{
  // calling parent timer handler
  Locator::timerEvent(te);
  // timer counter for putting sleeve animating process
  int delta = 0;
  if (te->timerId() == m_putres_timer)
  {
    // count new coordinate for the animation depending on time left value
    m_timeLeft = m_timeLeft <= timerResolution ? 0 : (m_timeLeft - timerResolution);
    if (m_timeReach != 0)
      delta = (m_timeReach - m_timeLeft) * (y() - m_anim->height() + height() - m_destY) / m_timeReach;
    // move animator widget
    m_anim->move(pos().x(), y() - m_anim->height() + height() - delta);
    // if timer is over
    if (m_timeLeft == 0)
    {
      // kill timer and destroy animator
      killTimer(te->timerId());
      m_putres_timer = 0;
      destroyAnimator();
      // update busy counter for the object
      updateLoggerItem(m_id, Logger::TIME_BUSY);
      // change status
      setStatus(m_sleeves > 0 ? PREPARING : EMPTY);
      update();
      // notify supervisor
      emit taskCompleted(m_session);
      if (m_status == EMPTY)
        // notify supervisor if sleever is empty
        emit emptySleever(m_id);
      else
        // switch machine onto the next state
        startMachine(PREPARE);
    }
  }
  // timer handler for preparing process
  else if (te->timerId() == m_prepare_timer)
  {
    // kill timer and set sleever as idle
    killTimer(te->timerId());
    m_prepare_timer = 0;
    setStatus(IDLE);
    update();
  }
}
//_________________________________________________________
//
// Private task machine starter
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sleever::startMachine(OperFunc operation)
{
  // set necessary timer and start
  // other timer counters should be set to 0
  switch(operation)
  {
    case REACH:             // this action maps to the Locator object
      m_putres_timer = 0;
      startMoving();
      break;
    case PUTRES:            // put sleeve action
      setStatus(BUSY);
      update();
      m_timeLeft = m_timePutDown;
      m_timeReach = m_timeLeft;
      m_movement_timer = 0;
      m_prepare_timer = 0;
      m_putres_timer = startTimer(timerResolution);
      break;
    case PREPARE:         // preparing action
      m_timeLeft = m_timePrepare;
      m_putres_timer = 0;
      m_prepare_timer = startTimer(m_timePrepare);
      break;
    default:
      break;
  }
}
//_________________________________________________________
//
// Public status setting method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sleever::setStatus(Status state)
{
  m_status = state;
}
//_________________________________________________________
//
// Public action method for putting sleeve
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sleever::putResult(QString idSession, int sleeves, int rings)
{
  // sleever should be ready
  if (m_status != READY) return;

  // decrease storage amount for sleeves and rings
  m_sleeves -= sleeves;
  m_rings -= rings;
  // set session
  m_session = idSession;
  // create animator
  createAnimator(sleeves == 1 ? Animator::SLEEVE1 : Animator::SLEEVE2);

  // update idle counter for the object
  updateLoggerItem(m_id, Logger::TIME_IDLE);
  // call state machine starter
  startMachine(PUTRES);
}
//_________________________________________________________
//
// Public method to set storage inventory amount
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sleever::setInventory(int sleeves, int rings)
{
  m_sleeves = sleeves;
  m_rings = rings;
}
//_________________________________________________________
//
// Create animator widget
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sleever::createAnimator(Animator::Type type)
{
  // if already exists, destroy animator
  destroyAnimator();
  // create widget and set its position
  m_anim = new Animator(type, m_bobbinsSize.width(), m_bobbinsSize.height(), parentWidget());
  m_anim->move(m_destX, y() - m_anim->height() + height());
  // show widget
  m_anim->show();
}
//_________________________________________________________
//
// Destroy animator widget
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sleever::destroyAnimator()
{
  // if animation exists
  if (m_anim != NULL)
  {
    // destroy it
    m_anim->hide();
    delete m_anim;
  }
  m_anim = NULL;
}
//_________________________________________________________
//
// Public action method for reaching
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sleever::reachObject(QString idSession, int x, int y, bool doEmit /* = true*/)
{
  // sleever should not be busy
  if (m_status == BUSY) return;
  // call locator method to reach a goal
  Locator::reachObject(idSession, x, y, doEmit);
}

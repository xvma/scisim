#include "supervisor.h"
#include "winder.h"
#include "doffer.h"

const int timerResolution = 70;   // default tick latency for get & put doffer timers
//_________________________________________________________
//
// Object constructor. Set parameters from model, count max brake distance as extraWidth
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Doffer::Doffer(DofferModel &model, QWidget *parent /*=0*/) :
  Locator(parent)
{
  // count extra width and drawing control height
  Supervisor *supervisor = (Supervisor *)parent;
  extraWidth = supervisor->toPixels(model.speed * model.speed / (model.acceleration << 1));
  int controlWidth = supervisor->toPixels(model.width);
  controlHeight = controlWidth >> 2;
  resize(controlWidth, 3 * controlHeight);  // set the real height greater than control's one to be able to render bobbing

  // init params from database model
  m_timeGetIn = model.timeGetIn;
  m_timePutDown = model.timePutDown;
  m_id = model.idDoffer;
  m_speed = supervisor->toPixels(model.speed);
  m_accel = supervisor->toPixels(model.acceleration);
  m_amount = 0;

  // set idle state
  m_status = IDLE;
  m_anim = NULL;

  m_getres_timer = 0;
  m_putres_timer = 0;
}
//_________________________________________________________
//
// Object destructor. Destroy animator object if exists
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Doffer::~Doffer()
{
  destroyAnimator();
}
//_________________________________________________________
//
// Draw the control content according to its state
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Doffer::paintEvent(QPaintEvent *pe)
{
  Q_UNUSED(pe)

  // count beam sizes
  QPainter painter;
  QRect srcRect = rect();
  QRect rct(srcRect.left(), srcRect.top(), srcRect.width(), controlHeight);
  QRect beamLeft(QPoint(srcRect.left(), srcRect.top()), m_bobbinsSize);

  // open drawing context
  painter.begin(this);
  painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));

  // draw background and bobbins if its delivery state
  switch(m_status)
  {
    case READY:
    case WAIT:
      painter.fillRect(rct, QBrush(QColor(95, 178, 150), Qt::SolidPattern));
      break;
    case BUSY:
      painter.fillRect(rct, QBrush(Qt::black, Qt::Dense5Pattern));
      break;
    case DELIVER:
      painter.fillRect(rct, QBrush(QColor(95, 178, 150), Qt::SolidPattern));
      Winder::drawSpools(painter, beamLeft, m_amount == 1);
      break;
    case IDLE:
    case WAITWINDER:
    default:
      painter.fillRect(rct, QBrush(Qt::gray, Qt::SolidPattern));
      break;
  }

  // draw the caption
  painter.drawText(rct.left() + 20, 15, m_id);

  // close drawing context
  painter.end();
}
//_________________________________________________________
//
// Event handler for timer counters
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Doffer::timerEvent(QTimerEvent* te)
{
  // calling parent timer handler
  Locator::timerEvent(te);
  // timer counter for getting bobbins animating process
  int delta = 0;
  if (te->timerId() == m_getres_timer)
  {
    // count new coordinate for the animation depending on time left value
    m_timeLeft = m_timeLeft <= timerResolution ? 0 : (m_timeLeft - timerResolution);
    if (m_timeReach != 0)
      delta = (m_timeReach - m_timeLeft) * (pos().y() - m_destY) / m_timeReach;
    // move animator widget
    m_anim->move(pos().x(), m_destY + delta);
    // if timer is over
    if (m_timeLeft == 0)
    {
      // kill timer and destroy animator
      killTimer(te->timerId());
      m_getres_timer = 0;
      destroyAnimator();
      // update busy counter for the object
      updateLoggerItem(m_id, Logger::TIME_BUSY);
      // change status
      setStatus(DELIVER);
      update();
      // notify supervisor
      emit bobAboard(m_session);
    }
  }
  // timer counter for putting bobbins animating process
  else if (te->timerId() == m_putres_timer)
  {
    // count new coordinate for the animation depending on time left value
    m_timeLeft = m_timeLeft <= timerResolution ? 0 : (m_timeLeft - timerResolution);
    if (m_timeReach != 0 && m_amount > 0)
      delta = (m_timeReach - m_timeLeft) * ((m_destY - pos().y()) / m_amount) / m_timeReach;
    // move animator widget
    m_anim->move(pos().x(), pos().y() + delta);
    // if timer is over
    if (m_timeLeft == 0)
    {
      // kill timer and destroy animator
      killTimer(te->timerId());
      m_putres_timer = 0;
      destroyAnimator();
      // update busy counter for the object
      updateLoggerItem(m_id, Logger::TIME_BUSY);
      // change status depending on amount left
      if (m_amount > 0)
        m_amount--;
      // if a bobbin has left
      if (m_amount > 0)
      {
        // continue delivery
        setStatus(DELIVER);
        // notify supervisor that first bobbin has been delivered
        emit bobPlaced(m_session);
      }
      else
      {
        // set status idle
        setStatus(IDLE);
        // notify supervisor about task completion
        emit taskCompleted(m_session);
      }
      // refresh widget after status change
      update();
    }
  }
}
//_________________________________________________________
//
// Private task machine starter
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Doffer::startMachine(OperFunc operation)
{
  // set necessary timer and start
  // other timer counters should be set to 0
  switch(operation)
  {
    case REACH:   // this action maps to the Locator object
      m_putres_timer = 0;
      m_getres_timer = 0;
      startMoving();
      break;
    case GETRES:  // get bobbins onboard action
      setStatus(BUSY);
      update();
      m_timeLeft = m_timeGetIn;
      m_timeReach = m_timeLeft;
      m_movement_timer = 0;
      m_putres_timer = 0;
      m_getres_timer = startTimer(timerResolution);
      break;
    case PUTRES:  // put bobbins action
      setStatus(BUSY);
      update();
      m_timeLeft = m_timePutDown;
      m_timeReach = m_timeLeft;
      m_movement_timer = 0;
      m_getres_timer = 0;
      m_putres_timer = startTimer(timerResolution);
      break;
    default:
      break;
  }
}
//_________________________________________________________
//
// Public status setting method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Doffer::setStatus(Status state)
{
  m_status = state;
}
//_________________________________________________________
//
// Public action method for getting bobbins
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Doffer::getResult(QString idSession, int amount)
{
  // doffer should be in ready state
  if (m_status != READY) return;

  // set amount to deliver
  m_amount = amount;
  // assign supervisor session
  m_session = idSession;
  // create animator widget
  createAnimator(m_amount == 1 ? Animator::SPOOL1 : Animator::SPOOL2, m_destY);

  // update idle counter for the object
  updateLoggerItem(m_id, Logger::TIME_IDLE);
  // call machine starter
  startMachine(GETRES);
}
//_________________________________________________________
//
// Public action method for putting bobbins
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Doffer::putResult(QString idSession, int row, int column, int amount)
{
  Q_UNUSED(row)
  Q_UNUSED(column)
  // doffer should be in delivery state
  if (m_status != DELIVER) return;

  // set amount to put (usually 1)
  m_amount = amount;
  // assign supervisor session
  m_session = idSession;
  // create animator widget
  createAnimator(m_amount == 1 ? Animator::SPOOL1 : Animator::SPOOL2, y());

  // update idle counter for the object
  updateLoggerItem(m_id, Logger::TIME_IDLE);
  // call machine starter
  startMachine(PUTRES);
}
//_________________________________________________________
//
// Create animator widget
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Doffer::createAnimator(Animator::Type type, int yPos)
{
  // if already exists, destroy animator
  destroyAnimator();
  // create widget and set its position
  m_anim = new Animator(type, m_bobbinsSize.width(), m_bobbinsSize.height(), parentWidget());
  m_anim->move(m_destX, yPos);
  // show widget
  m_anim->show();
}
//_________________________________________________________
//
// Destroy animator widget
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Doffer::destroyAnimator()
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
void Doffer::reachObject(QString idSession, int x, int y, bool doEmit /* = true*/)
{
  // doffer should not be busy
  if (m_status == BUSY) return;
  // call locator method to reach a goal
  Locator::reachObject(idSession, x, y, doEmit);
}





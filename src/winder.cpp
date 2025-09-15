#include "supervisor.h"
#include "winder.h"

const int timerResolution = 100;  // default tick latency for timers
//_________________________________________________________
//
// Object constructor. Set parameters from model, common styles and sizes
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Winder::Winder(WinderModel &model, int timeCoefficient, QWidget *parent /*=0*/) :
  QFrame(parent)
{
  setLineWidth(1);
  setFrameStyle(NoFrame | Plain);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  //calculate and set widget sizes from model
  Supervisor *supervisor = (Supervisor *)parent;
  int winderWidth = supervisor->toPixels(model.width);
  int winderHeight = 1.5 * winderWidth;
  resize(winderWidth, winderHeight);

  // init params from database model
  m_timeWind = model.timeWind;
  m_timeAlert = model.timeAlert;
  m_timeExchange = model.timeExchange;
  m_halfMode = model.isHalfMode;
  m_id = model.idWinder;

  // set winder status
  m_status = EMPTY;
  setCutEdgeMode(true);       // this means that after first bobbins are ready there'll be no call for doffer
                              // but supervisor notification instead. Supervisor will send a man to cut bobbins
  m_readiness = 0;
  m_timeLeft = m_timeWind;
  m_timeCoefficient = timeCoefficient;

  // init timer ids
  m_wind_timer = 0;
  m_rotate_timer = 0;
}
//_________________________________________________________
//
// Static drawing method which can draw sleeve
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Winder::drawSleeve(QPainter &painter, QRect &rct, bool isHalf)
{
  QColor sleeveColor(181, 147, 112);

  QSize sz(rct.width() / 5, rct.height() / (isHalf ? 2 : 1));
  QPoint pt(rct.left() + (rct.width() - sz.width()) / 2, rct.top());
  QRect rctW(pt, sz);
  painter.fillRect(rctW, QBrush(sleeveColor, Qt::SolidPattern));
}
//_________________________________________________________
//
// Static drawing method which can draw tray beam
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Winder::drawBeam(QPainter &painter, QRect &rct, bool isHalf, QColor color)
{
  QSize sz(rct.width() / 6, rct.height() / (isHalf ? 2 : 1));
  QPoint pt(rct.left() + (rct.width() - sz.width()) / 2, rct.top());
  QRect rctW(pt, sz);
  painter.fillRect(rctW, QBrush(color, Qt::SolidPattern));
}
//_________________________________________________________
//
// Calculate bobbins rectangle based on the winder dimensions
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QRect Winder::getBobbinsRect()
{
  QRect srcRect = rect();
  QRect rct(x(), y(), srcRect.width(), srcRect.height() >> 1);
  QSize bmSize(rct.width() >> 2, rct.height());
  return QRect(QPoint(rct.left() + (rct.width() >> 3), rct.bottom()), bmSize);
}
//_________________________________________________________
//
// Static drawing method which can draw winded bobbins according to ready percentage
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Winder::drawBobbins(QPainter &painter, QRect &rct, bool isHalf, int percentage/* = 100*/)
{
  QColor spoolColor(175, 177, 184);

  QSize sz(percentage * rct.width() / 100, rct.height() / 2.4);
  QPoint pt(rct.left() + (rct.width() - sz.width()) / 2, rct.top() + rct.width() / 5);
  QRect spoolRect(pt, sz);
  painter.fillRect(spoolRect, QBrush(spoolColor, Qt::SolidPattern));
  if (!isHalf)
  {
    spoolRect.setY(rct.bottom() - rct.width() / 5 - sz.height());
    spoolRect.setHeight(sz.height());
    painter.fillRect(spoolRect, QBrush(spoolColor, Qt::SolidPattern));
  }
}
//_________________________________________________________
//
// Static drawing method which can draw sleeve and winded bobbins
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Winder::drawSpools(QPainter &painter, QRect &rct, bool isHalf)
{
  drawSleeve(painter, rct, isHalf);
  drawBobbins(painter, rct, isHalf);
}
//_________________________________________________________
//
// Draw the control content according to its state
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Winder::paintEvent(QPaintEvent *pe)
{
  Q_UNUSED(pe)

  QPainter painter;
  QRect srcRect = rect();
  QColor defColor(120, 120, 120);

  // calculate sizes
  QRect rct(srcRect.left(), srcRect.top(), srcRect.width(), srcRect.height() >> 1);
  QSize bmSize(rct.width() >> 2, rct.height());
  QRect beamLeft(QPoint(rct.left() + (rct.width() >> 3), rct.bottom()), bmSize);
  QRect beamRight(QPoint(rct.left() + 5 * (rct.width() >> 3), rct.bottom()), bmSize);

  painter.begin(this);    // open drawing context

  // draw background
  switch(m_status)
  {
    case READY:
      painter.fillRect(rct, QBrush(QColor(232, 150, 55), Qt::SolidPattern));
      break;
    case FAIL:
      painter.fillRect(rct, QBrush(QColor(255, 97, 135), Qt::SolidPattern));
      break;
    case LOADED:
      painter.fillRect(rct, QBrush(QColor(95, 178, 150), Qt::SolidPattern));
      break;
    case EMPTY:
      painter.fillRect(rct, QBrush(QColor(190, 190, 190), Qt::SolidPattern));
      break;
    case CUTEDGE:
      painter.fillRect(rct, QBrush(QColor(115, 174, 206), Qt::SolidPattern));
      break;
    default:
      painter.fillRect(rct, QBrush(defColor, Qt::SolidPattern));
      break;
  }

  // draw info panel: id and winding time
  painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));
  painter.drawText(rct.left() + 5, 15, m_id);
  if (m_wind_timer > 0)
  {
      QString str = QString().setNum(m_timeLeft * m_timeCoefficient / 1000);
      painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
      painter.drawText(rct.left() + 5, rct.bottom() - 5, str);
  }

  // draw beams and/or sleeves
  switch(m_status)
  {
    case READY:
    case FAIL:
    case CUTEDGE:
      drawSpools(painter, beamLeft, m_halfMode);
      if (m_status == FAIL)
        drawBeam(painter, beamRight, m_halfMode, defColor);
      else
        drawSleeve(painter, beamRight, m_halfMode);
      break;
    case LOADED:
      drawSleeve(painter, beamLeft, m_halfMode);
      drawSleeve(painter, beamRight, m_halfMode);
      break;
    case EMPTY:
      drawBeam(painter, beamLeft, m_halfMode, defColor);
      drawSleeve(painter, beamRight, m_halfMode);
      break;
  }

  // draw bobbins
  if (m_wind_timer > 0 && m_status != FAIL)
  {
    drawBobbins(painter, beamRight, m_halfMode, m_readiness);
  }

  painter.end();        // close drawing context
}
//_________________________________________________________
//
// Timer handler event
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Winder::timerEvent(QTimerEvent* te)
{
  // wind timer handler
  if (te->timerId() == m_wind_timer)
  {
    // update ready percentage to reflect bobbins animation
    m_timeLeft = m_timeLeft <= timerResolution ? 0 : (m_timeLeft - timerResolution);
    m_readiness = 100 * (m_timeWind - m_timeLeft) / m_timeWind;
    // if timer is over
    if (m_timeLeft == 0)
    {
      killTimer(te->timerId());
      m_wind_timer = 0;
      // set winder status
      if (m_status != LOADED)
      {
        // failed if left tray is not loaded by sleever
        setStatus(FAIL);
        // notify supervisor
        emit winderFailed(m_id);
      }
      else
        // switch machine onto exchange task
        startMachine(EXCHANGE);
    }
    // if alert is possible notify supervisor
    else if (m_timeLeft == m_timeAlert && !m_cutEdgeMode)
    {
      emit winderAlert(m_id);
    }
    update();
  }
  // rotate timer handler. It's just a duration, not counter
  else if (te->timerId() == m_rotate_timer)
  {
    // the timer is over, kill it
    killTimer(te->timerId());
    m_rotate_timer = 0;
    // if cut edge mode is on notify supervisor
    if (m_cutEdgeMode)
    {
      setStatus(CUTEDGE);
      emit bobbinsCutNeeded(m_id);
    }
    // otherwise set winder to ready state
    else
    {
      setStatus(READY);
      emit bobbinsReady(m_id);
    }
    // start the next winding task
    startMachine(START);
    update();
  }
}
//_________________________________________________________
//
// Private task machine starter
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Winder::startMachine(OperFunc operation)
{
  switch(operation)
  {
    case START:
      m_timeLeft = m_timeWind;
      m_readiness = 0;
      m_rotate_timer = 0;
      m_wind_timer = startTimer(timerResolution);
      break;
    case EXCHANGE:
      m_timeLeft = m_timeExchange;
      m_wind_timer = 0;
      m_rotate_timer = startTimer(m_timeExchange);
      break;
    case STOP:
      break;
  }
}
//_________________________________________________________
//
// Public status setting method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Winder::setStatus(Status state)
{
  m_status = state;
}
//_________________________________________________________
//
// Public action method for winding start
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Winder::startWinding()
{
  // winder should be loaded
  if (m_status != LOADED) return;
  startMachine(START);
}


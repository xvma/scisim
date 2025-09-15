#include "winder.h"
#include "anim.h"

const int timerResolution = 70;
//_________________________________________________________
//
// Object constructor. Set common styles and resize the control
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Animator::Animator(Type srcType, int controlWidth, int controlHeight, QWidget *parent /*=0*/) :
  QFrame(parent)
{
  setLineWidth(1);
  setFrameStyle(NoFrame | Plain);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  resize(controlWidth, controlHeight);
  m_type = srcType;   // set initial animation form
}
//_________________________________________________________
//
// Draw the control content according to the type
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Animator::paintEvent(QPaintEvent *pe)
{
  Q_UNUSED(pe)

  QPainter painter;
  QRect srcRect = rect();

  painter.begin(this);      // open drawing context

  // use winder drawing static methods
  switch(m_type)
  {
    case SPOOL1:
    case SPOOL2:
      Winder::drawSpools(painter, srcRect, m_type == SPOOL1);
      break;
    case SLEEVE1:
    case SLEEVE2:
      Winder::drawSleeve(painter, srcRect, m_type == SLEEVE1);
      break;
    default:
      break;
  }
  painter.end();        // close drawing context
}




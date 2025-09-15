#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <QFrame>
#include <QtGui>
//_________________________________________________________
//
// Class represents animation widget for doffer and sleever.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class Animator : public QFrame
{
  Q_OBJECT
public:
  enum Type
  {
    SPOOL1 = 0,   // draw half-length sleeve and one bobbin
    SPOOL2,       // draw full-length sleeve and two bobbins
    SLEEVE1,      // draw only half-length empty sleeve
    SLEEVE2       // draw only full-length empty sleeve
  };
  explicit Animator(Type srcType, int controlWidth, int controlHeight, QWidget *parent = 0);

signals:

public slots:

protected:
  virtual void paintEvent(QPaintEvent *);

private:
  Type m_type;  // animation type
};

#endif

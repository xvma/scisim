#ifndef WINDER_H
#define WINDER_H

#include <QFrame>
#include <QtGui>
#include "invdatabase.h"
//_________________________________________________________
//
// Class represents winder widget.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class Winder : public QFrame
{
  Q_OBJECT
public:
  // Winder states
  enum Status
  {
    EMPTY = 0,          // winder tray is empty
    LOADED,             // winder tray has got sleeve
    READY,              // winder tray has got ready bobbins to deliver
    CUTEDGE,            // winder tray has got ready bobbins which should be cut
    FAIL                // winder has failed
  };
  // Machine actions
  enum OperFunc
  {
    EXCHANGE = 0,       // exchange trays
    START,              // start winding
    STOP                // reserved
  };
  explicit Winder(WinderModel &model, int timeCoefficient, QWidget *parent = 0);

  QString getId() {return m_id;}
  Status getStatus() {return m_status;}
  bool getCutEdgeMode() {return m_cutEdgeMode;}
  void setCutEdgeMode(bool newState) {m_cutEdgeMode = newState;}

  void setStatus(Status state);
  void startWinding();

  static void drawBobbins(QPainter &painter, QRect &rct, bool isHalf, int percentage = 100);
  static void drawBeam(QPainter &painter, QRect &rct, bool isHalf, QColor color);
  static void drawSpools(QPainter &painter, QRect &rct, bool isHalf);
  static void drawSleeve(QPainter &painter, QRect &rct, bool isHalf);

  QRect getBobbinsRect();

signals:
  void bobbinsReady(QString idWinder);
  void bobbinsCutNeeded(QString idWinder);
  void winderFailed(QString idWinder);
  void winderAlert(QString idWinder);

public slots:

protected:
  virtual void timerEvent(QTimerEvent *);
  virtual void paintEvent(QPaintEvent *);

private:
  void startMachine(OperFunc operation);

  Status m_status;        // current winder status
  int m_timeWind;         // winding time value
  int m_timeAlert;        // about ready alert time value
  int m_timeExchange;     // trays exchange time value
  int m_timeCoefficient;  // time coefficient from config model
  bool m_halfMode;        // true if only one bobbing can be winded otherwise false
  bool m_cutEdgeMode;     // true if it's necessary to cut bobbins after they're ready

  int m_readiness;        // winding completed percentage for the right tray
  QString m_id;           // object id
  int m_timeLeft;         // time left counter

  int m_wind_timer;       // wind timer id
  int m_rotate_timer;     // tray rotate timer id
};

#endif

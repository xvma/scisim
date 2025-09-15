#ifndef SPOOLER_H
#define SPOOLER_H

#include <QFrame>
#include <QtGui>
#include "invdatabase.h"
//_________________________________________________________
//
// Class represents spooler widget.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class Spooler : public QFrame
{
  Q_OBJECT
public:
  // Spooler states
  enum Status
  {
    READY = 0,                // reserved
    PROGRESS,                 // spooler is in process of filling up
    BUSY                      // spooler is under the maintenace (rotating or changing)
  };
  // Spooler cell states
  enum CellStatus
  {
    FREE = 0,                 // cell is empty and able to reserve
    CELLBUSY,                 // cell has got installed bobbin
    RESERVED                  // cell is empty but has been reserved
  };

  explicit Spooler(SpoolerModel &model, QWidget *parent = 0);

  QString getId() {return m_id;}
  Status getStatus() {return m_status;}
  void setStatus(Status state);

  int getCellWidth();
  void replace();
  bool reserve(int &x, int &y);
  bool cancelReserve(int x, int y);
  void putdown(int x, int y);
  bool isFilledUp();
  bool isAbleToRotate();

signals:
  void filledUp(QString idSpooler);

public slots:

protected:
  virtual void paintEvent(QPaintEvent *);

private:
  void createItems();
  void clearItems();

  Status m_status;                              // current spooler status
  int m_rows;                                   // rows amount
  int m_columns;                                // columns amount
  bool m_isDoubleSided;                         // true if spooler is double-sided

  int cellWidth;                                // converted cell width in pixels

  QString m_id;                                 // object id
  QVector< QVector<CellStatus> > m_items[2];    // array for spooler sides
  int m_activeSide;                             // active side index
};

#endif

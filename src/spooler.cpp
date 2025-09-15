#include "supervisor.h"
#include "spooler.h"

const int controlTitle = 20;    // spooler caption height
//_________________________________________________________
//
// Object constructor. Set parameters from model, common styles and sizes
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Spooler::Spooler(SpoolerModel &model, QWidget *parent /*=0*/) :
  QFrame(parent)
{
  setLineWidth(1);
  setFrameStyle(NoFrame | Plain);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  // init params from database model
  m_id = model.idSpooler;
  m_rows = model.rows;
  m_columns = model.columns;
  m_isDoubleSided = model.isDoubleSided;

  // convert cell width and resize the widget
  Supervisor *supervisor = (Supervisor *)parent;
  cellWidth = supervisor->toPixels(model.cellWidth);
  resize(cellWidth * m_columns, cellWidth * m_rows + controlTitle);

  // set spooler in progress
  m_status = PROGRESS;
  createItems();
}
//_________________________________________________________
//
// Public getter for cell width
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Spooler::getCellWidth()
{
  return cellWidth;
}
//_________________________________________________________
//
// Draw the control content according to its state
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Spooler::paintEvent(QPaintEvent *pe)
{
  Q_UNUSED(pe)

  QPainter painter;
  QRect rct = rect();
  QRect spoolRct = rect();

  QColor spoolColor(175, 177, 184);
  QColor backColor(95, 178, 150);

  // calculate spool rectangle
  spoolRct.setTop(rct.top() + controlTitle);
  // open drawing context
  painter.begin(this);

  // fill background
  painter.fillRect(rct, QBrush(Qt::gray, Qt::SolidPattern));
  painter.fillRect(spoolRct, QBrush(backColor, Qt::SolidPattern));

  // draw caption
  painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));
  if (m_isDoubleSided)
    painter.drawText(rct.left() + 5, 15, QString("%1: side %2").arg(m_id).arg(m_activeSide + 1));
  else
    painter.drawText(rct.left() + 5, 15, QString("%1").arg(m_id));

  int ht = cellWidth;
  QSize cellSize(ht, ht);

  // draw front and cells according to their state
  switch(m_status)
  {
    case READY:
    case PROGRESS:
      for(int i = 0; i < m_items[m_activeSide].size(); i++)
      {
        for(int j = 0; j < m_items[m_activeSide][i].size(); j++)
        {
          QPoint cellPoint(j * ht, i * ht + controlTitle);
          CellStatus status = m_items[m_activeSide][i][j];
          QRect cellRect(cellPoint, cellSize);

          switch(status)
          {
            case CELLBUSY:
              painter.setBrush(QBrush(spoolColor, Qt::SolidPattern));
              painter.drawEllipse(QPoint(cellPoint.x() + (cellSize.width() >> 1), cellPoint.y() + (cellSize.height() >> 1)), ht / 3, ht / 3);
              painter.setBrush(QBrush(backColor, Qt::SolidPattern));
              painter.drawEllipse(QPoint(cellPoint.x() + (cellSize.width() >> 1), cellPoint.y() + (cellSize.height() >> 1)), 3, 3);
              break;
            case RESERVED:
              painter.drawText(cellRect, Qt::AlignHCenter | Qt::AlignCenter, "R");
              break;

            case FREE:
            default:
              break;
          }
        }
      }

      break;
    case BUSY:
      painter.fillRect(spoolRct, QBrush(Qt::black, Qt::Dense5Pattern));
      painter.setPen(Qt::white);
      painter.drawText(spoolRct, Qt::AlignHCenter | Qt::AlignCenter, "Busy...");
      break;
  default:
    break;

  }

  painter.end();
}
//_________________________________________________________
//
// Public status setting method
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Spooler::setStatus(Status state)
{
  m_status = state;
}
//_________________________________________________________
//
// Public method to replace spooler
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Spooler::replace()
{
  // if spooler rotateable then change the side
  // otherwise reinit spooler

  if (isAbleToRotate())
    m_activeSide++;
  else
    createItems();
}
//_________________________________________________________
//
// Create all cell for both sides
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Spooler::createItems()
{
  clearItems();
  for(int i = 0; i < m_rows; i++)
  {
    QVector<CellStatus> row;
    for(int j = 0; j < m_columns; j++)
    {
       row.append(FREE);
    }
    m_items[0].append(row);
    m_items[1].append(row);
  }
}
//_________________________________________________________
//
// Clear all spooler cells items
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Spooler::clearItems()
{
  m_items[0].clear();
  m_items[1].clear();
  m_activeSide = 0;
}
//_________________________________________________________
//
// Reserve the cell method. Cell will be returned as (x, y) row-column positions
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Spooler::reserve(int &x, int &y)
{
  // for each cell check if it's free and if so mark it as reserved
  for(int i = 0; i < m_items[m_activeSide].size(); i++)
  {
    for(int j = 0; j < m_items[m_activeSide][i].size(); j++)
    {
      if (m_items[m_activeSide][i][j] == FREE)
      {
        x = i;
        y = j;
        m_items[m_activeSide][i][j] = RESERVED;
        update();
        return true;
      }
    }
  }
  return false;
}
//_________________________________________________________
//
// Cancel cell reservation of position (x, y)
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Spooler::cancelReserve(int x, int y)
{
  if (m_items[m_activeSide][x][y] == RESERVED)
  {
    m_items[m_activeSide][x][y] = FREE;
    update();
    return true;
  }
  return false;
}
//_________________________________________________________
//
// Set cell position (x, y) as busy
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Spooler::putdown(int x, int y)
{
  if (x >= m_rows || y >= m_columns) return;
  m_items[m_activeSide][x][y] = CELLBUSY;
  update();
  // notify supervisor if spooler active side is filled up
  if (isFilledUp())
    emit filledUp(m_id);
}
//_________________________________________________________
//
// Check if the spooler active side is filled up
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Spooler::isFilledUp()
{
  for(int i = 0; i < m_items[m_activeSide].size(); i++)
  {
    for(int j = 0; j < m_items[m_activeSide][i].size(); j++)
    {
        if (m_items[m_activeSide][i][j] == FREE ||
            m_items[m_activeSide][i][j] == RESERVED)
        {
          return false;
        }
    }
  }
  return true;
}
//_________________________________________________________
//
// Check if the spooler is able to rotate
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Spooler::isAbleToRotate()
{
  return (m_isDoubleSided && m_activeSide == 0);
}




#include "logger.h"
//_________________________________________________________
//
// Object constructor.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LoggerTableView::LoggerTableView(QObject *parent /*=0*/) :
  QAbstractTableModel(parent)
{

}
//_________________________________________________________
//
// Object destructor.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LoggerTableView::~LoggerTableView()
{
  clearItems();
}
//_________________________________________________________
//
// Cleanup the table
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LoggerTableView::clearItems()
{
  foreach(LoggerModel *it, m_items)
    if (it != NULL) delete it;
  m_items.clear();
}
//_________________________________________________________

int LoggerTableView::rowCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return m_items.size();
}
//_________________________________________________________

int LoggerTableView::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return 4;
}
//_________________________________________________________
//
// Return item for the table view
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QVariant LoggerTableView::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();
  if (role == Qt::TextAlignmentRole)
    switch (index.column())
    {
      case 0: return Qt::AlignCenter;
      case 1: return Qt::AlignRight;
      case 2: return Qt::AlignRight;
      case 3: return Qt::AlignRight;
      default: return QVariant();
    };

  if (role != Qt::DisplayRole)
    return QVariant();

  LoggerModel *model = m_items.at(index.row());
  switch (index.column())
  {
    case 0: return model->getId();
    case 1: return model->timeIdle;
    case 2: return model->timeMoving;
    case 3: return model->timeBusy;
    default: return QVariant();
  };
}
//_________________________________________________________
//
// Return item header for the table view
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QVariant LoggerTableView::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    return QVariant();
  switch (section)
  {
    case 0: return "Object";
    case 1: return "Idle(sec)";
    case 2: return "Moving(sec)";
    case 3: return "Busy(sec)";
    default: return QVariant();
  }
}
//==========================================================
//_________________________________________________________
//
// Object constructor. Set common styles and resize the control
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Logger::Logger(QWidget *parent /*=0*/): QDialog(parent)
{
  setWindowTitle("Statistics");
  setMinimumSize(300, 200);
  resize(500, 325);

  m_table = new QTableView();
  m_table->setModel(&m_loggerView);

  QPushButton* btnReset=new QPushButton("&Reset statistics");
  connect(btnReset, SIGNAL(clicked()), SLOT(resetStatistics()));

  QVBoxLayout* pvbxLayout = new QVBoxLayout(this);
  pvbxLayout->addWidget(m_table);
  pvbxLayout->addWidget(btnReset);

  m_table->show();
}
//_________________________________________________________
//
// Object destructor.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Logger::~Logger()
{
  if (m_table != NULL) delete m_table;
}
//_________________________________________________________
//
// Refresh table view content
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::refresh()
{
  m_loggerView.layoutChanged();
}
//_________________________________________________________
//
// Clean up table view content
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::clear()
{
  m_loggerView.clearItems();
  refresh();
}
//_________________________________________________________
//
// Reset model statistics values
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Logger::resetStatistics()
{
  foreach(LoggerModel *it, getItems())
  {
    it->timeBusy = 0;
    it->timeIdle = 0;
    it->timeMoving = 0;
    it->startTime = QDateTime::currentDateTime();
  }

  refresh();
}






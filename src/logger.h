#ifndef LOGGER_H
#define LOGGER_H

#include <QDialog>
#include <QTableWidget>
#include <QtGui>
#include <QVBoxLayout>
#include <QPushButton>

// Logger item models
struct LoggerModel
{
  QString idObject;
  QDateTime startTime;     // Stating statistics point
  qint64 timeIdle;           // Idle duration value (ms)
  qint64 timeMoving;         // Moving duration value(ms)
  qint64 timeBusy;           // Busy duration value (ms)

  QString getId() {return idObject;}
};
//_________________________________________________________
//
// Class represents the table view for logger model list
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class LoggerTableView : public QAbstractTableModel
{
  Q_OBJECT
public:

  explicit LoggerTableView(QObject *parent = 0);
  virtual ~LoggerTableView();

  int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

  QList<LoggerModel *> &getItems() {return m_items;}
  void clearItems();
signals:

public slots:

protected:

private:
  QList<LoggerModel *> m_items;
};
//_________________________________________________________
//
// Class represents the statistic window .
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class Logger : public QDialog
{
  Q_OBJECT
public:
  enum FieldNames
  {
    TIME_IDLE = 0,
    TIME_MOVE,
    TIME_BUSY
  };

  explicit Logger(QWidget *parent = 0);
  virtual ~Logger();

  QList<LoggerModel *> &getItems() {return m_loggerView.getItems();}
  void refresh();
  void clear();
signals:

private slots:
  void resetStatistics();

protected:

private:
  QTableView *m_table;
  LoggerTableView m_loggerView;
};

#endif

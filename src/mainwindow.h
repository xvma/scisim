#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>

#include "logger.h"
#include "invdatabase.h"
#include "supervisor.h"

//_________________________________________________________
//
// Class represents Main Window widget
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  virtual ~MainWindow();


protected:
  void resizeEvent(QResizeEvent *ev);

public slots:
  void appendLoggerItem(QString idObject);
  void updateLoggerItem(QString idObject, Logger::FieldNames field);

private slots:
  void startSession();
  void stopSession();
  void showStatistics();

private:
  void createActions();
  void createMenus();

  QMenu *appMenu;         //app menu reference

  // menu action widgets
  QAction *newAct;
  QAction *stopAct;
  QAction *exitAct;
  QAction *showStatAct;

  QScrollArea *scroller;    //scrolling widget as workarea
  Supervisor *supervisor;   // supervisor reference
  Logger *statLog;
};

#endif

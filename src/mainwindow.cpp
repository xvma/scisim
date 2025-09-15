#include <QtWidgets>

#include "mainwindow.h"
//_________________________________________________________
//
// Object constructor. Set menus actions and other objects
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MainWindow::MainWindow()
{
  createActions();
  createMenus();

  // create logger
  statLog = new Logger(this);

  // create supervisor and set it to the scroll area
  supervisor = new Supervisor();
  connect(supervisor, SIGNAL(appendLoggerItem(QString)), this, SLOT(appendLoggerItem(QString)));
  connect(supervisor, SIGNAL(updateLoggerItem(QString,Logger::FieldNames)), this, SLOT(updateLoggerItem(QString,Logger::FieldNames)));

  scroller = new QScrollArea();
  scroller->setWidget(supervisor);
  scroller->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  scroller->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  // show the work area
  setCentralWidget(scroller);
  scroller->show();
  supervisor->show();

  // show window
  statusBar()->showMessage("Ready");
  setWindowTitle("Scirocco");
  setMinimumSize(800, 600);
  resize(900, 700);
}
//_________________________________________________________
//
// Object destructor. Destroy all child objects
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MainWindow::~MainWindow()
{
  if (supervisor != NULL) delete supervisor;
  if (scroller != NULL) delete scroller;
  if (statLog != NULL) delete statLog;

  if (appMenu != NULL) delete appMenu;
  if (newAct != NULL) delete newAct;
  if (stopAct != NULL) delete stopAct;
  if (exitAct != NULL) delete exitAct;
  if (showStatAct != NULL) delete showStatAct;
}
//_________________________________________________________
//
// Start session action
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainWindow::startSession()
{
  // init containers and models from database
  supervisor->start();
  // start simulation
  supervisor->startWinders();
}
//_________________________________________________________
//
// Stop session action
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainWindow::stopSession()
{
  // destroy simulation
  supervisor->stop();
  statLog->clear();
}
//_________________________________________________________
//
// Show / Hide statistics window
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainWindow::showStatistics()
{
  if (statLog->isHidden())
  {
    statLog->move(10, y() + height() - 350);
    statLog->show();
    showStatAct->setChecked(true);
  }
  else
  {
    statLog->hide();
    showStatAct->setChecked(false);
  }
}
//_________________________________________________________
//
// Create all menu actions
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainWindow::createActions()
{
  newAct = new QAction("&Start", this);
  newAct->setShortcuts(QKeySequence::New);
  newAct->setStatusTip("Start new simulation");
  connect(newAct, SIGNAL(triggered()), this, SLOT(startSession()));

  stopAct = new QAction("&Stop", this);
  stopAct->setStatusTip("Stop current simulation");
  connect(stopAct, SIGNAL(triggered()), this, SLOT(stopSession()));

  showStatAct = new QAction("&Statistics", this);
  showStatAct->setStatusTip("Show statistics log");
  showStatAct->setCheckable(true);
  connect(showStatAct, SIGNAL(triggered()), this, SLOT(showStatistics()));

  exitAct = new QAction("E&xit", this);
  exitAct->setShortcuts(QKeySequence::Quit);
  exitAct->setStatusTip("Exit the application");
  connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
}
//_________________________________________________________
//
// Create main menu
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainWindow::createMenus()
{
  appMenu = new QMenu("&Actions");
  appMenu->addAction(newAct);
  appMenu->addAction(stopAct);
  appMenu->addSeparator();
  appMenu->addAction(showStatAct);
  appMenu->addSeparator();
  appMenu->addAction(exitAct);

  menuBar()->addMenu(appMenu);
}
//_________________________________________________________
//
// Resizer event handler
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainWindow::resizeEvent(QResizeEvent *ev)
{
  Q_UNUSED(ev)
  // calculate aspect ratio for converting mm to pixels
  supervisor->SetWholeWidthPixels(scroller->width());
}
//_________________________________________________________
//
// Add new item to the statistics window
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainWindow::appendLoggerItem(QString idObject)
{
  if (statLog == NULL) return;

  // create new logger model
  LoggerModel *pItem = new LoggerModel();
  pItem->idObject = idObject;
  pItem->startTime = QDateTime::currentDateTime();
  pItem->timeBusy = 0;
  pItem->timeMoving = 0;
  pItem->timeIdle = 0;

  // add to list
  statLog->getItems().append(pItem);
  statLog->refresh();
}
//_________________________________________________________
//
// Update logger item field
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainWindow::updateLoggerItem(QString idObject, Logger::FieldNames field)
{
  if (statLog == NULL) return;
  // get logger table item to update
  LoggerModel *model = Supervisor::getItemById<LoggerModel>(idObject, statLog->getItems());
  if (model == NULL) return;
  // count duration
  int timeCoef = supervisor->getConfigModel().timeCoefficient;
  qint64 duration = model->startTime.msecsTo(QDateTime::currentDateTime()) * timeCoef / 1000;
  //qDebug() << "update " << idObject << field << duration;
  // update logget model field
  switch(field)
  {
    case Logger::TIME_IDLE:
      model->timeIdle += duration;
      break;
    case Logger::TIME_MOVE:
      model->timeMoving += duration;
      break;
    case Logger::TIME_BUSY:
      model->timeBusy += duration;
      break;
    default:
      break;
  }
  // set new start time
  model->startTime = QDateTime::currentDateTime();
  statLog->refresh();
}

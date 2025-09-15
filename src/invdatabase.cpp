#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include "invdatabase.h"

//_________________________________________________________
//
// Open scirocco database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QSqlDatabase InventoryDatabase::open()
{
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "scirocco");
  db.setDatabaseName("scirocco.db ");
  if (!db.open())
    {
      qDebug() << db.lastError().text();
    }
  return db;
}
/*/_________________________________________________________
//
// Open scirocco database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QSqlDatabase InventoryDatabase::open()
{
  QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
  db.setHostName("DBSERVER-XP");
  db.setDatabaseName("scirocco.db");
  //db.setUserName("hm");
  //db.setPassword("hm123");
  if (!db.open())
    {
      qDebug() << db.lastError().text();
    }
  return db;
}*/
//_________________________________________________________
//
// Close scirocco database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InventoryDatabase::close(QSqlDatabase &db)
{
  db.close();
}
//_________________________________________________________
//
// Fill up winder models list from database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InventoryDatabase::getWindersView(QSqlDatabase &db, QList<WinderModel *> &list, int timeCoeff)
{
  // Check if database is opened
  if (!db.isOpen())
    return false;

  // Run winder query
  QSqlQuery query(db);
  if (!query.exec("SELECT * FROM winder ORDER BY id_doffer"))
    {
      qDebug() << "SELECT winders failed" << query.lastError().text() << ", [" << query.lastError().number() << ']';
      return false;
    }

  // Reading records and init the model
  QSqlRecord rec = query.record();
  while (query.next())
  {
    WinderModel *winderModel = new WinderModel();
    winderModel->idWinder = query.value(rec.indexOf("id")).toString();
    winderModel->idDoffer = query.value(rec.indexOf("id_doffer")).toString();
    winderModel->idSleever = query.value(rec.indexOf("id_sleever")).toString();
    winderModel->isHalfMode = query.value(rec.indexOf("ishalf")).toBool();
    winderModel->timeExchange = query.value(rec.indexOf("xchg_time_ms")).toInt();
    winderModel->timeWind = query.value(rec.indexOf("wind_time_ms")).toInt();
    winderModel->timeAlert = query.value(rec.indexOf("alert_time_ms")).toInt();
    winderModel->width = query.value(rec.indexOf("width_mm")).toInt();

    if (timeCoeff > 0)
    {
      winderModel->timeExchange /= timeCoeff;
      winderModel->timeWind /= timeCoeff;
      winderModel->timeAlert /= timeCoeff;
    }

    list.append(winderModel);
  }
  return true;
}
//_________________________________________________________
//
// Fill up doffer models list from database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InventoryDatabase::getDoffersView(QSqlDatabase &db, QList<DofferModel *> &list, int timeCoeff)
{
  // Check if database is opened
  if (!db.isOpen())
    return false;

  // Run doffer query
  QSqlQuery query(db);
  if (!query.exec("SELECT * FROM doffer d " \
                  "WHERE EXISTS (SELECT 1 FROM winder w WHERE w.id_doffer = d.id) " \
                  "ORDER BY d.id"))
    {
      qDebug() << "SELECT doffers failed" << query.lastError().text() << ", [" << query.lastError().number() << ']';
      return false;
    }

  // Reading records and init the model
  QSqlRecord rec = query.record();
  while (query.next())
  {
    DofferModel *itemModel = new DofferModel();
    itemModel->idDoffer = query.value(rec.indexOf("id")).toString();
    itemModel->speed = query.value(rec.indexOf("speed_mm_s")).toInt();
    itemModel->timeGetIn = query.value(rec.indexOf("getin_time_ms")).toInt();
    itemModel->timePutDown = query.value(rec.indexOf("pdown_time_ms")).toInt();
    itemModel->acceleration = query.value(rec.indexOf("accel_mm_ss")).toInt();
    itemModel->width = query.value(rec.indexOf("width_mm")).toInt();

    if (timeCoeff > 0)
    {
      itemModel->timeGetIn /= timeCoeff;
      itemModel->timePutDown /= timeCoeff;
      itemModel->speed *= timeCoeff;
      itemModel->acceleration *= (timeCoeff * timeCoeff);
    }

    list.append(itemModel);
  }
  return true;
}
//_________________________________________________________
//
// Fill up sleever models list from database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InventoryDatabase::getSleeversView(QSqlDatabase &db, QList<SleeverModel *> &list, int timeCoeff)
{
  // Check if database is opened
  if (!db.isOpen())
    return false;

  // Run sleever query
  QSqlQuery query(db);
  if (!query.exec("SELECT * FROM sleever d " \
                  "WHERE EXISTS (SELECT 1 FROM winder w WHERE w.id_sleever = d.id) " \
                  "ORDER BY d.id"))
    {
      qDebug() << "SELECT sleevers failed" << query.lastError().text() << ", [" << query.lastError().number() << ']';
      return false;
    }

  // Reading records and init the model
  QSqlRecord rec = query.record();
  while (query.next())
  {
    SleeverModel *itemModel = new SleeverModel();
    itemModel->idSleever = query.value(rec.indexOf("id")).toString();
    itemModel->speed = query.value(rec.indexOf("speed_mm_s")).toInt();
    itemModel->timePutDown = query.value(rec.indexOf("pdown_time_ms")).toInt();
    itemModel->sleeveSlots = query.value(rec.indexOf("sleeve_slots")).toInt();
    itemModel->rings = query.value(rec.indexOf("rings")).toInt();
    itemModel->acceleration = query.value(rec.indexOf("accel_mm_ss")).toInt();
    itemModel->prepare = query.value(rec.indexOf("prepare_time_ms")).toInt();
    itemModel->width = query.value(rec.indexOf("width_mm")).toInt();

    if (timeCoeff > 0)
    {
      itemModel->prepare /= timeCoeff;
      itemModel->timePutDown /= timeCoeff;
      itemModel->speed *= timeCoeff;
      itemModel->acceleration *= (timeCoeff * timeCoeff);
    }

    list.append(itemModel);

  }
  return true;
}
//_________________________________________________________
//
// Fill up spooler models list from database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InventoryDatabase::getSpoolersView(QSqlDatabase &db, QList<SpoolerModel *> &list)
{
  // Check if database is opened
  if (!db.isOpen())
    return false;

  // Run sleever query
  QSqlQuery query(db);
  if (!query.exec("SELECT s.* FROM doffer d " \
                  "LEFT JOIN spooler s ON s.id_doffer = d.id " \
                  "WHERE EXISTS (SELECT 1 FROM winder w WHERE w.id_doffer = d.id) " \
                  "ORDER BY s.id_doffer"))
    {
      qDebug() << "SELECT spoolers failed" << query.lastError().text() << ", [" << query.lastError().number() << ']';
      return false;
    }

  // Reading records and init the model
  QSqlRecord rec = query.record();
  while (query.next())
  {
    SpoolerModel *itemModel = new SpoolerModel();
    itemModel->idSpooler = query.value(rec.indexOf("id")).toString();
    itemModel->idDoffer = query.value(rec.indexOf("id_doffer")).toString();
    itemModel->isDoubleSided = query.value(rec.indexOf("isdouble")).toBool();
    itemModel->rows = query.value(rec.indexOf("rows")).toInt();
    itemModel->columns = query.value(rec.indexOf("columns")).toInt();
    itemModel->cellWidth = query.value(rec.indexOf("cell_width_mm")).toInt();
    list.append(itemModel);
  }
  return true;
}
//_________________________________________________________
//
// Fill up manview models list from database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InventoryDatabase::getMenView(QSqlDatabase &db, QList<ManServiceModel *> &list, int timeCoeff)
{
  // Check if database is opened
  if (!db.isOpen())
    return false;

  // Run manservice query
  QSqlQuery query(db);
  if (!query.exec("SELECT * FROM man"))
    {
      qDebug() << "SELECT men failed" << query.lastError().text() << ", [" << query.lastError().number() << ']';
      return false;
    }

  // Reading records and init the model
  QSqlRecord rec = query.record();
  while (query.next())
  {
    ManServiceModel *itemModel = new ManServiceModel();
    itemModel->idMan = query.value(rec.indexOf("id")).toString();
    itemModel->speed = query.value(rec.indexOf("speed_mm_s")).toInt();
    itemModel->timeStartWinder = query.value(rec.indexOf("wdrstart_time_ms")).toInt();
    itemModel->timeRotateSpooler = query.value(rec.indexOf("rotspl_time_ms")).toInt();
    itemModel->timeChangeSpooler = query.value(rec.indexOf("chgspl_time_ms")).toInt();
    itemModel->timeLoadSleever = query.value(rec.indexOf("loadslv_time_ms")).toInt();
    itemModel->timeCutEdge = query.value(rec.indexOf("cutedge_time_ms")).toInt();

    if (timeCoeff > 0)
    {
      itemModel->timeStartWinder /= timeCoeff;
      itemModel->timeRotateSpooler /= timeCoeff;
      itemModel->timeChangeSpooler /= timeCoeff;
      itemModel->timeLoadSleever /= timeCoeff;
      itemModel->timeCutEdge /= timeCoeff;
      itemModel->speed *= timeCoeff;
    }

    list.append(itemModel);
  }
  return true;
}
//_________________________________________________________
//
// Fill up config model from database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InventoryDatabase::getConfigView(QSqlDatabase &db, ConfigModel &config)
{
  // Check if database is opened
  if (!db.isOpen())
    return false;

  // Run config query
  QSqlQuery query(db);
  if (!query.exec("SELECT * FROM config"))
    {
      qDebug() << "SELECT config failed " << query.lastError().text() << ", [" << query.lastError().number() << ']';
      return false;
    }

  // Reading records and init the model
  QSqlRecord rec = query.record();
  if (query.next())
  {
    config.spaceBetweenWinders = query.value(rec.indexOf("winder_space_mm")).toInt();
    config.serviceZoneWidth = query.value(rec.indexOf("serv_zone_width_mm")).toInt();
    config.timeCoefficient = query.value(rec.indexOf("time_coef")).toInt();
    if (config.timeCoefficient == 0)
      config.timeCoefficient = 1;
  }
  return true;
}
//_________________________________________________________
//
// Update doffers in database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InventoryDatabase::updateDoffers(QSqlDatabase &db, QList<DofferSyncModel> &items)
{
  // Check if database is opened
  if (!db.isOpen())
    return false;

  // Prepare update query
  QSqlQuery query(db);
  query.prepare("UPDATE doffer SET " \
                "sync_xpos_mm = :xpos, " \
                "sync_speed_mm_s = :speed, " \
                "sync_status = :status, " \
                "sync_id_spooler = :id_spooler, " \
                "sync_row = :row, " \
                "sync_column = :column, " \
                "sync_id_winder = :id_winder " \
                "WHERE id = :id");

  //run transaction content
  foreach(DofferSyncModel dsm, items)
  {
    query.bindValue(":xpos", dsm.xPos);
    query.bindValue(":speed", dsm.curSpeed);
    query.bindValue(":status", dsm.status);
    query.bindValue(":id_spooler", dsm.idSpooler);
    query.bindValue(":row", dsm.row);
    query.bindValue(":column", dsm.column);
    query.bindValue(":id_winder", dsm.idWinder);
    query.bindValue(":id", dsm.idDoffer);
    if (!query.exec())
      {
        qDebug() << "UPDATE doffer failed: " << query.lastError().text() << ", [" << query.lastError().number() << ']';
        return false;
      }
  }

  return true;
}
//_________________________________________________________
//
// Update sleevers in database
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool InventoryDatabase::updateSleevers(QSqlDatabase &db, QList<SleeverSyncModel> &items)
{
  // Check if database is opened
  if (!db.isOpen())
    return false;

  // Prepare update query
  QSqlQuery query(db);
  query.prepare("UPDATE sleever SET " \
                "sync_xpos_mm = :xpos, " \
                "sync_speed_mm_s = :speed, " \
                "sync_status = :status, " \
                "sync_sleeves = :sleeves, " \
                "sync_rings = :rings, " \
                "sync_id_winder = :id_winder " \
                "WHERE id = :id");

  //run transaction content
  foreach(SleeverSyncModel dsm, items)
  {
    query.bindValue(":xpos", dsm.xPos);
    query.bindValue(":speed", dsm.curSpeed);
    query.bindValue(":status", dsm.status);
    query.bindValue(":sleeves", dsm.amountSleeves);
    query.bindValue(":rings", dsm.amountRings);
    query.bindValue(":id_winder", dsm.idWinder);
    query.bindValue(":id", dsm.idSleever);
    if (!query.exec())
      {
        qDebug() << "UPDATE sleever failed " << query.lastError().text() << ", [" << query.lastError().number() << ']';
        return false;
      }
  }

  return true;
}
//=========================================================   Seeding
//
// Following methods create draft set of models for all object
// for the case of working without database.
// Those seedings might be used i.e. for testing
//_________________________________________________________

void InventoryDatabase::seedWindersView(QList<WinderModel *> &list)
{
  int i = 1;
  for(; i <= 5; i++)
  {
    WinderModel *winderModel = new WinderModel();
    winderModel->idWinder = QString("W_%1").arg(i);
    winderModel->idDoffer = "D_1";
    winderModel->idSleever = "S_1";
    winderModel->isHalfMode = false;
    winderModel->timeExchange = 500;
    winderModel->timeWind = 90000;
    winderModel->timeAlert = 5000;
    list.append(winderModel);
  }

  for(; i <= 10; i++)
  {
    WinderModel *winderModel = new WinderModel();
    winderModel->idWinder = QString("W_%1").arg(i);
    winderModel->idDoffer = "D_2";
    winderModel->idSleever = "S_2";
    winderModel->isHalfMode = false;
    winderModel->timeExchange = 300;
    winderModel->timeWind = 90000;
    winderModel->timeAlert = 5000;
    list.append(winderModel);
  }

  for(; i <= 15; i++)
  {
    WinderModel *winderModel = new WinderModel();
    winderModel->idWinder = QString("W_%1").arg(i);
    winderModel->idDoffer = "D_3";
    winderModel->idSleever = "S_3";
    winderModel->isHalfMode = false;
    winderModel->timeExchange = 1000;
    winderModel->timeWind = 90000;
    winderModel->timeAlert = 5000;
    list.append(winderModel);
  }

  for(; i <= 20; i++)
  {
    WinderModel *winderModel = new WinderModel();
    winderModel->idWinder = QString("W_%1").arg(i);
    winderModel->idDoffer = "D_4";
    winderModel->idSleever = "S_4";
    winderModel->isHalfMode = false;
    winderModel->timeExchange = 500;
    winderModel->timeWind = 90000;
    winderModel->timeAlert = 5000;
    list.append(winderModel);
  }

}
//_________________________________________________________

void InventoryDatabase::seedDoffersView(QList<DofferModel *> &list)
{
  for(int i = 1; i <= 4; i++)
  {
    DofferModel *item = new DofferModel();
    item->idDoffer = QString("D_%1").arg(i);
    item->speed = 50;
    item->timeGetIn = 2000;
    item->timePutDown = 2000;
    list.append(item);
  }
}
//_________________________________________________________

void InventoryDatabase::seedSleeversView(QList<SleeverModel *> &list)
{
  for(int i = 1; i <= 4; i++)
  {
    SleeverModel *item = new SleeverModel();
    item->idSleever = QString("S_%1").arg(i);
    item->speed = 50;
    item->timePutDown = 2500;
    item->sleeveSlots = 20;
    item->rings = 10;
    list.append(item);
  }
}
//_________________________________________________________

void InventoryDatabase::seedSpoolersView(QList<SpoolerModel *> &list)
{
  int i = 1;
  for(; i <= 2; i++)
  {
    SpoolerModel *item = new SpoolerModel();
    item->idSpooler = QString("SP_%1").arg(i);
    item->idDoffer = "D_1";
    item->isDoubleSided = false;
    item->rows = 4;
    item->columns = 4;
    list.append(item);
  }
  for(; i <= 4; i++)
  {
    SpoolerModel *item = new SpoolerModel();
    item->idSpooler = QString("SP_%1").arg(i);
    item->idDoffer = "D_2";
    item->isDoubleSided = true;
    item->rows = 4;
    item->columns = 3;
    list.append(item);
  }
  for(; i <= 6; i++)
  {
    SpoolerModel *item = new SpoolerModel();
    item->idSpooler = QString("SP_%1").arg(i);
    item->idDoffer = "D_3";
    item->isDoubleSided = false;
    item->rows = 3;
    item->columns = 4;
    list.append(item);
  }
  for(; i <= 8; i++)
  {
    SpoolerModel *item = new SpoolerModel();
    item->idSpooler = QString("SP_%1").arg(i);
    item->idDoffer = "D_4";
    item->isDoubleSided = true;
    item->rows = 3;
    item->columns = 3;
    list.append(item);
  }
}
//_________________________________________________________

void InventoryDatabase::seedMenView(QList<ManServiceModel *> &list)
{
  for(int i = 1; i <= 1; i++)
  {
    ManServiceModel *item = new ManServiceModel();
    item->idMan = QString("Man_%1").arg(i);
    item->speed = 60;
    item->timeStartWinder = 1000;
    item->timeRotateSpooler = 1500;
    item->timeChangeSpooler = 3000;
    item->timeLoadSleever = 2000;
    item->timeCutEdge = 2000;
    list.append(item);
  }
}
//_________________________________________________________

void InventoryDatabase::seedConfigView(ConfigModel &config)
{
  config.serviceZoneWidth = 1000;
  config.spaceBetweenWinders = 200;
  config.timeCoefficient = 1;
}



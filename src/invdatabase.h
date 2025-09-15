#ifndef INVDATABASE_H
#define INVDATABASE_H

#include <QObject>
#include <QSqlDatabase>

// Database models
struct WinderModel
{
  QString idWinder;       // Winder Id
  QString idDoffer;       // Doffer Id which is responsible for getting winder bobbing
  QString idSleever;      // Sleever Id which is responsible for putting new sleeve
  bool isHalfMode;        // True if winder can wind only one bobbin, false if can wind two
  int timeWind;           // Winding time (ms)
  int timeExchange;       // Exchange bobbins time (ms)
  int timeAlert;          // Winder about ready alert time (ms)
  int width;              // Winder width (mm)

  QString getId() {return idWinder;}
};

struct DofferModel
{
  QString idDoffer;       // Doffer Id
  int speed;              // Doffer speed (mm/s)
  int timeGetIn;          // Getting bobbins time (ms)
  int timePutDown;        // Putting bobbins time (ms)
  int width;              // Doffer width (mm)
  int acceleration;       // Doffer acceleration (mm/(s*s))

  QString getId() {return idDoffer;}
};

struct SleeverModel
{
  QString idSleever;      // Sleever Id
  int speed;              // Sleever speed (mm/s)
  int timePutDown;        // Putting sleeve time (ms)
  int sleeveSlots;        // Sleeve slots amount
  int rings;              // Ring slots amount
  int width;              // Sleever width (mm)
  int acceleration;       // Sleever acceleration (mm/(s*s))
  int prepare;            // Prepare new sleeve time (ms)

  QString getId() {return idSleever;}
};

struct SpoolerModel
{
  QString idSpooler;        // Spooler Id
  QString idDoffer;         // Doffer Id which is responsible for putting winder bobbing
  int rows;                 // Spooler rows amount
  int columns;              // Spooler columns amount
  int cellWidth;            // Cell width (mm)
  bool isDoubleSided;       // True if spooler is double-sided otherwise False

  QString getId() {return idSpooler;}
};

struct ManServiceModel
{
  QString idMan;            // ManService Id
  int speed;                // ManService speed (mm/s)
  int timeStartWinder;      // Warming-up winder time (ms)
  int timeRotateSpooler;    // Spooler rotating time (ms)
  int timeChangeSpooler;    // Spooler change time (ms)
  int timeLoadSleever;      // Sleever reloading time (ms)
  int timeCutEdge;          // Bobbins cutting edges time (ms)

  QString getId() {return idMan;}
};

struct ConfigModel
{
  int spaceBetweenWinders;    // Space between winders in group (mm)
  int serviceZoneWidth;       // Service zone width (mm)
  int timeCoefficient;
};

// Update database models
struct DofferSyncModel
{
  QString idDoffer;       // Doffer Id
  QString idWinder;       // Winder Id
  int xPos;               // Reach position for doffer
  int curSpeed;           // Current doffer speed
  int status;             // Current doffer state
  QString idSpooler;      // Spooler Id to deliver bobbins
  int row;                // Spooler destination row
  int column;             // Spooler destination column
};
struct SleeverSyncModel
{
  QString idSleever;      // Sleever Id
  QString idWinder;       // Winder Id
  int xPos;               // Reach position for sleever
  int curSpeed;           // Current sleever speed
  int status;             // Current sleever state
  int amountSleeves;      // Sleeves amount
  int amountRings;        // Rings amount
};

class InventoryDatabase
{
public:
  static QSqlDatabase open();
  static void close(QSqlDatabase &db);

  static bool getWindersView(QSqlDatabase &db, QList<WinderModel *> &list, int timeCoeff);
  static bool getDoffersView(QSqlDatabase &db, QList<DofferModel *> &list, int timeCoeff);
  static bool getSleeversView(QSqlDatabase &db, QList<SleeverModel *> &list, int timeCoeff);
  static bool getSpoolersView(QSqlDatabase &db, QList<SpoolerModel *> &list);
  static bool getMenView(QSqlDatabase &db, QList<ManServiceModel *> &list, int timeCoeff);
  static bool getConfigView(QSqlDatabase &db, ConfigModel &config);

  static bool updateDoffers(QSqlDatabase &db, QList<DofferSyncModel> &items);
  static bool updateSleevers(QSqlDatabase &db, QList<SleeverSyncModel> &items);

  static void seedWindersView(QList<WinderModel *> &list);
  static void seedDoffersView(QList<DofferModel *> &list);
  static void seedSleeversView(QList<SleeverModel *> &list);
  static void seedSpoolersView(QList<SpoolerModel *> &list);
  static void seedMenView(QList<ManServiceModel *> &list);
  static void seedConfigView(ConfigModel &config);
};

#endif // INVDATABASE_H

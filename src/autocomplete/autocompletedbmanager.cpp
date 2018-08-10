#include "autocompletedbmanager.h"

#include <QSql>
#include <QSqlError>
#include <QSqlRecord>
#include <QMessageBox>
#include <QVector>
#include <QSqlResult>
#include <QSqlDriver>
#include <QDebug>

#include "../debugdialog.h"
//#include "modelset.h"

static const QString defaultname = "autocomplete.db";
// table, field name
static const QMap<QString, QString> components = {
    {"TABLE" , "components"},
    {"TITLE" , "title"}};

static const QMap<QString, QString> modules = {
{"TABLE" , "modules"},
{"NAME" , "name"},
{"COMPONENT_ID" , "component_id"},
{"COUNT" , "count"}};

static const QMap<QString, QString> modulescomponents = {
{"TABLE" , "modules_components"},
{"FROM_COMPONENT_LABEL" , "component_label"},
{"FROM_COMPONENT_TITLE" , "component_title"},
{"FROM_COMPONENT_TERMINAL" , "component_terminal"},
{"TO_COMPONENT_LABEL" , "to_component_label"},
{"TO_COMPONENT_TITLE" , "to_component_title"},
{"TO_COMPONENT_TERMINAL" , "to_component_terminal"},
{"MODULE_ID" , "module_id"}};

AutocompleteDBManager* AutocompleteDBManager::singleton = NULL;


void m_debugError(bool result, QSqlQuery & query) {
    if (result) return;

    QSqlError error = query.lastError();
    DebugDialog::debug(QString("%1 %2 %3").arg(error.text()).arg(error.number()).arg(error.type()));
}

void m_debugExec(const QString & msg, QSqlQuery & query) {
    DebugDialog::debug(
			"SQLITE: " + msg + "\n"
			"\t "+ query.lastQuery() + "\n"
			"\t ERROR DRIVER: "+ query.lastError().driverText() + "\n"
			"\t ERROR DB: " + query.lastError().databaseText() + "\n"
            //"\t moduleid:" + (DebugModelPart == NULL ? "" : DebugModelPart->moduleID()) + ""
		);
    QMap<QString, QVariant> map = query.boundValues();
    foreach (QString name, map.keys()) {
        DebugDialog::debug(QString("\t%1:%2").arg(name).arg(map.value(name).toString()));
    }
}

AutocompleteDBManager::AutocompleteDBManager(const QString & databasename) {
	m_database = QSqlDatabase::addDatabase("QSQLITE", "autocomplete");
	m_database.setDatabaseName(databasename);
 
	if (!m_database.open()) {
        DebugDialog::debug("Database connect fail");
	} else {
        DebugDialog::debug("Database connected");
		bool gotTransaction = m_database.transaction();
        DebugDialog::debug(gotTransaction ? "got transaction" : "no transaction");
	}

}

AutocompleteDBManager::~AutocompleteDBManager() {
	//deleteConnection();
}

void AutocompleteDBManager::loadDB(const QString & databasename) {
	if (singleton == NULL) {
        singleton = new AutocompleteDBManager(databasename);
	}
}

QList<QMap<QString, QVariant> *> AutocompleteDBManager::getModelSet(QString title) {
	if (singleton == NULL) {
        singleton = new AutocompleteDBManager(defaultname);
	}
	return singleton->selectModelSet(title);
	
}

QList<QPair<long, long>> AutocompleteDBManager::getFrequentConnect(long setid, int max) {
	if (singleton == NULL) {
		singleton = new AutocompleteDBManager(defaultname);
	}
    return singleton->selectFrequentConnect(setid, max);
}

QList<QMap<QString, QVariant> *> AutocompleteDBManager::getConnectionsByID(QList<long> ids) {
    if (singleton == NULL) {
		singleton = new AutocompleteDBManager(defaultname);
	}
    return singleton->selectConnectionsByID(ids);
}

QMap<QString, QVariant> * AutocompleteDBManager::getConnectionByID(long id) {
	if (singleton == NULL) {
		singleton = new AutocompleteDBManager(defaultname);
	}
    return singleton->selectConnectionByID(id);
}

QList<QMap<QString, QVariant> *> AutocompleteDBManager::getModelSetsByID(QList<long> ids) {
    if (singleton == NULL) {
		singleton = new AutocompleteDBManager(defaultname);
	}
    return singleton->selectModelSetsByID(ids);
}

QMap<QString, QVariant> * AutocompleteDBManager::getModelSetByID(long id) {
	if (singleton == NULL) {
		singleton = new AutocompleteDBManager(defaultname);
	}
    return singleton->selectModelSetByID(id);
}
/*
AutocompleteDBManager::getNextSet(QString title) {
	if (singleton == NULL) {
		singleton = new AutocompleteDBManager(defaultname);
	}
	return singleton->selectNextSet(title);
}*/

QList<QMap<QString, QVariant> *> AutocompleteDBManager::selectModelSet(QString title) {
	
    QList<QMap<QString, QVariant> *> resultList;
    QString queryStr = QString("SELECT mc.* FROM %1 mc "
    "INNER JOIN %2 m "
    "ON mc.%3=m.id "
    "INNER JOIN %4 c "
    "on m.%5=c.id "
    "WHERE c.%6 = '%8' \n"
    "ORDER BY m.%7 DESC, m.id, mc.id").arg(modulescomponents["TABLE"]).arg(modules["TABLE"])
            .arg(modulescomponents["MODULE_ID"]).arg(components["TABLE"])
            .arg(modules["COMPONENT_ID"]).arg(components["TITLE"])
            .arg(modules["COUNT"]).arg(title);
    QSqlQuery query(m_database);
    query.prepare(queryStr);
    //query.bindValue(0,title);
    if (query.exec()) {
		while(query.next()) {
            QSqlRecord record = query.record();
            QMap<QString, QVariant> * map = new QMap<QString, QVariant>();
            for (int i=0; i<record.count(); i++) {
                map->insert(record.fieldName(i), record.value(i));
                //DebugDialog::debug("result " + record.fieldName(i) + " " + record.value(i).toString());
            }
            resultList.append(map);
		}

	} else {
		m_debugExec(QString("couldn't find model set of %1").arg(title), query);
	}
    return resultList;
}

QList<QPair<long, long>> AutocompleteDBManager::selectFrequentConnect(long setid, int max) {
	QList<QPair<long, long>> toList;
	QString queryStr = QString("SELECT id, to_module_id FROM connections "
		"WHERE module_id=%1 "
		"ORDER BY count DESC LIMIT %2").arg(setid).arg(max);
	QSqlQuery query(m_database);
    query.prepare(queryStr);
    //query.bindValue(0,title);
    if (query.exec()) {
		while(query.next()) {
            toList.append(QPair<long, long>(query.value(0).toLongLong(), query.value(1).toLongLong()));
		}
	} else {
		m_debugExec(QString("couldn't select frequent connect of %1").arg(setid), query);
	}

	return toList;
}

QList<QMap<QString, QVariant> *> AutocompleteDBManager::selectConnectionsByID(QList<long> ids) {
    QList<QMap<QString, QVariant> *> mapList;

    QString valueStr = "";
    QString orderStr = "";
    int ind = 1;
    foreach(long id, ids) {
        if (ind != 1) {
            valueStr += ",";
            orderStr += ",";
        }
        valueStr += QString("%1").arg(id);
        orderStr += QString("connection_id=%1 DESC").arg(id);
        ind++;
    }
    DebugDialog::debug(valueStr);
    DebugDialog::debug(orderStr);

    QString queryStr = QString("SELECT * FROM terminals_connections "
    "WHERE connection_id IN (%1) "
    "ORDER BY %2").arg(valueStr).arg(orderStr);

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    //query.bindValue(":values",valueStr);
    if (query.exec()) {
        while(query.next()) {
            QMap<QString, QVariant> * map = new QMap<QString, QVariant>();
            QSqlRecord record = query.record();
            for (int i=0; i<record.count(); i++) {
                map->insert(record.fieldName(i), record.value(i));
                //DebugDialog::debug("result " + record.fieldName(i) + " " + record.value(i).toString());
            }
            mapList.append(map);
		}

	} else {
        m_debugExec(QString("couldn't find connection of %1").arg(valueStr), query);
	}
    return mapList;


}

QMap<QString, QVariant> * AutocompleteDBManager::selectConnectionByID(long id) {
	
	QMap<QString, QVariant> * map = new QMap<QString, QVariant>();
    QString queryStr = QString("SELECT * FROM terminal_connections "
    "WHERE connection_id=%1 ").arg(id);

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    //query.bindValue(0,title);
    if (query.exec()) {
		if(query.next()) {
            QSqlRecord record = query.record();
            for (int i=0; i<record.count(); i++) {
                map->insert(record.fieldName(i), record.value(i));
                //DebugDialog::debug("result " + record.fieldName(i) + " " + record.value(i).toString());
            }
		}

	} else {
        m_debugExec(QString("couldn't find connection of %1").arg(id), query);
	}
    return map;


}

QList<QMap<QString, QVariant> *> AutocompleteDBManager::selectModelSetsByID(QList<long> ids) {
	
    QList<QMap<QString, QVariant> *> mapList;

    QString valueStr = "";
    QString orderStr = "";
    int ind = 1;
    foreach(long id, ids) {
        if (ind != 1) {
            valueStr += ",";
            orderStr += ",";
        }
        valueStr += QString("%1").arg(id);
        orderStr += QString("module_id=%1 DESC").arg(id);
        ind++;
    }
    DebugDialog::debug(valueStr);
    DebugDialog::debug(orderStr);

    QString queryStr = QString("SELECT * FROM modules_components "
    "WHERE module_id IN (%1) "
    "ORDER BY %2").arg(valueStr).arg(orderStr);

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    //query.bindValue(0,title);
    if (query.exec()) {
        while(query.next()) {
            QMap<QString, QVariant> * map = new QMap<QString, QVariant>();
            QSqlRecord record = query.record();
            for (int i=0; i<record.count(); i++) {
                map->insert(record.fieldName(i), record.value(i));
                //DebugDialog::debug("result " + record.fieldName(i) + " " + record.value(i).toString());
            }
            mapList.append(map);
		}

	} else {
        m_debugExec(QString("couldn't find model set of %1").arg(valueStr), query);
	}
    return mapList;
}

QMap<QString, QVariant> * AutocompleteDBManager::selectModelSetByID(long id) {
	QMap<QString, QVariant> * map = new QMap<QString, QVariant>();
    QString queryStr = QString("SELECT * FROM modules_components "
    "WHERE id=%1 ").arg(id);

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    //query.bindValue(0,title);
    if (query.exec()) {
		if(query.next()) {
            QSqlRecord record = query.record();
            for (int i=0; i<record.count(); i++) {
                map->insert(record.fieldName(i), record.value(i));
                //DebugDialog::debug("result " + record.fieldName(i) + " " + record.value(i).toString());
            }
		}

	} else {
        m_debugExec(QString("couldn't find model set of %1").arg(id), query);
	}
    return map;
}
/*
AutocompleteDBManager::selectNextSet(long setid) {
	SELECT ? FROM connections c
	INNER JOIN terminal_connections tc
	ON tc.connection_id=c.id
	ORDER BY c.count DESC, 
	QString queryStr = QString("SELECT * FROM %1 mc "
    "INNER JOIN %2 m "
    "ON mc.%3=m.id "
    "INNER JOIN %4 c "
    "on m.%5=c.id "
    "WHERE c.%6 = '%8' \n"
    "ORDER BY m.%7 DESC, m.id, mc.id").arg(modulescomponents["TABLE"]).arg(modules["TABLE"])
            .arg(modulescomponents["MODULE_ID"]).arg(components["TABLE"])
            .arg(modules["COMPONENT_ID"]).arg(components["TITLE"])
            .arg(modules["COUNT"]).arg(title);

}*/



void AutocompleteDBManager::deleteConnection() {
	QSqlDatabase::removeDatabase("SQLITE");
}

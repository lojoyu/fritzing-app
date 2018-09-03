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

typedef QPair<QString, QString> StringPair;

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

QList<QMap<QString, QVariant> *> AutocompleteDBManager::getModelSet(QString moduleID) {
	if (singleton == NULL) {
        singleton = new AutocompleteDBManager(defaultname);
	}
	return singleton->selectModelSet(moduleID);
	
}

QList<QPair<long, long>> AutocompleteDBManager::getFrequentConnect(long setid, int max) {
	if (singleton == NULL) {
		singleton = new AutocompleteDBManager(defaultname);
	}
    return singleton->selectFrequentConnect(setid, max);
}

QList<QPair<long, long>> AutocompleteDBManager::getFrequentConnect(long setid, int max, QList<QString> nameList) {
    if (singleton == NULL) {
        singleton = new AutocompleteDBManager(defaultname);
    }
    if (nameList.length() == 0) return singleton->selectFrequentConnect(setid, max);
    else return singleton->selectFrequentConnect(setid, max, nameList);
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

QList<QMap<QString, QVariant> *> AutocompleteDBManager::getConnectionsByModuleID(long mid, QList<long> mids) {
    if (singleton == NULL) {
        singleton = new AutocompleteDBManager(defaultname);
    }
    return singleton->selectConnectionsByModuleID(mid, mids);
}

QList<QMap<QString, QVariant> *> AutocompleteDBManager::getConnectionsBetweenModules(long mid1, long mid2, QList<QPair<QString, QString>> includePair) {
    if (singleton == NULL) {
        singleton = new AutocompleteDBManager(defaultname);
    }
    return singleton->selectConnectionsBetweenModules(mid1, mid2, includePair);
}

QList<QList<QString> *> AutocompleteDBManager::getTutorialList(QList<long> ids, int max) {
    if (singleton == NULL) {
        singleton = new AutocompleteDBManager(defaultname);
    }
    return singleton->selectTutorialList(ids, max);
}

/*
AutocompleteDBManager::getNextSet(QString title) {
	if (singleton == NULL) {
		singleton = new AutocompleteDBManager(defaultname);
	}
	return singleton->selectNextSet(title);
}*/


QList<QMap<QString, QVariant> *> AutocompleteDBManager::selectModelSet(QString moduleID) {
    
    QList<QMap<QString, QVariant> *> resultList;
    QString queryStr = QString("SELECT mc.* FROM %1 mc "
    "INNER JOIN %2 m "
    "ON mc.%3=m.id "
    "INNER JOIN %4 c "
    "on m.%5=c.id "
    "WHERE c.module_fid = '%7' \n"
    "ORDER BY m.%6 DESC, m.id, mc.id").arg(modulescomponents["TABLE"]).arg(modules["TABLE"])
            .arg(modulescomponents["MODULE_ID"]).arg(components["TABLE"])
            .arg(modules["COMPONENT_ID"])
            .arg(modules["COUNT"]).arg(moduleID);
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
        m_debugExec(QString("couldn't find model set of %1").arg(moduleID), query);
    }
    return resultList;
}

// QList<QMap<QString, QVariant> *> AutocompleteDBManager::selectModelSet(QString title) {
	
//     QList<QMap<QString, QVariant> *> resultList;
//     QString queryStr = QString("SELECT mc.* FROM %1 mc "
//     "INNER JOIN %2 m "
//     "ON mc.%3=m.id "
//     "INNER JOIN %4 c "
//     "on m.%5=c.id "
//     "WHERE c.%6 = '%8' \n"
//     "ORDER BY m.%7 DESC, m.id, mc.id").arg(modulescomponents["TABLE"]).arg(modules["TABLE"])
//             .arg(modulescomponents["MODULE_ID"]).arg(components["TABLE"])
//             .arg(modules["COMPONENT_ID"]).arg(components["TITLE"])
//             .arg(modules["COUNT"]).arg(title);
//     QSqlQuery query(m_database);
//     query.prepare(queryStr);
//     //query.bindValue(0,title);
//     if (query.exec()) {
// 		while(query.next()) {
//             QSqlRecord record = query.record();
//             QMap<QString, QVariant> * map = new QMap<QString, QVariant>();
//             for (int i=0; i<record.count(); i++) {
//                 map->insert(record.fieldName(i), record.value(i));
//                 //DebugDialog::debug("result " + record.fieldName(i) + " " + record.value(i).toString());
//             }
//             resultList.append(map);
// 		}

// 	} else {
// 		m_debugExec(QString("couldn't find model set of %1").arg(title), query);
// 	}
//     return resultList;
// }

QList<QPair<long, long>> AutocompleteDBManager::selectFrequentConnect(long setid, int max) {
	QList<QPair<long, long>> toList;
    QString queryStr = QString("SELECT id, to_module_id, sum(count) as sum FROM "
        "(SELECT * FROM connections WHERE module_id=%1 ORDER BY count) "
        "GROUP BY to_module_id ORDER BY sum DESC LIMIT %2").arg(setid).arg(max);

	// QString queryStr = QString("SELECT id, to_module_id FROM connections "
	// 	"WHERE module_id=%1 "
	// 	"ORDER BY count DESC LIMIT %2").arg(setid).arg(max);
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

QList<QPair<long, long>> AutocompleteDBManager::selectFrequentConnect(long setid, int max, QList<QString> nameList) {
    QList<QPair<long, long>> toList;

    QString valueStr = "";
    int ind = 1;
    foreach(QString name, nameList) {
        if (ind != 1) {
            valueStr += ",";
        }
        valueStr += QString("'%1'").arg(name);
        ind++;
    }
    QString queryStr = QString("SELECT id, to_module_id, sum(count) as sum FROM "
    "(SELECT * FROM connections "
    "WHERE module_id = %1 AND "
    "id NOT IN "
    "(SELECT DISTINCT(connection_id) FROM terminals_connections "
    "WHERE terminal_id IN (%2)) ORDER BY count)"
    "GROUP BY to_module_id ORDER BY sum DESC LIMIT %3").arg(setid).arg(valueStr).arg(max);

    // QString queryStr = QString("SELECT id, to_module_id FROM connections "
    // "WHERE module_id = %1 AND "
    // "id NOT IN "
    // "(SELECT DISTINCT(connection_id) FROM terminals_connections "
    // "WHERE terminal_id IN (%2)) "
    // "ORDER BY count DESC, id LIMIT %3").arg(setid).arg(valueStr).arg(max);
    DebugDialog::debug(QString("select frequent connect %1").arg(queryStr));

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
        orderStr += QString("mc.module_id=%1 DESC").arg(id);
        ind++;
    }
    DebugDialog::debug(valueStr);
    DebugDialog::debug(orderStr);

//    QString queryStr = QString("SELECT * FROM modules_components "
//    "WHERE module_id IN (%1) "
//    "ORDER BY %2").arg(valueStr).arg(orderStr);

    QString queryStr = QString("SELECT c.module_fid, mc.* FROM modules_components mc "
    "INNER JOIN modules m ON m.id=mc.module_id "
    "INNER JOIN components c ON c.id=m.component_id "
    "WHERE mc.module_id IN (%1) "
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

QList<QMap<QString, QVariant> *> AutocompleteDBManager::selectConnectionsByModuleID(long mid, QList<long> mids) {
    QList<QMap<QString, QVariant> *> mapList;

    QString valueStr = "";
    int ind = 1;
    foreach(long id, mids) {
        if (ind != 1) {
            valueStr += ",";
        }
        valueStr += QString("%1").arg(id);
        ind++;
    }
    
    QString queryStr = QString("SELECT c.*, tc.* FROM terminals_connections tc "
    "INNER JOIN connections c "
    "ON c.id = tc.connection_id "
    "WHERE c.module_id = %1 AND c.to_module_id IN (%2) "
    "ORDER BY c.count DESC, c.id").arg(mid).arg(valueStr);

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

QList<QMap<QString, QVariant> *> AutocompleteDBManager::selectConnectionsBetweenModules(long mid1, long mid2, QList<QPair<QString, QString>> includePair) {

    QList<QMap<QString, QVariant> *> mapList;

    QString valueStr = "";
    int ind = 1;
    // foreach(StringPair s, includePair) {
    //     if (ind != 1) {
    //         valueStr += ",";
    //     }
    //     valueStr += QString("('%1', '%2')").arg(s.first).arg(s.second);
    //     ind++;
    // }
    foreach(StringPair s, includePair) {
        if (ind != 1) {
            valueStr += " OR ";
        } else {
            valueStr += "WHERE ";
        }
        valueStr += QString("terminal_id='%1' AND to_terminal_id='%2'").arg(s.first).arg(s.second);
        ind++;
    }

    QString queryStr = QString("SELECT c.*, tc.* FROM terminals_connections tc "
    "INNER JOIN connections c "
    "ON c.id = tc.connection_id AND c.id IN "
    "(SELECT DISTINCT(connection_id) FROM terminals_connections "
    "%3) "
    "WHERE c.module_id = %1 AND c.to_module_id = %2 "
    "ORDER BY c.count DESC, c.id").arg(mid1).arg(mid2).arg(valueStr);

    // queryStr = QString("SELECT DISTINCT(connection_id) FROM terminals_connections "
    //  "WHERE (terminal_id, to_terminal_id) IN (VALUES%1)").arg(valueStr);

    // (SELECT DISTINCT(tc.connection_id) FROM terminals_connections tc 
    // JOIN (VALUES %3) AS x (terminal_id, to_terminal_id)
    // ON  x.terminal_id = tc.terminal_id AND x.to_terminal_id = tc.to_terminal_id) ;

    // select id1 + id2 as FullKey, *
    // from players
    // where FullKey in ('11','12','13')

    DebugDialog::debug(QString("queryString : %1").arg(queryStr));

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

QList<QList<QString> *> AutocompleteDBManager::selectTutorialList(QList<long> ids, int max) {
    QList<QList<QString> *> tutorialList;

    QString valueStr = "";
    QString orderStr = "";
    int ind = 1;
    foreach(long id, ids) {
        if (ind != 1) {
            valueStr += ",";
            orderStr += ",";
        }
        valueStr += QString("%1").arg(id);
        orderStr += QString("ct.connection_id=%1 DESC").arg(id);
        ind++;
    }
    DebugDialog::debug(valueStr);
    DebugDialog::debug(orderStr);

    QString queryStr = QString("SELECT ct.connection_id, tl.link FROM tutorial_link tl "
    "INNER JOIN connections_tutorial ct "
    "ON tl.id = ct.tutorial_id AND ct.connection_id IN (%1) "
    "ORDER BY %2").arg(valueStr).arg(orderStr);

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    //query.bindValue(":values",valueStr);
    QList<QString>* tList;
    long prev = -1;
    if (query.exec()) {
        while(query.next()) {
            long id = query.value(0).toLongLong();
            if (prev == -1 || prev != id) {
                tList = new QList<QString>();
                tutorialList.append(tList);
                prev = id;
            }
            tList->append(query.value(1).toString());
        }

    } else {
        m_debugExec(QString("couldn't find connection of %1").arg(valueStr), query);
    }
    return tutorialList;



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

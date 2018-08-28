#ifndef AUTOCOMPLETEDBMANAGER_H
#define AUTOCOMPLETEDBMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QApplication>

class AutocompleteDBManager {

	public:
		AutocompleteDBManager(const QString& databasename);
		~AutocompleteDBManager();
		static void loadDB(const QString & databasemname);
        static QList<QMap<QString, QVariant> *> getModelSet(QString title);
        static QList<QPair<long, long>> getFrequentConnect(long setid, int max);
        static QList<QPair<long, long>> getFrequentConnect(long setid, int max, QList<QString> nameList);
	    static QList<QMap<QString, QVariant> *> getConnectionsByID(QList<long> ids);
	   	static QList<QMap<QString, QVariant> *> getModelSetsByID(QList<long> ids);
	    static QMap<QString, QVariant> * getConnectionByID(long id);
		static QMap<QString, QVariant> * getModelSetByID(long id);
		static QList<QMap<QString, QVariant> *> getConnectionsByModuleID(long mid, QList<long> mids);
		static QList<QMap<QString, QVariant> *> getConnectionsBetweenModules(long mid1, long mid2, QList<QPair<QString, QString>> includePair);


	protected:
        QList<QMap<QString, QVariant> *> selectModelSet(QString title);
        QList<QMap<QString, QVariant> *> selectConnectionsByID(QList<long> ids);
        QMap<QString, QVariant> * selectConnectionByID(long id);
        QList<QMap<QString, QVariant> *> selectModelSetsByID(QList<long> ids);
        QMap<QString, QVariant> * selectModelSetByID(long id);
		QList<QPair<long, long>> selectFrequentConnect(long setid, int max);
        QList<QPair<long, long>> selectFrequentConnect(long setid, int max, QList<QString> nameList);
		QList<QMap<QString, QVariant> *> selectConnectionsByModuleID(long mid, QList<long> mids);
		QList<QMap<QString, QVariant> *> selectConnectionsBetweenModules(long mid1, long mid2, QList<QPair<QString, QString>> includePair);


		void deleteConnection();


	protected:
		static AutocompleteDBManager* singleton;
		QSqlDatabase m_database;

};

#endif

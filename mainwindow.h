#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QDebug>
#include <QRegularExpression>
#include <QtSql>
#include <QHash>
#include <QJsonDocument>
#include <QTextEdit>
#include <QTreeWidgetItem>
#include <QHash>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	static QTextEdit * s_textEdit;
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private:
	struct aliOptions {
		QString optionName;
		QString optionValue;
	};
	struct aliOrder {
		QString orderDetails;
		float orderPrice;
		uint orderQuantity;
		QVector<aliOptions> options;
	};
	QHash<QString, QVector<aliOrder>> aliOrders;
	struct part
	{
		part() {}
		QString name;
		QString description;
		QString type;
		QString footprint;
		int footprintID;
		int stock;
		QString locationStr;
		int location;
		QString toString() {return QString("name:%0 description:%1 type:%2 footprint:%3 stock:%4 locationstr:%5 location:%6").arg(name).arg(description).arg(type).arg(footprint).arg(stock).arg(locationStr).arg(location																																																		  );};
	};
	Ui::MainWindow *ui;
	QSettings settings;
	QList<part> getPartsFromCSV(QString file);
	QHash<int, QString> getLocationsFromDB();
	int getOrCreateAliDistributerID();
	int getOrCreateAliLocationID();
	QHash<int ,QString> getFootprintsFromDB();
	bool insertPartsToDB(QList<part> parts);
	bool insertFootprintsToDB(QStringList footprints);
	bool insertLocationsToDB(QStringList locations);
	struct category {
		int id;
		QString name;
		int rootID;
		QString description;
	};
	QStringList parseCsvFields(const QString &line, const QChar delimiter);
	QList<category> getCategoriesFromDB();
	bool insertCategoriesToDB(QList<category> *categories);
	bool updateCategoriesToDB(QList<category> *categories);
	int parseCategory(int level, QList<category> *categories, QJsonObject obj);
	int main();
	int createDefaultCategories();
	bool importFromCSV();
	QSqlDatabase db;
	int loadDefaultCategories();
	void loadTableFromCSV();
	void populateCategoryTree(QTreeWidgetItem *parentItem, category cat, QList<category> *categories);
	QList<category> defaultCategories;
	QList<part> newParts;
	QStringList orderIds;
private slots:
	void onConnectClicked();
	void onDisconnectClicked();
	void on_tabWidget_currentChanged(int index);
	void on_sendToDatabaseButton_clicked();
	void on_openFileButton_clicked();
	void on_sendPartsToDatabase_clicked();
	void on_fetchOrders_clicked();
	void webloadprogress(int);
	void openAliLink(int, int);
	void on_aliImport_clicked();
};
#endif // MAINWINDOW_H

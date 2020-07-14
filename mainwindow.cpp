#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QWebEngineProfile>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	QCoreApplication::setOrganizationName("JB");
	QCoreApplication::setOrganizationDomain("jbtecnologia.com");
	QCoreApplication::setApplicationName("partkeeprTools");
	QSettings settings;
	ui->setupUi(this);
	s_textEdit = new QTextEdit;
	s_textEdit->setParent(ui->centralwidget);
	ui->centralwidget->layout()->addWidget(s_textEdit);
	ui->centralwidget->layout()->removeWidget(ui->centralwidget);
	ui->textEdit->deleteLater();
	s_textEdit->setFixedHeight(100);
	QPalette p = s_textEdit->palette();
	p.setColor(QPalette::Base, Qt::black);
	p.setColor(QPalette::Text, Qt::white);
	s_textEdit->setPalette(p);
	ui->name->setText(settings.value("db_name", "").toString());
	ui->hostname->setText(settings.value("db_hostname", "").toString());
	ui->username->setText(settings.value("db_username", "").toString());
	ui->password->setText(settings.value("db_password", "").toString());
	ui->autoconnect->setChecked(settings.value("db_autoconnect", false).toBool());
	ui->savecreds->setChecked(settings.value("db_savecreds", false).toBool());
	ui->aliuser->setText(settings.value("ali_name", "").toString());
	ui->alipass->setText(settings.value("ali_pass", "").toString());
	ui->alisave->setChecked(settings.value("ali_save", "").toBool());

	connect(ui->connect, &QPushButton::pressed, this, &MainWindow::onConnectClicked);
	connect(ui->disconnect, &QPushButton::pressed, this, &MainWindow::onDisconnectClicked);

	connect(ui->m_view, &QWebEngineView::loadProgress, this, &MainWindow::webloadprogress);

	connect(ui->aliTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::openAliLink);
	db = QSqlDatabase::addDatabase("QMYSQL");
	if(ui->autoconnect->isChecked())
		onConnectClicked();
	qDebug() << "Application started";
}

MainWindow::~MainWindow()
{
	if(ui->savecreds->isChecked()) {
		QSettings settings;
		settings.setValue("db_name", ui->name->text());
		settings.setValue("db_hostname", ui->hostname->text());
		settings.setValue("db_username", ui->username->text());
		settings.setValue("db_password", ui->password->text());
		settings.setValue("db_autoconnect", ui->autoconnect->isChecked());
		settings.setValue("db_savecreds", ui->savecreds->isChecked());
	}
	if(ui->alisave->isChecked()) {
		QSettings settings;
		settings.setValue("ali_name", ui->aliuser->text());
		settings.setValue("ali_pass", ui->alipass->text());
		settings.setValue("ali_save", ui->alisave->isChecked());
	}
	db.close();
	delete ui;
}

void MainWindow::onConnectClicked()
{
	db.setHostName(ui->hostname->text());
	db.setDatabaseName(ui->name->text());
	db.setUserName(ui->username->text());
	db.setPassword(ui->password->text());
	bool ok = db.open();
	if(ok)
		ui->db_results->setText("Connected successfully");
	else
		ui->db_results->setText(QString("ERROR:%0").arg(db.lastError().text()));
}

void MainWindow::onDisconnectClicked()
{
	db.close();
	ui->db_results->setText("Disconnected");
}

bool MainWindow::insertCategoriesToDB(QList<category> *categories){
	bool ret = true;
	QSqlQuery query;
	query.prepare("INSERT INTO PartCategory (name, description, lft, rgt, lvl) "
				  "VALUES (:name, :description, :lft, :rgt, :lvl)");
	foreach (category f, *categories) {
		query.bindValue(":name", f.name);
		query.bindValue(":description", f.description);
		query.bindValue(":lft", 1);
		query.bindValue(":rgt", 2);
		query.bindValue(":lvl", 0);

		bool r = query.exec();
		if(!r)
			qDebug() << "Error inserting category " << f.name << ">>" << query.lastError();
		ret = ret && r;	}
	return ret;
}

bool MainWindow::updateCategoriesToDB(QList<category> *categories){
	bool ret = true;
	QSqlQuery query;
	query.prepare("UPDATE PartCategory SET parent_id=:parent_id WHERE id=:id");
	foreach (category f, *categories) {
		query.bindValue(":id", f.id);
		query.bindValue(":parent_id", f.rootID);
		bool r = query.exec();
		if(!r)
			qDebug() << "Error updating category " << f.name << ">>" << query.lastError();
		ret = ret && r;
	}
	return ret;
}

int MainWindow::parseCategory(int level, QList<category> *categories, QJsonObject obj) {
	int ilevel = level;
	category c;
	c.rootID = obj.value("parent").toString().remove("\/api\/part_categories\/").toInt();
	c.id = obj.value("@id").toString().remove("\/api\/part_categories\/").toInt();
	c.name = obj.value("name").toString();
	c.description = obj.value("description").toString();
	categories->append(c);
	obj.value("children");
	QJsonArray catArr = obj.value("children").toArray();
	int newlevel;
	foreach (const QJsonValue & value, catArr) {
		newlevel = parseCategory(ilevel + 1, categories, value.toObject());
		if(newlevel > level) {
			level = newlevel;
		}
	}
	return level;
}
void MainWindow::populateCategoryTree(QTreeWidgetItem *parentItem, category c, QList<category> *categories) {
	QTreeWidgetItem *item = nullptr;
	if(!parentItem) {
		item = new QTreeWidgetItem(ui->treeWidget);
		foreach(category cc, *categories) {
			if(cc.rootID == 0) {
				c = cc;
				break;
			}
		}
	}
	else
		item = new QTreeWidgetItem(parentItem);
	item->setText(0, c.name);
	foreach (category cc, *categories) {
		if(cc.rootID == c.id)
			populateCategoryTree(item, cc, categories);
	}
}

int MainWindow::main() {
	db = QSqlDatabase::addDatabase("QMYSQL");
	db.setHostName("127.0.0.1");
	db.setDatabaseName("symfony");
	db.setUserName("symfony");
	db.setPassword("symfony");
	bool ok = db.open();
	QList<part> parts = getPartsFromCSV("/home/jose/Downloads/partsbox-parts.csv");
	qDebug() << parts.count();
	return 0;
}

int MainWindow::loadDefaultCategories() {
	QFile file(":/categories.json");
	if (!file.open(QIODevice::ReadOnly)) {
		qDebug() << "Error loading default categories from file:" << file.errorString();
		return -1;
	}
	QJsonDocument categories = QJsonDocument::fromJson(file.readAll());
	QJsonObject catObj = categories.object();
	QList<category> cats;
	int levels = parseCategory(0, &cats, catObj);
	qDebug() << "Default category levels:" << levels;
	ui->treeWidget->setColumnCount(1);
	category c;
	populateCategoryTree(nullptr, c, &cats);
	defaultCategories = cats;
	return 0;
}

void MainWindow::loadTableFromCSV()
{
	QList<part> parts = getPartsFromCSV(ui->filename->text());
	ui->tableWidget->clear();
	ui->tableWidget->setRowCount(parts.length());
	ui->tableWidget->setColumnCount(4);
	ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "Name" << "Description" << "Stock" << "Location");
	for (auto r=0; r < parts.length(); r++) {
		ui->tableWidget->setItem( r, 0, new QTableWidgetItem(parts.at(r).name));
		ui->tableWidget->setItem( r, 1, new QTableWidgetItem(parts.at(r).description));
		ui->tableWidget->setItem( r, 2, new QTableWidgetItem(QString::number(parts.at(r).stock)));
		ui->tableWidget->setItem( r, 3, new QTableWidgetItem(parts.at(r).locationStr));
	}
	newParts = parts;
}
int MainWindow::createDefaultCategories()
{
	QList<category> catsDB = getCategoriesFromDB();
	QList<category> missingCats;
	foreach (category c, defaultCategories) {
		bool found = false;
		foreach (category cdb, catsDB) {
			if(c.name == cdb.name) {
				found = true;
				break;
			}
		}
		if(!found) {
			missingCats.append(c);
		}
	}
	qDebug() << "New categories found:" << missingCats.length();
	foreach (category a, missingCats) {
		qDebug() << a.name;
	}
	if(missingCats.length() == 0)
		return 0;
	qDebug() << "insert parts result" << insertCategoriesToDB(&missingCats);
	catsDB = getCategoriesFromDB();
	QList<category> catsFixed;
	foreach (category orig, defaultCategories) {
		QString name = orig.name;
		QString parentName;
		foreach (category ob, defaultCategories) {
			if(ob.id == orig.rootID) {
				parentName = ob.name;
			}
		}
		int id, parentID;
		foreach (category c, catsDB) {
			if(c.name == parentName)
				parentID = c.id;
			if(c.name == name)
				id = c.id;
		}
		category n = orig;
		n.id = id;
		n.rootID = parentID;
		catsFixed.append(n);
	}
	qDebug() << "category indexes needing fix:" << catsFixed.length();
	qDebug() << "update categories result:" << updateCategoriesToDB(&catsFixed);
	return 0;
}

bool MainWindow::importFromCSV()
{
	QHash<int, QString> locs = getLocationsFromDB();
	QHash<int, QString> foots = getFootprintsFromDB();

	QStringList newLocations;
	foreach (part p, newParts) {
		bool found = false;
		foreach (QString s, locs.values()) {
			if(p.locationStr.trimmed() == s.trimmed()) {
				found = true;
				break;
			}
		}
		if(!found) {
			if(!newLocations.contains(p.locationStr))
				newLocations.append(p.locationStr);
		}
	}

	QStringList newFootprints;
	foreach (part p, newParts) {
		bool found = false;
		//qDebug() << p.toString();
		foreach (QString s, foots.values()) {
			if(p.footprint.trimmed() == s.trimmed()) {
				found = true;
				break;
			}
		}
		if(!found) {
			if(!newFootprints.contains(p.footprint))
				newFootprints.append(p.footprint);
		}
	}
	qDebug() << "New locations" << newLocations;
	qDebug() << "Insert locations result" << insertLocationsToDB(newLocations);
	qDebug() << "New footprints" << newFootprints;
	qDebug() << "Insert footprints result" << insertFootprintsToDB(newFootprints);

	locs = getLocationsFromDB();
	foots = getFootprintsFromDB();

	for(int x = 0; x < newParts.length(); ++x) {
		int id = locs.key(newParts.at(x).locationStr);
		int foot = foots.key(newParts.at(x).footprint);
		if(newParts.at(x).footprint.isEmpty())
			foot = -1;
		newParts[x].location = id;
		newParts[x].footprintID = foot;
		//qDebug() << parts.at(x).location << parts.at(x).locationStr << parts.at(x).footprintID << parts.at(x).footprint;
	}
	qDebug() << "Insert parts result" << insertPartsToDB(newParts);
	return 0;
}
QStringList MainWindow::parseCsvFields(const QString &line, const QChar delimiter)
{
	QString temp = line;
	QString field;
	QStringList field_list;
	QString regex = "(?:^|,)(\"(?:[^\"]+|\"\")*\"|[^,]*)";
	regex.replace(",", delimiter);
	QRegularExpression re(regex);
	if (temp.right(1) == "\n") temp.chop(1);
	QRegularExpressionMatchIterator it = re.globalMatch(temp);
	while (it.hasNext())
	{
		QRegularExpressionMatch match = it.next();
		if (match.hasMatch())
		{
			field = match.captured(1);
			if (field.left(1) == "\"" && field.right(1) == "\"")
				field = field.mid(1, field.length()-2);
			field_list.push_back(field);
		}
	}
	return field_list;
}

QList<MainWindow::part> MainWindow::getPartsFromCSV(QString filename) {
	QList<part> parts;
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly)) {
		qDebug() << file.errorString();
		return parts;
	}

	QStringList wordList;
	bool once = false;
	while (!file.atEnd()) {
		QByteArray line = file.readLine();
		if(!once) {
			once = true;
			continue;
		}
		QStringList a = parseCsvFields(QString(line), ',');

		if(a.size() < 6) {
			qDebug() << "ERROR parsing " << line;
			continue;
		}
		part p;
		p.name = a.at(0);
		p.description = a.at(1);
		p.type = a.at(2);
		p.footprint = a.at(3);
		QStringList loc = a.at(5).split(")");
		loc.removeAll("");
		for (int x = 0; x < loc.length(); ++x) {
			loc[x].remove(QRegExp("^,"));
			loc[x] = loc[x].trimmed();
			QStringList l = loc[x].split("(");
			p.locationStr = l.at(0).trimmed();
			p.stock = l.at(1).toInt();
			qDebug() << "CSV:" << p.name << p.description;
			parts.append(p);
		}
	}
	return parts;
}

QHash<int, QString> MainWindow::getLocationsFromDB() {
	QHash<int, QString> locations;
	QSqlQuery query;
	query.exec("SELECT * FROM `StorageLocation` WHERE 1");
	while (query.next()) {
		int id = query.value(0).toInt();
		int cat_id = query.value(1).toInt();
		QString name = query.value(2).toString();
		qDebug() << id << cat_id << name;
		locations.insert(id, name);
	}
	return locations;
}

int MainWindow::getOrCreateAliDistributerID()
{
	QSqlQuery query;
	query.exec("SELECT id FROM `Distributor` WHERE name LIKE 'Aliexpress'");
	if(query.next()) {
		return  query.value(0).toUInt();
	}
	query.exec("INSERT INTO `Distributor` (name) VALUES('Aliexpress')");
	return query.lastInsertId().toUInt();
}

int MainWindow::getOrCreateAliLocationID()
{
	QSqlQuery query;
	query.exec("SELECT id FROM `StorageLocation` WHERE name LIKE 'Aliexpress'");
	if(query.next()) {
		return  query.value(0).toUInt();
	}
	query.exec("INSERT INTO `StorageLocation` (name) VALUES('Aliexpress')");
	return query.lastInsertId().toUInt();
}

QList<MainWindow::category> MainWindow::getCategoriesFromDB() {
	QList<category> cats;
	QSqlQuery query;
	query.exec("SELECT * FROM `PartCategory` WHERE 1");
	category c;
	while (query.next()) {
		c.id = query.value(0).toInt();
		c.rootID = query.value(1).toInt();
		c.name = query.value(6).toString();
		c.description = query.value(7).toString();
		cats.append(c);
	}
	return cats;
}
QHash<int, QString> MainWindow::getFootprintsFromDB() {
	QHash<int, QString> footprints;
	QSqlQuery query;
	query.exec("SELECT * FROM `Footprint` WHERE 1");
	while (query.next()) {
		int id = query.value(0).toInt();
		int cat_id = query.value(1).toInt();
		QString name = query.value(2).toString();
		qDebug() << id << cat_id << name;
		footprints.insert(id, name);
	}
	return footprints;
}

bool MainWindow::insertPartsToDB(QList<part> parts){
	bool ret = true;
	QSqlQuery query;
	bool q = query.prepare("INSERT INTO Part (category_id, lowStock, removals, needsReview, averagePrice, name, description, comment, stockLevel, storageLocation_id, footprint_id, minStockLevel) "
						   "VALUES (:category_id, :lowStock, :removals, :needsReview, :averagePrice, :name, :description, :comment, :stockLevel, :storageLocation_id, :footprint_id, :minStockLevel)");
	if(!q)
		qDebug() << "Problem setting query";
	foreach (part p, parts) {
		query.bindValue(":name", p.name);
		query.bindValue(":stockLevel", p.stock);
		query.bindValue(":storageLocation_id", p.location);
		if(p.footprintID == -1)
			query.bindValue(":footprint_id", QVariant(QVariant::String));
		else
			query.bindValue(":footprint_id", p.footprintID);
		query.bindValue(":description", p.description);
		query.bindValue(":comment", "Imported from partsbox");
		query.bindValue(":minStockLevel", 0);
		query.bindValue(":averagePrice", 0);
		query.bindValue(":needsReview", 0);
		query.bindValue(":removals", 0);
		query.bindValue(":lowStock", 0);
		query.bindValue(":category_id", 1);
		bool r = query.exec();
		if(!r)
			qDebug() << "DB error:" << query.lastError();
		ret = ret && r;
	}
	return ret;
}

bool MainWindow::insertFootprintsToDB(QStringList footprints){
	bool ret = true;
	QSqlQuery query;
	query.prepare("INSERT INTO Footprint (name, description) ""VALUES (:name, :description)");
	foreach (QString f, footprints) {
		query.bindValue(":name", f);
		query.bindValue(":description", "imported from partsbox");
		ret = ret && query.exec();
	}
	return ret;
}

bool MainWindow::insertLocationsToDB(QStringList locations){
	bool ret = true;
	QSqlQuery query;
	query.prepare("INSERT INTO Footprint (name) ""VALUES (:name)");
	foreach (QString f, locations) {
		query.bindValue(":name", f);
		ret = ret && query.exec();
	}
	return ret;
}

QTextEdit * MainWindow::s_textEdit = 0;

void MainWindow::on_tabWidget_currentChanged(int index)
{
	if(index == 1)
		loadDefaultCategories();
}

void MainWindow::on_sendToDatabaseButton_clicked()
{
	createDefaultCategories();
}

void MainWindow::on_openFileButton_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open File"), "", "All Files (*.*);;CSV files (*.csv)");
	ui->filename->setText(fileName);
	loadTableFromCSV();
}

void MainWindow::on_sendPartsToDatabase_clicked()
{
	importFromCSV();
}

void MainWindow::on_fetchOrders_clicked()
{
	ui->fetchOrders->setEnabled(false);
	orderIds.clear();
	QEventLoop loop;
	QEventLoop loop2;
	QTimer timer;
	timer.setSingleShot(true);
	connect(ui->m_view, &QWebEngineView::loadFinished, &loop, &QEventLoop::quit);
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	connect(&timer, &QTimer::timeout, &loop2, &QEventLoop::quit);
	ui->m_view->load(QUrl("https://login.aliexpress.com/?returnUrl=https%3A%2F%2Ftrade.aliexpress.com%2ForderList.htm"));
	timer.start(10000);
	loop.exec();
	timer.stop();
	ui->m_view->page()->runJavaScript(QString("document.getElementById('fm-login-id').value = '%1'").arg(ui->aliuser->text()));
	ui->m_view->page()->runJavaScript(QString("document.getElementById('fm-login-password').value = '%1'").arg(ui->alipass->text()));
	ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('fm-submit')[0].click()"));
	timer.start(3000);
	loop2.exec();
	timer.stop();
	timer.start(10000);
	loop.exec();
	timer.stop();
	qDebug() << "Loading finished orders";
	ui->m_view->page()->runJavaScript(QString("document.getElementById('order-status').value = 'FINISH'"));
	ui->m_view->page()->runJavaScript(QString("document.getElementById('search-btn').click()"));
	timer.start(10000);
	loop.exec();
	timer.stop();

	qDebug() << "Loading order details";
	bool ordersleft = true;
	int page=1;
	int maxorders = ui->xOrders->value();
	int currentorders = 0;
	while (ordersleft && page<=50) {
		ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('order-item-wraper').length"),
												  [this, &loop, &ordersleft, &page, maxorders, &currentorders](const QVariant& res) {
			int items = res.toInt();
			for(int x = 0; x < items; ++x) {
				qDebug() << x;
				ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('order-item-wraper')[%0].querySelector('.order-info .first-row .info-body').textContent").arg(x),
														  [this](const QVariant& ress) {
					this->orderIds.append(ress.toString());
					qDebug() << ress.toString();
				});
				++currentorders;
				if(currentorders == maxorders) {
					ordersleft = false;
					break;
				}
			}
			loop.quit();
			++page;
		});
		loop.exec();
		if(ordersleft) {
			ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('ui-pagination-next')[0].click()"));
			timer.start(10000);
			loop.exec();
			timer.stop();
		}
	}
	timer.start(1000);
	loop.exec();
	qDebug() << orderIds;
	aliOrders.empty();
	foreach (QString s, orderIds) {
		ui->m_view->load(QUrl(QString("https://trade.aliexpress.com/order_detail.htm?orderId=%0").arg(s)));
		timer.start(10000);
		loop.exec();
		timer.stop();
		ui->m_view->page()->runJavaScript(QString("document.querySelectorAll('.order-bd[id]').length"),
												  [this, s](const QVariant& res) {
			aliOrders.insert(s, QVector<aliOrder>(res.toUInt()));
			for(int x = 0; x < res.toInt(); ++x) {
				ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('order-bd')[%0].hasAttribute('id')").arg(x),
											  [this, x, s](const QVariant& res) {
					if(res.toBool()) {
						ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('order-bd')[%0].querySelector('.baobei-name').textContent").arg(x),
															  [this, x, s](const QVariant& res) {
							qDebug() << "Name:" << x << res.toString().trimmed();
							aliOrders[s][x].orderDetails = res.toString().trimmed();

						});
						ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('order-bd')[%0].querySelector('.price').textContent").arg(x),
															  [this, x, s](const QVariant& res) {
							qDebug() << "Price:" << x << res.toString().trimmed();
							QRegExp rx("(\\d+\\.?\\d+)");
							rx.indexIn(res.toString().replace(',','.'));
							QStringList list = rx.capturedTexts();
							aliOrders[s][x].orderPrice = list.at(0).toFloat();
						});
						ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('order-bd')[%0].querySelector('.quantity').textContent").arg(x),
															  [this, x, s](const QVariant& res) {
							qDebug() << "Quantity:" << x << res.toString().trimmed();
							QRegExp rx("(\\d+)");
							rx.indexIn(res.toString());
							QStringList list = rx.capturedTexts();
							aliOrders[s][x].orderQuantity = list.at(0).toUInt();
						});
						ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('order-bd')[%0].getElementsByClassName('spec').length").arg(x),
															  [this, x, s](const QVariant& res) {
							aliOrders[s][x].options = QVector<aliOptions>(res.toUInt());
							for(uint y = 0; y < res.toUInt(); ++y) {
								ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('order-bd')[%0].getElementsByClassName('spec')[%1].getElementsByTagName('font')[0].textContent").arg(x).arg(y),
																	  [this, x, y, s](const QVariant& res) {
									qDebug() << "SPEC:" << x << y << res.toString().trimmed();
									aliOrders[s][x].options[y].optionName = res.toString().trimmed();
								});
								ui->m_view->page()->runJavaScript(QString("document.getElementsByClassName('order-bd')[%0].getElementsByClassName('spec')[%1].querySelector('.val').textContent").arg(x).arg(y),
																	  [this, x, y, s](const QVariant& res) {
									qDebug() << "VALUE:" << x << y << res.toString().trimmed();
									aliOrders[s][x].options[y].optionValue = res.toString().trimmed();
								});
							}
						});
					}
				});
			}
		});

	}
	timer.start(1000);
	loop.exec();
	timer.stop();
	ui->aliTable->clearContents();
	uint count = 0;
	foreach(QVector<aliOrder> o, aliOrders.values()) {
		count = count + o.count();
	}
	uint currentIndex = 0;
	qDebug() << "Total items:" << count;
	foreach(QString n, aliOrders.keys()) {
		qDebug() << "BEGIN ORDER";
		qDebug() << "ID:" << n;
		ui->aliTable->setRowCount(count);
		QVector<aliOrder> oo = aliOrders.value(n);
		foreach (aliOrder o, oo) {
			ui->aliTable->setItem(currentIndex, 0, new QTableWidgetItem(n));
			ui->aliTable->setItem(currentIndex, 1, new QTableWidgetItem(o.orderDetails));
			ui->aliTable->setItem(currentIndex, 2, new QTableWidgetItem(QString::number(o.orderQuantity)));
			ui->aliTable->setItem(currentIndex, 4, new QTableWidgetItem(QString::number(o.orderPrice)));
			qDebug() << "Details:" << o.orderDetails;
			qDebug() << "Price:" << o.orderPrice;
			qDebug() << "Quantity:" << o.orderQuantity;
			QString options;
			foreach (aliOptions op, o.options) {
				qDebug() << "OPTION Name" << op.optionName;
				options.append(op.optionName + "=");
				qDebug() << "OPTION Value" << op.optionValue;
				options.append(op.optionValue + ";");
			}
			ui->aliTable->setItem(currentIndex, 3, new QTableWidgetItem(options));
			QCheckBox *b = new QCheckBox(this);
			b->setChecked(true);
			ui->aliTable->setCellWidget(currentIndex, 5, b);
			++currentIndex;
		}
	}
	ui->fetchOrders->setEnabled(true);

}

void MainWindow::webloadprogress(int p)
{
	ui->progressBar->setValue(p);
}

void MainWindow::openAliLink(int x, int y)
{
	if(y != 0)
		return;
	QDesktopServices::openUrl(QUrl(QString("https://trade.aliexpress.com/order_detail.htm?orderId=%0").arg(ui->aliTable->itemAt(x, y)->text())));
}

void MainWindow::on_aliImport_clicked()
{
	bool ret;
	QSqlQuery query;
	int aliStorage = getOrCreateAliLocationID();
	int aliDist = getOrCreateAliDistributerID();
	bool q = query.prepare("INSERT INTO Part (category_id, lowStock, removals, needsReview, averagePrice, name, description, comment, stockLevel, storageLocation_id, footprint_id, minStockLevel) "
						   "VALUES (:category_id, :lowStock, :removals, :needsReview, :averagePrice, :name, :description, :comment, :stockLevel, :storageLocation_id, :footprint_id, :minStockLevel)");
	if(!q)
		qDebug() << "Problem setting query";
	for (int x = 0; x < ui->aliTable->rowCount(); ++x) {
		if(qobject_cast<QCheckBox*>(ui->aliTable->cellWidget(x, 5))->isChecked()) {
		query.bindValue(":name", ui->aliTable->item(x, 1)->text());
		query.bindValue(":stockLevel", ui->aliTable->item(x, 2)->text());
		query.bindValue(":storageLocation_id", QString::number(aliStorage));
		QString com = QString("Order:%0 Options:%1").arg(ui->aliTable->item(x, 0)->text()).arg(ui->aliTable->item(x, 3)->text());
		query.bindValue(":comment", com);
		query.bindValue(":minStockLevel", 0);
		query.bindValue(":averagePrice", 0);
		query.bindValue(":needsReview", 0);
		query.bindValue(":removals", 0);
		query.bindValue(":lowStock", 0);
		query.bindValue(":category_id", 1);
		bool r = query.exec();
		if(!r)
			qDebug() << "DB error:" << query.lastError();
		int pID = query.lastInsertId().toUInt();
		q = query.prepare("INSERT INTO PartDistributor (part_id, distributor_id, price, orderNumber, packagingUnit) "
							   "VALUES (:part_id, :distributor_id, :price, :orderNumber, :packagingUnit)");
		query.bindValue(":part_id", pID);
		query.bindValue(":distributor_id", aliDist);
		query.bindValue(":name", ui->aliTable->item(x, 0)->text());
		query.bindValue(":price", ui->aliTable->item(x, 4)->text());
		query.bindValue(":packagingUnit", 1);
		r = query.exec();
		if(!r)
			qDebug() << "DB error:" << query.lastError();
		}
	}
}

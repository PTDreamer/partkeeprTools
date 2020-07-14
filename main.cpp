#include "mainwindow.h"

#include <QApplication>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	if(MainWindow::s_textEdit == 0)
	{
		QByteArray localMsg = msg.toLocal8Bit();
		switch (type) {
		case QtDebugMsg:
			fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
			break;
		case QtWarningMsg:
			fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
			break;
		case QtCriticalMsg:
			fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
			break;
		case QtFatalMsg:
			fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
			abort();
		}
	}
	else
	{
		switch (type) {
		case QtDebugMsg:
		case QtWarningMsg:
		case QtCriticalMsg:
			// redundant check, could be removed, or the
			// upper if statement could be removed
			if(MainWindow::s_textEdit != 0)
				MainWindow::s_textEdit->append(msg);
			break;
		case QtFatalMsg:
			abort();
		}
	}
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	qInstallMessageHandler(myMessageOutput);
	MainWindow w;
	w.show();
	return a.exec();
}

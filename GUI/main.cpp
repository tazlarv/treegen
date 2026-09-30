/// Entry point of Qt TreeGen application.
/// This file uses Qt 5.10.1 - see Licenses in folder External libraries.
/// \file tree_renderGL_widget.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <treegen_app.h>

#include <QtWidgets/QApplication>

/// Entry point of application.
int main(int argc, char *argv[])
{
	QApplication::setStyle("fusion");
	QApplication a(argc, argv);

	QSurfaceFormat format{};
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setSamples(8);
	QSurfaceFormat::setDefaultFormat(format);

	TreegenApp w;
	w.setWindowTitle("TreeGen");
	w.show();

	return QApplication::exec();
}

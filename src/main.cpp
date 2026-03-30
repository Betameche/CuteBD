#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QLabel label("Qt5 + C++23 minimal app is running.");
    label.resize(360, 80);
    label.show();

    return app.exec();
}
